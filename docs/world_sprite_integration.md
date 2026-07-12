# World ↔ Sprite Integration — Implementation Plan

Status: **Phases 1–13 done — feature complete** (backlog items in §5 remain)
Owner: igor
Last updated: 2026-07-09

This document specifies how a loaded `world` (from the TMX loader today, LDtk
later) is turned into drawable sprite resources and rendered, with a resource
cache that loads/unloads only the delta on level switches. It is divided into
small, independently testable phases.

---

## 1. Goals

- Draw a `world`'s tile layers (and later objects, image layers, infinite maps)
  through the existing sprite/texture subsystem.
- Keep the **world layer format-agnostic**: the renderer depends only on a
  neutral per-tile resolution interface, never on TMX concepts (gid encoding,
  wang/terrain, embedded bytes). An LDtk loader must be able to feed the same
  path.
- On a level switch (each level is one `world`), **load/unload only what
  changed** — shared tilesets stay resident.
- Reuse what the tree already ships; do not hand-roll packers, hashes, image
  decoders, or geometry.

### Non-goals (this plan)

- No megatexturing of uniform tilesets (one texture each for now).
- No async/threaded decode in the first cut (hooks noted, deferred).
- No LDtk loader yet — only the *seam* is made LDtk-ready.
- No new gameplay/physics coupling; tile collision stays with the physics side.

---

## 2. Architecture (recap of agreed design)

Three lifecycle tiers, plus a neutral data seam:

```
world (pure data)                     ── neutral resolution API, no render deps
   │  resolve(cell)      → (tileset, local id, transform)
   │  resolve(tileset,N) → (image ref, src rect, origin, size, animation?)
   ▼
world_renderer  (per level, RAII)     ── Tier 3: acquires bundles, culls, draws
   │  holds tileset → bundle handles + camera
   ▼
resource_cache  (long-lived)          ── Tier 2: content-keyed, refcounted,
   │  key: (path,mtime,size) → hash → bundle{atlas,sheet,anim states}
   │  refcount + LRU cold pool; acquire-before-release on switch
   ▼
texture_registry / sprites_manager    ── Tier 1: dumb id stores (exist, hardened)
```

### Key models (locked)

- **Neutral resolution interface.** The renderer asks `world` "for this cell,
  which tileset + local id + transform" and "for this tileset local id, which
  image + source rect + origin + size, and is it animated." gid/first_gid stay a
  loader-private detail. Uniform tilesets, collection-of-images tilesets, and
  LDtk tilesets all satisfy the same interface.

- **Transform superset.** `sprite_flip` (H/V/diagonal) + a general `rotation`
  (degrees). TMX hex-120 maps to +120°; TMX object rotation and LDtk's H/V
  subset use the same field. `cell.flip` composes with a draw-time flip via the
  `^` operator (each flip is an involution).

- **Packing.** Only **collection-of-images** tilesets are packed (stb_rect_pack
  → composited surface → `cpu_texture_atlas`). Uniform tilesets upload as-is.
  Gutter + edge-extrude and multi-page spill are built in from the start.

- **Two-level cache key.** Hot path keys on `(resolved path, mtime, size)` via
  `stat` (no read on a hit). Canonical identity is a 64-bit **content hash =
  two seeded `SDL_murmur3_32` composed**, with byte length kept beside it as a
  collision discriminator. The hash is computed only when bytes are actually
  loaded (it rides the read), cached against the stat-key, and recomputed when
  mtime/size changes (free hot-reload). Content hash carries the
  collection/embedded/packed and cross-path-dedup cases where a path can't.

- **Refcount + acquire-before-release + LRU cold pool.** A bundle refcounts as a
  unit. Switch acquires the new level's bundles *before* releasing the old
  level's, so shared bundles never hit zero. At refcount 0 a bundle goes to a
  budgeted LRU cold pool (still registered, cheaply resurrected), and is only
  unregistered on eviction, in dependency order (states → animations → sheet →
  atlas).

- **Shared-clock animated tiles.** One `sprite_state` per *animated tile type*
  per tileset (not per cell), owned by the bundle and refcounted with it, ticked
  by the existing `sprites_manager::update`. Every cell of that tile reads the
  state's current frame; flip is applied per-cell on top. This is the correct
  Tiled semantics (lockstep), O(distinct animated tiles) not O(cells), and the
  phase survives level switches for shared tilesets.

---

## 3. Reuse inventory (do not hand-roll)

| Need | Use | Location |
|---|---|---|
| Image decode (png, qoi, bmp, gif, jpeg…) | onyx_image `decode(bytes, surface)` | dep `onyx_image` |
| Rect packing | `stb_rect_pack.h` | dep `stb` (already compiled via onyx_image) |
| 32-bit hash primitive | `SDL_murmur3_32` | SDL3 |
| Surface compositing | `sdlpp::surface::blit_to` | dep `sdlpp` |
| Atlas value type (surface+frames) | `cpu_texture_atlas` | `include/neutrino/video/sprite/` |
| GPU atlas / sheet registration | `register_atlas`, `register_sprite_sheet` | sprite subsystem |
| Errors | `THROW_*` / `ENFORCE` | `failsafe` |
| hash_combine, transparent string hash | `neutrino::details::` | `include/neutrino/detail/hash.hh` |

Not available / do **not** use: xxHash via zstd (optional dep + private header).

---

## 4. Phase dependency graph

```
P1 hash ─┐
P2 image ─┼─► P3 packer ─┐
P4 rotation ──────────────┼─► P6 builder ─► P7 cache ─► P8 renderer ─┐
P5 world resolution ──────┘                                          │
P9 camera/cull ─────────────────────────────────────────────────────┼─► P10 draw
                                                                     │
                                          P10 ─► P11 animated ─► P12 objects/image ─► P13 infinite
```

P1–P5 are foundations and can be built in parallel (P3 needs P2's surface type).
P6 onward is largely linear.

---

## Phase 1 — Content hash utility ✅ DONE

**Goal.** A fast, stable 64-bit content hash plus a length discriminator, built
from two seeded `SDL_murmur3_32`.

**Deliverables.** (all landed; `content_key` in `include/neutrino/world/content_key.hh`.)
- `details::hash_bytes(const void*, size_t)` / `hash_bytes(std::span<T>)` —
  composes two seeded `SDL_murmur3_32` into a fixed 64-bit digest, identical on
  every platform. **Done.**
- `struct content_key { std::uint64_t hash; std::uint64_t length; auto operator<=>() = default; };`
  with `content_key content_hash(std::span<const std::byte>) = { hash_bytes(bytes), bytes.size() }`. **Done.**
- A `std::hash<content_key>` specialization for hash-table bucketing — mixes in
  full 64-bit via `hash_combine64`, narrows to `size_t` once at the end (keeps
  both murmur halves influencing the bucket on 32-bit targets). **Done.**

**Design notes.**
- `hash_bytes` returns a full `std::uint64_t` and never narrows to `std::size_t`,
  so the `content_key.hash` value is identical on 32- and 64-bit builds — a
  correctness requirement, since the key may be persisted or shared across
  platforms. Do **not** route the content key through the `std::size_t`
  `hash_combine`.
- `length` is a free, cheap discriminator: two inputs with different lengths can
  never be equal even on a hash collision.
- The `std::hash<content_key>` bucket specialization may use the `std::size_t`
  `hash_combine`/`hash_value` — bucket indices are `size_t` and in-process only.
- Endianness: hash **byte streams** (`std::byte` / `unsigned char`), not spans of
  wider integers, when the key must be cross-platform stable — the byte image of
  multi-byte elements is endianness-dependent.

**Tests** (`test/detail/test_content_hash.cc`, no app context needed):
- Determinism: same bytes → same key across calls.
- Distinctness: a corpus of small distinct byte strings → all-distinct keys.
- Length discriminator: two different-length inputs never compare equal.
- Empty input is handled (no UB).
- **Golden value:** a fixed input hashes to a hard-coded expected `hash` value.
  This pins cross-platform/cross-build stability — if the mixing ever becomes
  `size_t`-width-dependent again, a 32-bit build would fail this test.

**Done when.** Tests pass; util is usable from any TU. — **Met:** 7 cases in
`test/detail/test_content_hash.cc` green (golden `0xE12B6F8F24F63248`); full
suite 470/470.

---

## Phase 2 — Image decode → `sdlpp::surface` ✅ DONE

**Goal.** The missing bridge: turn a file path or byte span into an
`sdlpp::surface`. Generic, no world dependency.

**Deliverables.** (header-only; `include/neutrino/video/image_loader.hh`.)
- Discovery: `sdlpp::image` already wraps onyx_image and exposes
  `load(const std::filesystem::path&)` and `load(std::span<const std::uint8_t>)`,
  both returning `expected<surface, std::string>` — so no new `.cc`, no
  onyx→sdlpp conversion to hand-write. The bridge is a thin wrapper. **Done.**
- `neutrino::load_image(const std::filesystem::path&) -> sdlpp::surface`
  and `load_image(std::span<const std::uint8_t>) -> sdlpp::surface`, both
  forwarding to `sdlpp::image::load` and `THROW_RUNTIME(error)` on failure.
  **Done.**
- CMake: header registered on the neutrino target; `neutrino::sdlpp` +
  `neutrino::onyx_image` already linked PUBLIC. **Done.**

**Design notes.**
- **Chose throw over `expected`** (diverges from the original plan): consistent
  with the rest of the world loader's `THROW_*` convention. If the resource
  cache later wants per-image graceful degradation, add a non-throwing
  `decode_image -> expected` overload beside these; the wrapper is trivial.
- Byte span is `std::span<const std::uint8_t>` so const image buffers (the
  hashed bytes, `world_image::data` via a const `world`) can call it.
- Format auto-detected by onyx_image; no format hint needed.

**Tests.** Covered upstream in onyx_image's own suite (decode correctness,
format detection, malformed input). The neutrino wrapper is a pass-through with
no logic of its own; a compile check confirms the const-span accepts both const
and mutable buffers. No neutrino-side test added.

**Done when.** A path/byte span round-trips to an `sdlpp::surface`. — **Met**
via `sdlpp::image::load`; wrapper compiles against real include flags.

---

## Phase 3 — Rectangle packer + surface compositing ✅ DONE

**Goal.** Given N sized inputs, assign non-overlapping positions with gutter and
multi-page spill (stb_rect_pack), and composite N surfaces into `cpu_texture_atlas`
pages.

**Deliverables.** (`include/neutrino/video/sprite/atlas_packer.{hh}` +
`src/neutrino/video/sprite/atlas_packer.cc`; stb pinned in the top `CMakeLists.txt`.)
- Low level: `pack_atlas(std::span<const rect>, max_w, max_h, margin) -> pack_result`
  — per-input `placement{page, input_index, bounds}` ordered by `(page, input_index)`,
  auto-spilling to new pages; `pages` holds trimmed extents. `pack_atlas(rects, margin)`
  resolves the cap from the renderer. **Done.**
- General primitive: `pack_regions(std::span<const pack_region>, format, …)` where a
  `pack_region` is `{surface, src_rect, mask}` — composites via straight (blend-none)
  `blit_to` with gutter + edge-extrude, converts each distinct source to `format`
  once, preserves or (opt-in) generates the per-frame bitmask. **Done.**
- Adapters over it: `pack_surfaces(span<const surface>)` (whole images) and
  `pack_atlases(span<const cpu_texture_atlas>)` (repack many sheets' frames into
  fewer textures). Each has a convenience overload (format from first input, cap
  from renderer) and an optional `cpu_texture_atlas_mask_options generate`. **Done.**

**Design notes.**
- Gutter: each rect inflated by `2*margin` (symmetric belt); after blitting, edge
  strips are extruded into the belt so linear/scaled sampling never bleeds.
- Page size is the **trimmed** used-extent (incl. gutter), not the cap.
- Multi-page spill built in; a rect larger than the cap throws (terminal).
- Robustness: non-positive tile sizes and overflowing inflations rejected in 64-bit;
  every surface op (`fill`/`convert`/`set_blend_mode`/`blit`) is `ENFORCE`d.
- Build surfaces via raw `SDL_CreateSurface`/`SDL_ConvertSurface`? No — the earlier
  ENFORCE-guts-the-expected bug was fixed in failsafe, so the sdlpp factories are
  used directly with `ENFORCE(x.has_value())`.

**Tests** (`test/video/test_atlas_packer.cc`, 17 cases, headless).
- Packer: exactly-once placement, no overlap + gutter separation, tiles inside the
  trimmed page, multi-page spill, `(page,input_index)` order, oversized/invalid/
  non-positive throws, determinism.
- Compositing: each source lands in its slot, gutter carries the edge colour,
  straight-copy (translucent RGB not premultiplied), heterogeneous formats convert.
- Merge: `pack_atlases` relocates frame pixels intact and carries masks; mask
  generation is opt-in and reflects source alpha; convenience overloads derive
  format+cap; empty input → empty.

**Done when.** Packing is overlap-free and complete; compositing places pixels
correctly with a bleed-safe gutter. — **Met**; full suite 492/492.

---

## Phase 4 — Draw-time rotation (transform superset) ✅ DONE

**Goal.** Add `rotation` to the sprite draw transform and wire it through both
draw paths; this serves hex-120 tiles and object rotation.

**Deliverables.** (`include/neutrino/video/draw.hh` + `src/neutrino/video/draw.cc`.)
- `sprite_draw_params` gains `float rotation_degrees{0}`. **Done.**
- Wired into both paths, threaded through every `draw_sprite` overload:
  - `copy_ex` (non-diagonal): pass the angle and pivot around the flip-adjusted
    visual origin (`center = {origin_x, origin_y}`). **Done.**
  - `render_geometry` (diagonal): rotate the four vertices clockwise (screen
    space) about the anchor `position` after building them. **Done.**
- Rotation validated finite (`ENFORCE`); `0` skips the pivot / rotation loop so
  it is byte-identical to no-rotation. **Done.**

**Design notes.**
- Both paths pivot around the same screen point (the draw `position`/anchor):
  `copy_ex`'s dst-relative center maps to `position`, and the geometry path
  rotates about `position` — so scale, flip, and rotation all resolve around the
  visual origin and a rotated tile stays pinned to its cell anchor.
- Clockwise-positive in screen space, matching `SDL_RenderTextureRotated`, so the
  two paths agree. hex-120 is `rotation_degrees = 120` composed with the cell flip.
- The geometry path's vertex rotation uses `euler::complex::polar` (a 2D rotation
  is a unit complex) rather than hand-rolled trig; `copy_ex` delegates its angle to
  SDL. No bespoke rotation math.

**Tests** (`test/video/test_draw.cc`, app/render context).
- `rotation_degrees = 90 / 120 / -90 / 360` all succeed on the non-diagonal path;
  rotation composed with a diagonal flip (and scale) succeeds on the geometry path.
- `rotation_degrees = 0` succeeds via the no-rotation path.
- Non-finite rotation throws.

**Done when.** Rotation renders through both paths; existing draw tests still
pass. — **Met**; full suite 493/493.

---

## Phase 5 — Neutral tile-resolution interface on `world` ✅ DONE

**Goal.** Expose the format-neutral, render-neutral resolution the bridge
depends on. Pure data — no sprite/texture/GPU types.

**Deliverables.** (`include/neutrino/world/world.hh` + `src/neutrino/world/world.cc`.)
- `struct tile_drawable { const world_image* image; rect src; point origin; bool animated; }`.
  **Done.**
- `world_tileset::drawable(world_local_tile_id) const -> tile_drawable`
  — uniform: `image = &*this->image, src = tile_rect(id)`; collection (tile owns an
  image): `image = &*tile(id)->image, src = {0,0,w,h}`. `origin = {offset_x,
  offset_y}` (tileset draw offset); `animated` set from the tile's animation.
  Throws (via `tile_rect`) for a uniform tile with no shared image / bad id. **Done.**
- `world_tileset::animation_of(world_local_tile_id) const -> const std::vector<world_tile_animation_frame>*`
  (nullptr if not animated). **Done.**
- Cell resolution already existed: `world::tileset_for(gid)` + `to_local(gid)` +
  `world_tile_cell{flip, rotated_hex_120}`. Additive; no behavioral change.

**Design notes.**
- The descriptor references a `world_image` (path/bytes) — the *bridge* turns
  that into a texture. World never touches the GPU.
- `drawable` is where collection-of-images stops being a special case: it hides
  whether the source is a shared atlas image or a per-tile image.
- `origin` is the tileset per-tile draw offset in pixels; the anchor convention
  (bottom-left alignment etc.) is applied by the renderer in Phase 9/10.

**Tests** (`test/world/test_tile_resolution.cc`, pure data, no render context):
- Uniform tileset: `drawable(id).src == tile_rect(id)`, image is the shared image,
  `origin == {offset_x, offset_y}`.
- Collection tileset: `drawable(id).image` is the tile's own image, `src` covers
  the whole image.
- `animation_of` returns frames for an animated tile, nullptr otherwise; the
  `drawable.animated` flag matches.
- `tileset_for` / `to_local` / `to_global` round-trip across two tilesets (incl.
  the Tiled "highest first_gid, not upper-bounded" selection rule).

**Done when.** Both tileset kinds resolve through one descriptor; existing world
tests still pass. — **Met**; full suite 497/497.

---

## Phase 6 — Tileset → render-resource bundle builder ✅ DONE

**Goal.** Given one `world_tileset` (+ loader + packer), produce a registered
bundle: atlas(es), sheet(s), and one animation+state per animated tile. No cache
yet.

**What shipped** (`include/neutrino/video/world/tileset_bundle.hh`,
`src/neutrino/video/world/tileset_bundle.cc`):
- `struct tileset_bundle { std::vector<gpu_texture_atlas_id> atlases; std::vector<sprite_sheet_id> sheets; std::vector<sprite_visual_ref> visuals; std::vector<sprite_state_id> states; std::vector<sprite_animation_id> animations; };`
  plus `visual(id)` / `state(id)` bounds-checked accessors.
- `tileset_bundle build_bundle(const world_tileset&);`, a
  `build_bundle(const world_tileset&, dim max_page_size)` overload with an
  explicit per-page cap (renderer-cap default vs. an injected bound; format is
  still inferred), and `void destroy_bundle(tileset_bundle&) noexcept;`
  (unregisters states → animations → sheets → atlases).

**Design decisions (settled during Phase 6):**
- **Flat direct-index vectors, not `unordered_map`.** Local tile ids are dense
  and 0-based, and Phase 10 resolves `local_id → visual` per cell per frame; a
  `vector` indexed by id is a single contiguous load with no hashing. `visuals`
  and `states` are sized to `tile_count`; absent tiles (sparse collection gaps,
  non-animated tiles) hold the invalid sentinel. (`sorted_array` from physics was
  considered and rejected here: the keyspace is dense so direct index beats its
  binary search, and it would couple `video` to `physics`.)
- **Honest multi-page bundle.** A `sprite_sheet` backs exactly one atlas, so a
  tileset that spills past the page cap yields several pages/sheets and `visuals`
  points each tile at whichever sheet holds it. This drops the earlier
  "visual index == local id" shortcut in favour of the `sprite_visual_ref` seam,
  which also absorbs sparse collection tilesets for free.
- **One unified pack path.** Uniform *and* collection tiles are packed through
  `pack_regions` (uniform: one region per tile at `drawable(id).src` into the
  shared decoded image, deduped so the sheet image is decoded once; collection:
  each tile's own whole image). Packing every tile gives all of them a gutter
  belt, so uniform tiles no longer bleed under draw-time scaling. `drawable()`
  is consulted per tile, so mixed tilesets (some tiles with own images) work.
- **`origin`** is derived from `drawable(id)` into each `sprite_visual::origin` —
  which is why the sheet is built manually with `add_visual`, not via the
  origin-less `register_sprite_sheet(cpu_atlas)` convenience. Per Phase 10's D1 the
  stored value is the **bottom-left draw anchor** `(−off_x, src.h − off_y)`, not the
  raw tileset offset, so the renderer can anchor each cell at its bottom-left and get
  Tiled placement (oversized tiles extend up; offset carries the correct sign).
- **Transactional build.** All decoding + packing (the throwing work) precedes
  any `register_*`; a failure mid-registration tears the partial bundle down and
  rethrows, so a failed build leaks nothing.

**Tests** (`test/video/world/test_tileset_bundle.cc`, app context — surfaces are
fabricated in-memory and BMP-encoded into `world_image::data`, so no on-disk
assets): uniform → one valid visual per tile, one sheet per page, no states;
collection → present tiles resolve, gaps stay invalid, single page; **spill →
under an explicit 64×64 cap two 40×40 tiles land on separate pages, giving two
atlases, two sheets, and the two tiles resolving through different sheets** (the
ref-map's whole reason for being); animated → one shared state per animated id,
`sprite_state_appearance` at t=0 resolves to the first frame's visual; teardown →
order-clean, tileset rebuilds afterwards.

**Update (canonical format — shipped).** The atlas pages were originally packed in
whatever format the first source surface had. That broke on **palettized (indexed)
PNGs**: a palettized GPU texture can't be sampled by the sprite path ("Texture
doesn't have a palette"). Bundles now pack in a **canonical `RGBA8888`** regardless of
source (new `pack_regions(regions, format, margin)` convenience; the packer already
converts every source on the way in).

**Update (PNG tRNS transparency — fixed upstream).** `onyx_image` (lodepng) was
emitting palettized PNGs as an indexed surface with an RGB palette + single
transparent-index, which downstream consumers dropped when rebuilding an SDL/GPU
palette — so a transparent sprite drew with a solid background. Fixed in
`devbrain/onyx_image` (`src/codecs/png.cpp`): the compact indexed form is kept only
for fully-opaque palettes; a palette with `tRNS` transparency now decodes to RGBA so
per-pixel alpha survives. Neutrino carries no workaround for this. (Surfaced by
`forest`'s squirrel sprite.)

**Update (image-collection atlas tilesets — shipped).** A collection tileset can
slice one shared image into per-tile sub-rectangles (Tiled `<tile x y w h><image/>`)
and number its tiles **sparsely, past `tile_count`**. Support added: `world_tile`
gained an optional `source_rect` (loader parses the tile's `x/y/w/h`; `drawable`
returns it in place of the whole image), and the bundle sizes its id-indexed tables
to `max(tile_count, highest id + 1)` so the sparse high ids resolve. Overhang uses the
sub-rect size, not the shared image's. (Surfaced by the `forest` example map.)

**Done when.** A tileset of each kind builds a drawable bundle and tears down
cleanly. — **Met:** 5 cases (incl. deterministic multi-page spill), full suite
502/502.

---

## Phase 7 — Content-keyed resource cache ✅ DONE

**Goal.** The heart of "load/unload only the delta": a cache mapping tileset
identity → bundle, with refcounts and an LRU cold pool.

**What shipped.**
- **`src/neutrino/utils/lru.hh` — `neutrino::utils::lru_index<Key>`**: a generic,
  capacity-bounded, recency-ordered *key* set (ordering only, no values), extracted
  and tested in isolation (`test/utils/test_lru.cc`, 7 cases). `std::list` (front =
  LRU, back = MRU) + `unordered_map<Key, list::iterator>` → O(1) `touch`
  (insert/move-to-MRU, returns any evicted key), `erase`, `pop_oldest`, `contains`.
  The test target gains `${PROJECT_SOURCE_DIR}/src/neutrino` on its include path so
  internal headers like this are testable directly.
- **`include/neutrino/video/world/resource_cache.{hh,cc}`**: public
  `resource_cache` (pimpl, so the internal `lru_index` + maps stay out of the public
  header) and a `bundle_handle { content_key key; const tileset_bundle* bundle; }`
  with `visual`/`state` resolution passthrough.
  - `acquire(const world_tileset&)` — hit: `++refcount` (erasing from the cold index
    if it was idle); miss: `build_bundle` (Phase 6) then insert at 1. Builds *before*
    inserting, so a build failure leaves the cache untouched.
  - `release(const bundle_handle&)` — `--refcount`; at 0 `cold.touch(key)`, and if
    that overflows the budget, `destroy_bundle` + erase the evicted LRU entry.
  - `resident_count()` / `cold_count()` introspection.

**Design decisions (settled during Phase 7):**
- **Two-level file identity.** A file-backed image is identified by a
  `(path → mtime, size)` stat key mapped to a cached content hash (Phase 1),
  recomputed only when the file changes — so re-acquiring an unchanged tilesheet is
  a stat, not a re-hash. Embedded images hash their bytes directly.
- **The key folds in geometry + per-tile images/animations, and excludes
  `first_gid`.** The bundle's visuals are geometry-specific, so tile size etc. must
  be in the key; `first_gid` is a *map-level* id assignment, not tileset content, so
  excluding it lets the same tileset share one bundle across maps that number it
  differently. (Sharing a *texture* across tile-size variants stays the deferred
  "texture-vs-sheet key split.")
- **Bundles never move.** They live in one pointer-stable `unordered_map` node for
  their whole life; only their membership in the cold `lru_index` toggles, so a held
  `bundle_handle` never dangles while acquired. This is why the LRU is ordering-only
  rather than a value-owning cache.
- **Cold budget is a bundle count** (default 8); cold bundles stay registered and
  keep ticking, so re-acquire is instant. Refcount is *intent*; the sprite manager's
  `uses()` guard stays the safety net. Destructor tears down every bundle.

**Tests** (`test/video/world/test_resource_cache.cc`, app context — observing build
vs. share vs. rebuild through `sprite_sheet_id` identity, no test hook): acquire
twice → one build + shared bundle; release-to-cold stays resident and resolvable;
cold-budget overflow evicts + tears down the LRU (rebuild yields a newer sheet id);
identical content → shared bundle; a rewritten file-backed image → new bundle; and
the **switch delta** — acquire {A,B}, then {B,C}, release {A,B} → B never rebuilt
(same bundle + sheet id throughout), C built once, A idle in the cold pool.

**Done when.** The switch-delta test proves shared bundles survive and only the
difference loads/unloads. — **Met:** 6 cache cases + 7 LRU cases, full suite
515/515.

---

## Phase 8 — `world_renderer` binding (lifecycle only) ✅ DONE

**Goal.** A per-level RAII object that acquires bundles for every tileset in a
world and releases them on destruction. No drawing yet.

**What shipped** (`include/neutrino/video/world/world_renderer.{hh}`,
`src/neutrino/video/world/world_renderer.cc`):
- `world_renderer(const world&, resource_cache&)` — acquires a bundle per tileset
  into a `std::vector<bundle_handle>` **index-aligned with `world::tilesets()`**
  (so Phase 10's `tileset_for(gid)` → index is pointer arithmetic, no map).
  Destructor releases all handles.
- `world_renderer(const world&)` — defaults to the **application-wide cache**
  published through the service locator (see below), so call sites just write
  `world_renderer r(level)`. The explicit-cache ctor stays for tests/tools.
- Non-copyable, move-only. Move leaves the source inert (empty handles, null
  cache) so a moved-from renderer releases nothing; move-assign releases the
  target's own handles before adopting the source's.
- `switch_to(const world& next)` — the switch helper: `*this = world_renderer(next,
  *m_cache)`, so the temporary acquires *next*'s bundles before the move-assignment
  releases the current level's. Acquire-before-release is encoded in one place;
  call sites can't reverse it.
- `tileset_count()` / `handle_for_tileset(index)` accessors (the latter also serves
  Phase 10).

**Service-locator integration (agreed during Phase 8).** `resource_cache` is
Tier 2 sitting directly on Tier 1 (`texture_registry` + `sprites_manager`), which
are already app-owned and published through the internal `service_locator`, and the
cache already reaches them transitively via `register_*`. So the `application` now
**owns a default `resource_cache`** (a `unique_ptr` in its pimpl) and publishes a
pointer through the locator, exactly like the other render services. The cache stays
an ordinary constructible class (not a singleton) so tests still build local caches
with a chosen cold budget.
- **Teardown order is the one constraint:** `destroy_bundle` destroys GPU textures
  and unregisters through the sprite/texture services, so the cache must tear down
  while those (and the renderer) are still live. It is reset in `application::on_quit`
  — after scenes (and their `world_renderer`s) release their handles, before the
  renderer teardown and `clear_application` — with an idempotent `reset()` in
  `~application` as a fallback.

**Tests** (`test/video/world/test_world_renderer.cc`, app context): construct →
one refcount per tileset (`resident_count`), destroy → all released to cold; a
tileset shared by two renderers survives the first's destruction (same bundle
pointer, only the departed tileset goes idle); move-construct and move-assign each
release exactly once (no double-release); `switch_to` keeps the shared tileset's
bundle *and* sheet id across the switch with only the departed tileset going idle;
the default ctor acquires through the located cache.

**Done when.** Renderer lifetime drives cache refcounts correctly, including the
shared-tileset switch case. — **Met:** 5 cases, full suite **522/522**.

---

## Phase 9 — Camera + culling (finite orthogonal) ✅ DONE

**Goal.** A minimal camera and the visible-cell-range computation for finite
orthogonal tile layers. Pure math, no drawing, no render context.

**Scope.** Finite orthogonal only. Infinite/chunked culling is Phase 13; isometric
and hex are later. This phase answers *which cells are on screen*; Phase 10 turns
cells into draws.

### Coordinate model (the part worth pinning down)

Three spaces:
- **Layer-cell** `(cx, cy)` — integer grid index into a `world_tile_layer`.
- **World-pixel** — `world_point` (float). A cell occupies
  `[cx·tw, (cx+1)·tw) × [cy·th, (cy+1)·th)`, where `(tw, th)` is the **map** tile
  size (`world::tile_width/height`), i.e. the cell stride.
- **Screen-pixel** — `point` (int), what `draw_sprite` consumes.

The camera holds a **float** world position and a zoom:

```cpp
struct camera {
    world_point offset{0.0f, 0.0f};  // world-pixel mapped to the viewport's TOP-LEFT corner (parallax 1)
    float       zoom{1.0f};
};
```

Per layer, parallax and the layer's static offset fold into an effective origin:

```
layer_origin(cam, layer) = { cam.offset.x·layer.parallax_x - layer.offset_x,
                             cam.offset.y·layer.parallax_y - layer.offset_y }
```

and the transform for any world point in that layer is:

```
to_screen = round( (world_pt - layer_origin) · zoom )
```

`visible_cell_range` is exactly the inverse of that same expression, so culling and
drawing can never drift apart.

### Deliverables (`include/neutrino/video/world/camera.hh` [+ `camera.cc`])

```cpp
struct camera { world_point offset{0,0}; float zoom{1.0f}; };

point to_screen(const camera&, world_point p) noexcept;                        // parallax 1, no layer offset (HUD/objects)
point to_screen(const camera&, const world_layer_header& layer, world_point);  // folds parallax + layer offset

struct cell_range {                        // half-open [x0,x1) x [y0,y1), already clamped to the layer
    int x0{0}, y0{0}, x1{0}, y1{0};
    [[nodiscard]] bool empty() const noexcept { return x1 <= x0 || y1 <= y0; }
};

cell_range visible_cell_range(const world&, const world_tile_layer&, const camera&, dim viewport);
```

The math inside `visible_cell_range`:

```
o  = layer_origin(cam, layer)
wl = o.x;  wr = o.x + viewport.w / zoom      // world-x at screen 0 and at screen width
wt = o.y;  wb = o.y + viewport.h / zoom
x0 = clamp(floor(wl/tw), 0, layer.width);   x1 = clamp(ceil(wr/tw), 0, layer.width)
y0 = clamp(floor(wt/th), 0, layer.height);  y1 = clamp(ceil(wb/th), 0, layer.height)
```

### Design decisions (agreed)

- **Separate render-space camera, in `world_point` — deliberately not the physics
  `vec`/`view`.** The physics module already has a world-position type
  (`physics::vec = euler::vec2f`) and a proto-camera `view { origin, pixels_per_unit,
  y_up }` in `simplex/collide/bridge.hh` (whose comment notes "a real camera does not
  yet exist"). We do **not** reuse them here. Physics is deliberately kept in pure
  world units with no SDL/display dependency, bridged to display space in exactly one
  place; the tile world is authored in TMX **pixels** (`world_point`), which is the
  space the camera culls and positions. So the camera speaks tile-pixel space, and
  making it follow a physics body is an explicit `physics::vec → pixel` conversion at
  the call site (a gameplay/scene concern), not a shared type. This keeps two
  coordinate vocabularies — intentional, to preserve the physics module's clean
  separation — rather than tying the tile world into the physics coordinate model.
- **`world_point` (float) offset, not the plan's original `point offset`.** Parallax
  multiplies the offset by a float and zoom is fractional; an int camera can't scroll
  or parallax sub-pixel. Float world offset, rounding to int only at the final
  `to_screen` — the same "diverge-when-justified" call as P4's rotation.
- **Half-open, pre-clamped `cell_range` as a type**, not a bare `{x0,y0,x1,y1}`.
  Matches C++ iteration and can never yield negative / out-of-bounds indices.
- **Offset = viewport top-left**, with a `center_on(world_point, dim viewport)`
  helper that sets `offset = target - (viewport/2)/zoom`. Top-left keeps the core
  transform simple; centring is one line on top.
- **`to_screen` / `layer_origin` inline in the header** (Phase-10 per-cell hot path
  wants inlining); `visible_cell_range` in a `.cc` (once per layer per frame).
- **Zoom multiplies into `sprite_draw_params.scale`** at draw time (Phase 10).
  `visible_cell_range` validates `zoom > 0` (and finite) once per layer via `ENFORCE`;
  `to_screen` assumes a valid camera (no per-cell check).
- **Orthogonal-only, lenient.** Computed orthogonally; isometric/hex are future
  phases. No throw when `orientation() == unknown`, since manually-built test worlds
  often leave it unset.

### Deferred (with rationale, not silently)

- **Oversized-tile overhang** — `visible_cell_range` returns the exact *grid*
  intersection. A tileset with tiles taller than the map cell draws them extending
  upward (bottom-aligned), so a tile whose cell is just off the top edge can still be
  visible. Phase 10 inflates the range by the tileset's known max overhang (in cells)
  before iterating, keeping this function a clean grid intersection.
- **Per-cell rounding seams** — adjacent tiles can differ by 1px under fractional
  zoom; Phase 10 handles it by rounding shared edges consistently (round position and
  position+size, not position + scaled size).

**Tests** (`test/video/world/test_world_camera.cc`, no render context):
- A camera over a large layer yields a cell range matching the viewport rect.
- Offset/zoom shift and shrink/grow the range as predicted.
- Ranges clamp at all four layer edges (no negative / out-of-bounds indices).
- Parallax < 1 shifts a background layer less than parallax = 1; layer offset shifts
  the range correctly.
- A camera fully off the layer → `empty()`.
- `to_screen` round-trips a known world point to the expected screen pixel under
  offset/zoom/parallax; `center_on` puts the target at the viewport centre.

**Done when.** Culling returns exactly the on-screen cell rectangle for varied
camera states (offset, zoom, parallax, edge clamping), and `to_screen` agrees with
the same transform `visible_cell_range` inverts. — **Met:** 10 cases in
`test/video/world/test_world_camera.cc`, full suite **535/535**.

**Update (parallax — shipped, look-at camera).** Parallax layers (factor ≠ 1) drew in
the wrong place through several iterations against the TMX model (top-left offset +
view-centre anchor + separate origin + a `(1−factor)` inversion — four places to get a
sign or term wrong). The final form **reparameterises the camera as a look-at**:
`camera{ target, zoom, parallax_rest }`, where `target` is the world point shown at
the screen centre and `parallax_rest` is the target value at which layers align. The
whole transform collapses to
`screen(p) = viewport/2 + (p − eff_target)·zoom`, `eff_target = rest + factor·(target
− rest)`. This removes the tangle: the viewport appears only as a centring term (never
woven into the factor), `factor` reads directly (0 static, 1 follows), and the origin
sign is self-evident — layers align exactly when `target == rest`. `world_renderer::
draw` fills `parallax_rest` from the map's `parallaxoriginx/y` (loader-parsed); the
viewer's pan/zoom/clamp operate on `target` and got simpler. Same visual result as the
view-centre formula, just without the foot-guns. (Verified against Tiled's
`MapScene::parallaxOffset` and the `forest` screenshots.)

**What shipped / divergences from the plan above.**
- `camera`, both `to_screen` overloads, and `eval_layer_origin` are header-inline in
  `include/neutrino/video/world/camera.hh`; `visible_cell_range` is out-of-line in
  `src/neutrino/video/world/camera.cc` (once per layer per frame; carries the
  `ENFORCE(zoom > 0 && finite)` guard). `NEUTRINO_EXPORT` on the out-of-line symbol.
- **`to_screen` snaps with `round`** (nearest pixel), as the coordinate model here
  specifies. Cull edges use `floor` (near) / `ceil` (far) so the range includes every
  cell the viewport touches; the two conventions are mutually consistent.
- **Layer offset was ported to `world_point` (float).** `world_layer_header.offset_x/y`
  (`double`) became a single `world_point offset` — the world's canonical world-space
  vector, matching the space the camera computes in. The TMX loader narrows its
  double accumulation into it; `world_tileset.offset` (int draw offset) is unchanged.
- **`center_on` is deferred, not shipped.** The `offset = viewport top-left` model and
  `to_screen` round-trips are covered; the centring helper can land with Phase 10 when
  a call site needs it (it is one line on top of the existing transform).
- Oversized-tile overhang and per-cell rounding seams remain deferred to Phase 10 as
  the plan states.

---

## Phase 10 — Tile-layer draw path (static tiles) ✅ DONE

**Goal.** Render finite orthogonal tile layers of static tiles, pixel-correct
under camera offset/zoom/parallax, honoring per-cell flip + hex-rotation.

### Placement model (the part worth pinning down)

Draw is anchor-based: `draw_sprite(position, visual, params)` computes
`dst_topleft = position - visual.origin·scale` (see `draw.cc`). So *where a tile
lands is split* between the per-cell `position` the renderer computes and the
`origin` baked into the visual. Getting that split right is the whole phase.

Tiled orthogonal semantics: a tile's **bottom-left** aligns to its cell's
bottom-left, the tileset draw offset shifts it, and a tile taller than the map
cell **extends upward**. The tile's top-left in world pixels is:

```
TL_world = ( cx·mtw + off_x , (cy+1)·mth − pth + off_y )
```

where `mtw,mth` = map cell size, `pth` = tile pixel height, `off` = tileset draw
offset.

The clean split: the **renderer positions each cell at its plain bottom-left**,
`world_anchor = (cx·mtw, (cy+1)·mth)`, `position = viewport_origin +
to_screen(cam, layer, world_anchor)` (see D4 for `viewport_origin`); the
**bundle bakes the image-local anchor** `origin = (−off_x,
pth − off_y)`. Then `dst = position − origin·scale` reduces exactly to
`TL_world`. Same-size tiles with no offset → `origin = (0, pth)` and it just
works; oversized tiles extend up for free; `off` gets the correct sign. `mth` is
map-level, so the bundle only needs `pth` (= `drawable(id).src.h`, in hand at
build time) — it never bakes in the map cell size and stays map-agnostic/shared.

### Design decisions (agreed)

- **D1 — origin reconciliation (revisits Phase 6).** The bundle today stores
  `sprite_visual.origin = (+off_x, +off_y)` — wrong sign for a *subtracted* origin
  and missing the bottom-left term; it is correct only when `off = 0` **and** the
  tile size equals the cell size. Phase 10 changes the built origin to
  `(−off_x, drawable(id).src.h − off_y)` (a ~2-line change through
  `world_tileset::drawable`/`tileset_bundle.cc`), and the renderer anchors at the
  cell bottom-left. This makes both phases correct and **subsumes the
  Phase-9-deferred oversized-tile overhang** at the draw origin.
- **D2 — `draw()` returns stats, never throws on content.** A per-cell failure
  must not kill the scene. `draw` returns `draw_stats { std::size_t drawn,
  skipped, failed; }`: a *non-empty* gid that resolves to no tileset or an invalid
  visual increments `skipped`; a `draw_sprite` error increments `failed`; a
  successful tile draw increments `drawn`; empty cells (gid 0) are not counted (they
  are absent, not skipped). Nothing throws. Testable ("everything drew, nothing
  failed") without a hook.
- **D3 — fractional-zoom seams.** Originally deferred, then **fixed** when the map
  viewer surfaced a black grid under wheel zoom: `round(x·z) + round(w·z) ≠
  round((x+w)·z)`, so independently-rounded position and size left 1px gaps. The tile
  path now derives the destination rect from **two rounded world corners** of the
  tile's pixel AABB via a new `draw_sprite(rect dst, visual, flip)` overload — a
  shared world edge rounds to one screen pixel, so neighbours meet exactly. Rotated
  and diagonally-flipped cells keep the anchor/scale path (the AABB blit is H/V only)
  and may still seam; both are rare and not seam-sensitive. **Tile objects share the
  same seam-free path** — cells and objects both go through one `draw_gid_at(gid,
  world_anchor, …)` helper — so side-by-side background bands (e.g. a parallax
  layer's four objects) don't show 1px gaps under fractional zoom either. Image
  layers are a single picture (nothing adjacent to seam against), so they keep the
  position overload.
- **D4 — the viewport is a destination `rect`, not a bare size.** The camera says
  *what world region* is shown; it does not say *where on the target* the map is
  painted. `to_screen` maps the camera's viewport top-left to the target's origin,
  so with only a `dim` a map can never be inset into a sub-region (split-screen, a
  UI panel, a letterboxed view). `draw` therefore takes a screen-space
  `rect viewport`: its size `{w,h}` drives culling exactly as before, and its
  top-left `{x,y}` translates every tile's screen position —
  `screen = {viewport.x, viewport.y} + to_screen(cam, layer, world_anchor)`.
  `rect{0,0,w,h}` is the whole-window default. The *render target itself* stays
  implicit (`get_renderer()`, like every other draw); an offscreen pass sets the
  SDL render target before calling `draw`, unchanged by this API. Clipping edge/
  overhang tiles to the viewport rect (so a sub-region can't bleed into its
  neighbor) is a one-line `set_clip_rect` guard, noted as an optional refinement.

### Deliverables

- `world_renderer::draw(const camera& cam, const rect& viewport) -> draw_stats`:
  - Iterate `m_world->tile_layers()` in map order; skip `!layer.visible`.
  - Cull with `visible_cell_range(*m_world, *layer, cam, {viewport.w, viewport.h})`
    (Phase 9), **inflated for overhang** (below).
  - Per non-empty cell `(cx,cy)`: `gid → tileset_for → index (pointer arithmetic
    vs `tilesets()`) → handle_for_tileset(index) → to_local(gid) →
    handle.visual(local)`; then `draw_sprite({viewport.x, viewport.y} +
    to_screen(cam, layer, cell_bottom_left), visual, {scale = zoom, flip =
    cell.flip, rotation_degrees = cell.rotation_degrees})`.
- **Overhang inflation.** A tile larger than its cell (or with a draw offset)
  overhangs its grid rect — with the bottom-left anchor, a taller tile pokes
  *upward* out of cells below the viewport, a wider one *rightward* out of cells to
  the left, and an offset can push either way. Rather than reason per-direction,
  inflate the culled range **on all four sides** by `ceil(max_overhang / cell)`
  cells, where `max_overhang = max over this level's tilesets of
  (max tile px size − cell size, plus |off|)`, then re-clamp to the layer.
  Computed once at construction (tilesets are fixed for the level), applied as a
  cheap clamp-adjust each frame; over-including an off-screen border cell is
  harmless (it clips). This pays the Phase-9 overhang deferral.

**Design notes.**
- Empty cells (gid 0) skipped before any resolution.
- gid→bundle is pointer arithmetic: `tileset_for` returns a pointer into
  `tilesets()`, and the handle table is index-aligned with it (Phase 8), so no map.
- The `draw_sprite(position, sprite_visual_ref, params)` overload resolves the
  sheet/atlas internally — no per-draw service lookup in the renderer.
- Draw order follows the world's render order within a layer (cheap to honor;
  matters once overhang/overlap draws overlap).

**Tests** (`test/video/world/test_world_draw.cc`, app/offscreen context):
- A small finite world renders → `stats.failed == 0`, `stats.drawn == non-empty
  cell count`.
- A flipped cell and a hex-rotated (120°) cell draw without error.
- A known tile lands at the predicted screen position under a given camera
  (pixel readback if available; else assert against the same `to_screen` the
  renderer uses).
- An oversized tile (taller than the cell) still draws when its anchoring cell is
  just *below* the viewport (it overhangs upward — overhang inflation catches it).
- An out-of-range/garbage gid → counted in `stats.skipped`, scene survives.

**Done when.** A finite orthogonal map renders its static tiles at correct
positions under camera transforms (offset/zoom/parallax), with flip and
hex-rotation, oversized tiles bottom-aligned and culled with overhang, and a
garbage gid skipped rather than fatal. — **Met:** 8 cases in
`test/video/world/test_world_draw.cc`, full suite **543/543**.

**What shipped / notes.**
- D1 (bottom-left anchor baked into `sprite_visual.origin`), D2 (`draw_stats`
  return, per-cell non-throwing), D3 (fractional-zoom seams left deferred), and D4
  (destination `rect` viewport) all landed as specified.
- **Render order is honored within a layer** — `right/left` sets the column
  direction, `*_down/*_up` the row direction (y outer), so overlapping/oversized
  tiles paint in the correct z-order. A test drives all four orders and confirms
  full-range coverage.
- **Not asserted: pixel-exact positions.** Tests verify plumbing (draw counts,
  no-throw, cull + overhang + order coverage), not exact screen pixels — that would
  need render-target readback under the dummy driver. The placement math is verified
  by construction (D1).
- Object and image layers are still stubs (Phase 12); animated tiles use the static
  path for now (Phase 11 swaps `visual(local)` for the shared state's current frame).

---

## Phase 11 — Animated tiles (shared-clock draw integration) ✅ DONE

**Goal.** Draw animated tiles in lockstep off the shared per-tile clock, composing
each cell's flip/rotation on top.

**What shipped.** A single branch in the tile draw loop
(`world_renderer::draw_layer`): after resolving `local`, if `handle.state(local)`
is valid the cell draws through that shared `sprite_state_id`
(`draw_sprite(pos, state, params)`), otherwise through the fixed
`handle.visual(local)`. The draw stats/skip logic is unchanged. Almost everything
this phase needs already existed:
- **Shared state per animated tile** — Phase 6's `build_bundle` registers one
  `sprite_state` per animated local id; every cell of that tile reads the same
  state, so lockstep is automatic (one clock, not per-cell).
- **Ticking is free** — `application` already calls `sprites_manager::update(dt)`
  each frame (`application.cc`), which advances every registered state; the
  renderer's held bundle keeps them resident, and Phase 7 cold bundles keep ticking,
  so a switched-away-but-resident tileset stays in phase.
- **Transform composition is free** — `draw_sprite(pos, state, params)` resolves the
  current frame from the manager and composes `appearance.flip ^ params.flip` and
  `params.rotation_degrees` (`draw.cc`), so the cell flip and hex-120 apply on top of
  the current frame with no extra code.

### Design decision (diverges from the original plan above)

- **Resolve per cell, not into a per-frame table.** The plan called for a
  `local_id → current visual` table resolved once per frame ("O(distinct animated
  tiles)"). But **lockstep comes from the shared state, not the table** — the table
  is only a perf optimization saving one `manager.appearance(state)` call (a map
  lookup + short frame scan) per animated cell, which is negligible for realistic
  animated-cell counts. The per-cell `draw_sprite(state)` is one branch, keeps
  `draw_layer` `const` (a table would need a `mutable` scratch buffer), and is
  correctness-identical. The table remains a documented optimization to add only if
  a huge animated field measures hot. This revises the done-criterion from
  "O(distinct animated tiles)" to **"O(animated cells), one shared clock."**

**Tests** (`test/video/world/test_world_draw.cc`, app context; the clock is advanced
in-test via `sprites_manager::update`):
- An animated tile's resolved current visual is the first frame at t=0 and the
  second frame after advancing past the first frame's duration; the animated field
  still draws every cell.
- A flipped animated cell draws without error (composition through the state path).
- **Resident switch** — advance the clock into frame 1, `switch_to` a world sharing
  the identical tileset; the shared `sprite_state_id` is unchanged (not rebuilt) and
  its appearance is still frame 1 (no reset).

**Done when.** Animated tiles play in lockstep off one shared clock (O(animated
cells)), compose per-cell flip/rotation, and survive resident-tileset switches
without a phase reset. — **Met:** 3 added cases (11 in the draw suite), full suite
**546/546**.

---

## Phase 12 — Tile objects + image layers ✅ DONE

**Goal.** Render object-layer tile objects (gid) and image layers alongside tile
layers.

**What shipped.**
- **`draw_gid(gid, pos, params, stats)`** — the cell resolution/draw (tileset_for →
  aligned handle → visual/animated-state → draw + count) extracted into one helper,
  now shared by the tile-cell loop and tile objects. Animated tile objects get the
  shared-clock behaviour for free.
- **Tile objects** (`draw_layer(world_object_layer)`): iterate `objects`, take the
  `world_object_base` of each variant; a visible object with `gid != 0` draws via
  `draw_gid` at `to_screen(cam, layer, base.origin)` with `{zoom, base.flip,
  base.rotation}`. Tiled anchors a tile object by its **bottom-left** at its origin
  and rotates about that point — which is exactly the bundle's baked bottom-left
  visual origin and `draw_sprite`'s pivot, so objects reuse the tile placement math
  unchanged.
- **Image layers** (`draw_layer(world_image_layer)`): each image-layer picture is
  wrapped as a **synthetic one-tile collection tileset** and acquired through the
  same `resource_cache` at construction (held in a `layer* → bundle_handle` map,
  released/moved with the rest). It draws once at the layer origin, with parallax/
  offset riding through `to_screen`.

**Design decisions.**
- **Image layers go through the cache, not a bespoke path.** Wrapping the image as a
  content-keyed bundle means an identical background shared across levels stays
  resident across a `switch_to` (the Phase-7 delta-loading guarantee) with zero new
  lifecycle code — build/teardown/refcount are all reused.
- **Non-gid objects are absent, not skipped.** Shapes, text, points and hidden
  objects are handled by the debug/UI layer, not sprite-rendered; like an empty
  cell they are not counted in `draw_stats`.

**Deferred (documented, not silent).**
- **Object width/height resize** — `sprite_draw_params.scale` is a single uniform
  float, so a tile object authored at a non-native size currently draws at native
  size (scaled only by zoom). Per-axis object scaling needs a draw-params extension.
- **Object `draw_order`** (`top_down` vs `index`) — objects draw in list order;
  y-sorting for `top_down` is a small follow-up (matters only for overlapping
  objects).
- **Large image-layer standalone upload** — the picture is packed (gutter belt) like
  any tile; a background larger than the max atlas page would throw. A direct
  standalone-texture path is the fix, tracked in the backlog.

**Tests** (`test/video/world/test_world_draw.cc`): a tile object draws through the
gid path (`drawn == 1`); a shape (gid 0) and a hidden object are neither drawn nor
counted; a rotated + diagonally-flipped tile object draws clean; an animated tile
object plays off the shared clock; an image layer draws its picture once.

**Done when.** Tile objects and image layers render alongside tile layers. —
**Met:** 5 added cases (16 in the draw suite), full suite **551/551**.

---

## Phase 13 — Infinite / chunked maps ✅ DONE

**Goal.** Support infinite maps by culling to intersecting chunks.

**What shipped.**
- **`visible_cell_bounds`** — the unclamped floor/ceil cell rect was factored out of
  `visible_cell_range` (which now just clamps it to the layer). Infinite layers have
  no finite `width`/`height` to clamp against and address arbitrary — including
  negative — cell coordinates, so the chunk path culls with the unclamped bounds.
- **`draw_cell(cell, cx, cy, ...)`** — the per-cell anchor/position/`draw_gid` step
  extracted from the finite loop and reused by both paths, so finite and chunked
  cells resolve and place identically.
- **Chunk path** in `draw_layer(world_tile_layer)`: when a layer has chunks, compute
  the overhang-inflated unclamped bounds and, for each `world_tile_chunk`, iterate
  only the intersection of its cell rect with those bounds (the loops run zero times
  for a non-intersecting chunk — so far-away chunks are never touched). A malformed
  chunk (bad dims or short cell vector) is skipped rather than indexed out of range.
  World cell `(chunk.x + lx, chunk.y + ly)` feeds the same bottom-left anchor as a
  finite cell.

**Deferred.** Render order *across* chunks — cells within a chunk draw in
`right_down` order and chunks in list order; full cross-chunk z-ordering (matters
only for overlapping oversized tiles spanning chunk seams) is a follow-up, like the
finite path's per-layer order but lifted over the chunk grid.

**Tests** (`test/video/world/test_world_draw.cc`): a two-chunk map draws only the
visible corner of the near chunk (`drawn == 16`) and never visits the far one; a
chunk at negative coordinates renders under a negatively-offset camera; a camera off
every chunk draws nothing.

**Done when.** An infinite map renders only its visible chunks. — **Met:** 3 added
cases (19 in the draw suite), full suite **554/554**.

---

## 5. Backlog (explicitly deferred)

- **Async load / preload.** Decode on a worker; upload on the render thread.
  Preload the next level's *new* tilesets during a transition (the refcount model
  already supports acquiring early).
- **Parsed-`world` CPU cache** keyed by map path — avoid re-parsing/decompressing
  on revisit. Separate from the GPU resource cache.
- **Megatexture uniform tilesets** into shared pages for batching — only if
  draw-call count is measured to matter; doesn't change the resolution interface.
  The merge primitive already exists: `pack_atlases(std::span<const cpu_texture_atlas>)`
  (a thin adapter over `pack_regions`, which packs `{surface, src_rect, mask}`
  regions). **Constraint:** only merge content with the *same lifetime* (e.g. one
  level's static tiles) — merging across the Phase-7 refcount/delta boundary
  destroys per-tileset unload granularity.
- **Generate bitmasks while packing.** *Done.* `pack_regions`/`pack_surfaces`/
  `pack_atlases` take an optional `cpu_texture_atlas_mask_options generate`: an input
  region's mask is preserved (correct across relocation + format conversion — the
  mask is a format-agnostic solid/transparent grid), and for regions with no mask
  the policy evaluates one via `cpu_texture_atlas_frame::evaluate_bitmask` against
  the original (pre-conversion) surface. Phase 6 just picks the policy (key-colour
  vs alpha). Convenience overloads (format from first input, cap from the renderer)
  also carry it.
- **Texture-vs-sheet key split** — share a texture across different tile-size
  uses of the same image. Only if a real case needs it.
- **Layer opacity/tint** compositing beyond visible/skip.
- **LDtk loader** — populates the same `world` + neutral resolution interface.

---

## 8. Non-orthogonal maps (post-Phase-13)

**Hexagonal — done.** A `layout` seam concentrates the only orientation-specific
logic in two functions: `cell_to_world(world, cx, cy)` (the cell's bottom-left anchor)
and `visible_cell_bounds`'s cell derivation. Orthogonal uses the plain grid;
**hexagonal** uses Tiled's staggered formula (`HexagonalRenderer::tileToScreenCoords`
+ `doStaggerX/Y`, driven by `hex_side_length` / `stagger_axis` / `stagger_index`),
verified by unit test against hand-computed positions. Hex/non-orthogonal tiles take
the **anchor draw path** (they overlap on a staggered layout, so the seam-free
axis-aligned AABB blit doesn't apply; their transparent edges hide any boundary), and
culling is **conservative** (map the view rect to grid indices by the per-axis stride,
inflated to cover the half-stagger and tile overhang; over-included cells clip). The
rest of the stack — bundle/cache/animation/parallax/stats — is orientation-agnostic.
Verified end-to-end on `hexagonal-mini.tmx` (400 tiles) and `test_hexagonal_tile…`.

A loader gap surfaced alongside it: **columns/tilecount are now derived from the
image** when a uniform tileset omits them (older/hand-authored TMX), matching Tiled —
otherwise the tileset builds no tiles.

**Staggered — done.** `StaggeredRenderer` *inherits* `HexagonalRenderer::
tileToScreenCoords`, i.e. staggered **is** the hex formula with `hex_side_length` 0
(which staggered maps have). So `cell_to_world` and the culling just route `staggered`
through the same hex path — no new math. Verified end-to-end on
`isometric_staggered_grass_and_water.tmx` (650 tiles).

**Isometric — done.** The diamond projection (`IsometricRenderer`): `cell_to_world`
maps `(cx, cy)` to `((cx−cy)·tw/2 + originX, (cx+cy)·th/2)` (the diamond top corner,
with Tiled's `originX = map_height·tw/2`) and offsets to the tile bbox bottom-left
`(x − tw/2, y + th)`. Culling inverts the projection at the four view-rect corners and
takes the bounding cell box, inflated for tiles taller than the diamond. Verified on
`isometric_grass_and_water.tmx` (625 tiles).

All four Tiled orientations now render (orthogonal, hexagonal, staggered, isometric),
each hand-verified against the matching Tiled renderer and unit-tested on known
positions. The layout seam stayed exactly two functions.

---

## 9. Dynamic actors — `sprite_batch` + `actor_layer` (design)

Everything so far renders a **static** map: `world_renderer::draw` (world_renderer.cc:154)
is a closed loop over `world.layers()`, drawn back-to-front. Games also need a **dynamic**
layer — player, enemies, projectiles, falling pickups — whose positions come from the
physics/AI systems each frame, at continuous world coordinates, and which must interleave
at a chosen z-order among the static layers (walk *behind* a treetop layer but *in front*
of the floor). This section designs that.

### What is *not* the problem

Continuous positions are already handled. `to_screen(cam, plane, viewport, world_point)`
(camera.hh:87) maps any float world point to a screen pixel; the grid rounding is only the
static-tile seam-free path (`draw_gid_at`). An actor is just a sprite at a continuous world
point on some parallax plane — the engine can already draw it. The real work is **z-order
composition** and **ownership**, not coordinates.

### Ownership (locked)

`world` stays const, static, and shareable (the whole cache model rests on it). Actors are
**runtime state owned by the game** (physics bodies / ECS), rewritten every frame. So:

- An actor layer is **not** baked into `world` and is **not** a `world_layer` variant member.
- The map may *seed* actors — an object layer of spawn points (`type = "enemy"`, KE's
  `spawn_seq`) — but live instances live in the game.
- The actor layer depends only on `camera` + `draw_sprite` + `sprite_visual_ref` /
  `sprite_state_id`. It never touches `world`, the tileset, gids, or the resource cache.
  (That decoupling is a feature — map and actors meet only through spawn seeds.)

### `sprite_batch` — accumulate-then-flush, depth-sorted

A **pure draw sink**: a camera-aware sprite drawer you fill during a draw pass; `flush()`
**stable**-sorts by depth and draws back-to-front. It carries only what it needs to place a
queued sprite — `cam + viewport + plane` — and nothing about *which* sprites to draw
(culling) or *where between physics steps* we are (interpolation); those are the layer's
inputs, delivered via `layer_view` below. Reusable beyond actor layers (HUD, particles,
floating damage text). One `add`, no second method: a *stable* sort means equal depths keep
call order, so `depth`-agnostic drawing is just "give everything the same depth."

```cpp
class sprite_batch {
public:
    // cam + viewport + plane are all it needs to transform queued positions at flush.
    sprite_batch(const camera& cam, rect viewport, const world_layer_header& plane);

    // Queue a static visual at world `pos`, sorted by `depth` (usually pos.y for top-down).
    // Equal depths keep call order (stable sort) => "unsorted" == all the same depth.
    void add(world_point pos, float depth, sprite_visual_ref visual, sprite_draw_params params = {});
    // Same, for an animated runtime state (its current frame is resolved at flush time).
    void add(world_point pos, float depth, sprite_state_id state, sprite_draw_params params = {});

    // Stable-sort the queued sprites by depth ascending, draw back-to-front, then clear.
    // The compositor calls this at the layer boundary, so sorting is scoped to this slot.
    void flush();
};
```

At flush, each entry becomes a `draw_sprite(pos, visual, params)`: position via
`to_screen(cam, plane, viewport, pos)` plus the viewport top-left; `params.scale` is
multiplied by `cam.zoom` (the caller's scale is *on top of* zoom), exactly as the tile
anchor path does (world_renderer.cc:199-201). A content problem on one sprite is a no-op,
never a throw — same policy as `draw_stats`.

### `actor_layer` — the game-owned slot

`draw` takes the batch **and** a `layer_view`: the read-only per-frame inputs the layer needs
to decide *what* to draw. `visible` is the camera's visible world rectangle **on this layer's
plane** (parallax + zoom already applied), so the layer can skip off-screen actors; `alpha`
is the fixed-step interpolation factor. Both are computed by the compositor per slot — the
world rect is per-plane, so a parallaxed actor layer culls against *its* view, not the
camera's raw one. (`world_rect` = `sdlpp::rect<float>`, the world-space twin of `world_point`;
worth adding as an alias next to it in `world.hh`.)

```cpp
struct layer_view {
    world_rect visible;      // camera's visible region in world pixels, on this slot's plane
    float      alpha{1.0f};  // fixed-step interpolation factor, for lerp(prev, curr, alpha)
};

class actor_layer {             // a role of the shared render_layer base (see §10)
public:
    virtual ~actor_layer() = default;
    world_layer_header plane;   // parallax/offset/opacity/tint — like a tile layer
    virtual void draw(const layer_view& view, sprite_batch& batch) = 0;
};
```

> The base is generalized in §10 to `render_layer`, shared with live backgrounds: the
> interface is identical, and "actor" vs "background" is just what the subclass draws and
> where the compositor slots it. Treat `actor_layer` here as the sorted-sprite role of that
> one base.

Render-only: there is **no** `update()`. Physics/AI advance entities elsewhere at a fixed
step; `draw` reads *interpolated* positions (`lerp(prev, curr, view.alpha)`) so motion stays
smooth when the physics rate and the frame rate differ. A subclass reads like intent — cull
against `view.visible`, interpolate with `view.alpha`, queue into `batch`:

```cpp
void draw(const layer_view& view, sprite_batch& batch) override {
    for (const auto& e : m_entities) {
        const world_point p = e.render_pos(view.alpha);   // lerp(prev, curr, alpha)
        if (view.visible.intersects(e.aabb_at(p)))        // skip off-screen actors
            batch.add(p, p.y, e.visual, {.flip = e.flip});
    }
}
```

### The compositor seam

`world_renderer::draw` gains a boundary-callback overload — it fires after each map layer,
in order, and **knows nothing about actors**:

```cpp
draw_stats draw(const camera& cam, const rect& viewport,
                const std::function<void(world_layer_id done)>& after_layer);
```

A thin `world_compositor` owns the interleave: it holds the `world_renderer` plus
`{after: world_layer_id, actor_layer*}` insertions, wires the callback, and at each slot
builds a `sprite_batch` on that layer's plane, computes the slot's `layer_view` (the visible
world rect on that plane, from the camera + plane + viewport; plus the frame's `alpha`), calls
`actor_layer::draw(view, batch)`, and flushes. Computing `visible` reuses the same plane
transform culling already relies on (`eval_layer_origin`, camera.hh:60), so actor culling and
tile culling never drift.

```cpp
world_compositor comp{renderer};
comp.insert_after(ground_layer_id, enemies);
comp.insert_after(ground_layer_id, player);   // player drawn above enemies, below treetops
comp.draw(cam, viewport, alpha);              // static layers + actor slots, in map order
```

### Design decisions (agreed)

- **One `add`, stable sort, flush at the layer boundary.** No `submit`/`sprite_sorted`
  split: the depth argument *is* the sort/no-sort choice (ties keep call order). Flushing
  per slot scopes sorting to that slot.
- **`sprite_batch` is a pure draw sink; culling + interpolation are the layer's inputs.**
  The batch holds only `cam + viewport + plane` (what it takes to place a queued sprite). The
  visible world rect and `alpha` travel in `layer_view` to `draw`, because the layer — not the
  batch — decides *which* actors to draw and *where between physics steps* they are.
- **The cull rect is per-plane.** `layer_view::visible` is the camera's visible region mapped
  onto *this slot's* plane, so a parallaxed actor layer culls against its own view. The
  compositor computes it with the same `eval_layer_origin` transform the tile culling uses.
- **`sprite_batch` is reusable, not actor-specific.** HUD, particles, damage numbers fill
  the same batch. The actor layer is just one client.
- **Actor layer is decoupled from `world`/cache** — it draws `sprite_visual_ref` /
  `sprite_state_id` the game already resolved; no gid or tileset leaks into the API.
- **Interpolated positions; physics stays fixed-step and out of the layer.** `alpha` is
  optional: a game that just moves by `dt` per frame passes `alpha = 1` and ignores it.
- **The renderer stays actor-agnostic** — it only fires a layer-done callback; the
  `world_compositor` (not `world_renderer`) knows about `actor_layer`.

### Deferred (with rationale)

- **Cross-layer (tiles + actors) unified depth sort.** Current model sorts *within* a slot
  (actor strictly above "ground", strictly below "treetops" — Tiled's standard below/above
  split, and enough for KE and most top-down games). A true 2.5D merge — one actor behind
  one tile and in front of its neighbour — turns the whole compose step into a single
  y-sorted list of tiles *and* actors and is a much larger change. Do it only when a game
  needs it.
- **Dynamic / surface-backed backgrounds** are the read-only counterpart of this layer —
  designed in §10. The actor layer itself needs none of that: it draws already-registered
  visuals.

---

## 10. Dynamic backgrounds — image-source variant, animation, and live layers (design)

Backgrounds today are static file-loaded `world_image` layers. "Dynamic background" is
three separate needs, and they split on one line — **is the pixel content immutable after
creation?** — because that decides whether the content-keyed cache (Phase 7) applies. None
is a new subsystem; each lands on machinery we have or half-have, and the first step is a
model cleanup that pays for itself.

### The organizing split

- **Immutable content** (from disk, from memory, or from a computed-once surface) is
  *cacheable* — it just may not come from a file. This is a `world_image` **source**
  problem.
- **Mutable content** (recomputed per frame) is *not* cacheable — it must bypass the cache
  and stay a game-owned **layer**.

### Image source as a variant (replaces the `format` string)

`world_image` currently holds `format` + `source` + `data` and `decode_world_image` infers
intent from *which field is set* — a stringly-typed discriminator, the same class of bug as
a magic `format`. Replace it with a sum type of sources; each arm owns its decode and its
cache identity, and illegal states (raw bytes tagged as a file, etc.) become
unrepresentable:

```cpp
// Undecoded (decoded lazily at bind, in decode_world_image):
struct image_from_disk    { std::filesystem::path source; };          // load_image(path)
struct image_from_memory  { std::vector<std::uint8_t> bytes; };       // encoded blob: load_image(bytes)
// Decoded (arrives ready — a producer, not the TMX loader):
struct image_from_surface { std::shared_ptr<const sdlpp::surface> pixels;   // procedural / decoded-once
                            std::optional<content_key> identity; };   // producer-supplied id (below)

using world_image_source = std::variant<image_from_disk, image_from_memory, image_from_surface>;

struct world_image {
    world_image_source source;
    std::optional<sdlpp::color> transparent_color;   // shared presentation
    unsigned width{}, height{};                       // declared (disk/memory) or from the surface
};
```

The two field-sniffing functions become honest visits — no `format`, and no raw-vs-encoded
collision because the *type* is part of the identity:

```cpp
sdlpp::surface decode_world_image(const world_image& img) {
    return std::visit(overloaded{
        [](const image_from_disk&   s) { return load_image(s.source); },
        [](const image_from_memory& s) { return load_image(s.bytes);  },
        [](const image_from_surface& s){ return clone(*s.pixels);     },   // create_from_pixels / copy
    }, img.source);
}
```

**Two axes, not three peers.** The arms sit on two distinct questions:

- *Decoded yet?* — `{disk, memory}` are **undecoded** (they still need `load_image`);
  `surface` is **decoded**. This is the categorical line.
- *If undecoded, where do the bytes live?* — a file path (`disk`) or in RAM (`memory`).
  This is a minor sub-axis: it changes only *how* you reach the bytes and how you identify
  them, not *when* you decode.

**Decode stays lazy, at bind.** Both undecoded arms decode in `decode_world_image` at
bundle-build time (cache acquire / `world_renderer` construction) — not at parse. So `disk`
and `memory` are *both lazy*; they are not a lazy-vs-eager pair. `from_surface` is the only
*eager/decoded* form, and it is reserved for producers that genuinely arrive pre-decoded —
the KE importer, procedural generation. The **TMX loader does not manufacture a surface from
embedded `<data>` bytes**: it keeps them as `image_from_memory` and lets the video layer
decode lazily, so the `world` model stays compact pure data with no image-codec dependency.

**`from_sprites` was considered and dropped:** arranging sprite frames on a grid *is* a tile
layer; the only extra it offered was baking that layer to one texture for batching, which is
an optimization (the megatexture backlog item) whose output is a surface — so it collapses
into `image_from_surface`, not a distinct source.

`image_from_surface` is the whole immutable-*decoded* story: KE's assembled background, any
generated-once image, a decoded BOB block. It works for image layers *and* tilesets (KE
bricks) unchanged, because both already route images through `decode_world_image`.

**Identity, per arm.** `disk` takes the cheap mtime+size stat-cache path (resource_cache.cc:69);
`memory` hashes its encoded bytes — i.e. exactly the two identity branches that exist today,
made explicit. Raw pixels (`surface`) have no cheap identity: either hash the whole surface
(costly for a big image, every build) or let the **producer supply a `content_key`** ("these
pixels are `ke_bg_level_3`") — hence the optional `identity`, the one non-obvious cost of
raw-pixel content. `create_from_pixels` (surface.hh:720) is non-owning, so a wrapping decode
must copy or the backing buffer must outlive the pack (it does, within `build_bundle`'s
synchronous decode→pack).

**Considered — decode as a load-phase step (rejected for now).** If the `world` model held
decoded surfaces as its normal currency, `from_memory` would collapse into `from_surface`
(the loader decoding embedded bytes at parse) and the variant would shrink to
`variant<from_disk, from_surface>`. That is coherent — it moves decode off the render thread
(the async-load backlog wants that) and fails fast at load — but it heavies the pure-data
model (raw pixels are 4–10× the encoded size and pull `sdlpp::surface` into `world.hh`), does
work that a parse-only tool wastes, and couples the loader to the codec. Kept lazy for now;
if we flip, forward the encoded-byte hash as the surface `identity` so caching still dedups.

### Animated backgrounds — a layer concern, over any source

A looping backdrop (KE_FILL cycling its ~41 frames on a free-running counter) is a frame set
+ durations on the shared clock — which the engine already plays for tile cells (Phase 11).
The only gap: image layers resolve `visual(0)` but not `state()`. Give `world_image_layer` an
optional animation (frames are ordinary `world_image`s, any source), have `image_as_tileset`
build an animated single-tile bundle, and let `draw_layer(image_layer)` resolve `state(0)`
first — the same two-liner `draw_gid_at` uses (world_renderer.cc:182):

```cpp
sprite_visual_ref v = h.state(0).valid() ? sprite_state_appearance(h.state(0)).visual : h.visual(0);
```

Orthogonal to the source variant: an animated background of in-memory frames is just N
`image_from_surface`s.

### Live / procedural-per-frame — a game-drawn layer, same as actors

A minimap, plasma/fog recomputed each frame, a live starfield: content changes every frame,
so the content cache *cannot* apply. It is not a `world_image` source — it is a **game-drawn
layer with a virtual `draw`, the exact interface the actor layer already has** (§9). There
is no separate `background_layer` type: a live background is just a `render_layer` slotted
*below* the tile layers instead of between them. `render_layer` is the shared base (§9's
`actor_layer` generalized); "actor" and "background" are roles, not classes.

```cpp
class render_layer {            // the game-drawn slot base — actors AND live backgrounds
public:
    virtual ~render_layer() = default;
    world_layer_header plane;                                    // parallax/offset/opacity/tint
    virtual void draw(const layer_view& view, sprite_batch& batch) = 0;
};
```

The only difference is *what the subclass draws*, not the interface:

- **Actor role** — fills `batch` with depth-sorted sprites (`batch.add(...)`).
- **Background role** — ignores `batch` and blits its own `texture_access::streaming` texture
  (renderer.hh:86), which the game owns and updates each frame: fill the viewport, sample the
  region `layer_view::visible` gives for scroll/parallax (`texture_address_mode::wrap`,
  renderer.hh:97, for a tiled fill).

Both are `render_layer`s at compositor slots; the compositor builds the batch and flushes it
after each `draw`, so a background that never calls `add` simply flushes nothing. A live
surface must **not** be smuggled in as `image_from_surface`: the cache would key it once and
never see the updates — that immutable/mutable line is what separates a *cached source* from
a *game-drawn layer*.

*(Deferred: letting `sprite_batch` also carry a raw-texture entry, so a live texture can be
depth-interleaved with actor sprites in one sorted flush. Usually a background is its own slot
and needs no sorting; add it only when a game wants a live texture mid-stack.)*

### Shape / build order

| Need | Content | Mechanism | Builds on |
| --- | --- | --- | --- |
| source cleanup | — | `world_image` → `variant<disk, memory, surface>` | Phase 6/7 |
| from-surface (KE, procedural-once) | immutable | `image_from_surface` | source cleanup |
| animated backdrop | immutable | animated image layer | Phase 11 + a source |
| live / procedural | **mutable** | game-drawn `render_layer` (streaming texture) + compositor slot | §9 slot seam |

**Source cleanup first** — it is a refactor that also fixes the existing field-sniffing, and
`from_surface` (the KE unblock) falls out of it. Then animation and live are independent of
each other on top. Blast radius of the refactor is mechanical but wide: the TMX loader
(`image.cc` builds `image_from_disk`/`from_memory`), the cache (`image_identity` visits),
the bundle (`decode_world_image` visits), image layers, and `map_viewer`'s
`absolutize_image_paths` (touches `image_from_disk::source`).

### Design decisions (agreed)

- **Image *source* is a variant; the image *layer* is not.** Drawing an image on a plane is
  identical regardless of where the pixels came from — only decode + identity differ, which
  is exactly what the source variant localizes. No `world_image_layer` subtypes.
- **Three source arms on two axes — `disk`, `memory`, `surface`.** Decoded-vs-not is the
  categorical line (`surface` is decoded; `disk`/`memory` are not); path-vs-bytes is the
  minor sub-axis under undecoded. Both undecoded arms **decode lazily at bind**, so they are
  not a lazy/eager pair — `surface` is the only eager form. The **TMX loader keeps embedded
  bytes as `memory`** (no parse-time decode), so the `world` model stays compact pure data.
  `from_sprites` collapses into a tile layer (or, when flattened for batching, into
  `from_surface`).
- **`from_surface` carries a producer-supplied identity** (optional), falling back to a
  pixel hash — the one non-obvious cost of raw-pixel content.
- **Immutable → `world_image` + cache; mutable → a game-drawn `render_layer`.** Live content
  is never a source arm; it is the *same* virtual-`draw` layer as actors (§9), slotted below
  the tiles. One interface (`render_layer`), two roles (actor / background) — no separate
  `background_layer` type. The split is enforced, not incidental.

---

## 11. Implementation plan — dynamic actors & backgrounds (§9 + §10)

Seven phases continuing the Phase-1–13 numbering. **Phase 14 is a header-split refactor** the
rest builds on; then two independent tracks that meet at the end: **Track B (backgrounds)** =
Phases 15–16; **Track A (actor/compositor infra)** = Phases 17–19; **Phase 20** (live
background) needs both. Phases 15 and 17 can start in parallel once 14 lands. Each phase ships
with its tests green and the full suite passing before the next.

Design lives in §9/§10; this section is only the build sequence, deliverables, and test
obligations.

### Phase 14 — split `world.hh` into cohesive headers — prerequisite ✅ DONE

`world.hh` has grown to ~500 lines spanning the entire world vocabulary, and every later phase
edits it. Split it **first** — a pure mechanical move, no type or behavior change — so those
edits land in focused files and the header stops being a pile. Dependency order of the split:
common → layers → tileset → world.

**Done:** `world_common.hh` / `world_layers.hh` / `world_tileset.hh` created; `world.hh` reduced
to the `world` class + the three includes (umbrella preserved → zero downstream `#include`
changes). Each header compiles standalone; suite 568/568 unchanged. Also marked the two
behavior-bearing data structs (`world_tile_layer`, `world_tileset` — they have out-of-line
methods in `world.cc`) `NEUTRINO_EXPORT`, so a future global hidden-visibility build still links
(no-op today; `-fvisibility=hidden` is currently scoped only to `decompressor.cc`).

- **Depends on:** nothing. Do it before everything else.
- **Deliverables (new headers under `include/neutrino/world/`):**
  - `world_common.hh` — the shared vocabulary: the enums (`world_orientation`, render order,
    stagger axis/index, object draw order, text align), the id aliases (`world_tile_id` …
    `world_point`), the property system (`world_typed_string`, `world_object_reference`,
    `world_property`, `world_property_map`, `world_component`), and `world_image`.
  - `world_layers.hh` — `world_layer_header`, `world_tile_cell`, `world_tile_chunk`, the object
    model (`world_object_base`, the shape structs, the `world_object` variant), and the layer
    types (`world_tile_layer`, `world_image_layer`, `world_object_layer`, the `world_layer`
    variant). Includes `world_common.hh`.
  - `world_tileset.hh` — `world_tile_animation_frame`, `world_tile`, `world_tileset_grid`,
    `tile_drawable`, `world_tileset`. Includes `world_layers.hh` (a `world_tile` owns an optional
    `world_object_layer`).
  - `world.hh` — keeps only the `world` class and includes the three above, staying the umbrella
    so **no downstream `#include` changes**. Split `world.cc` to match (or leave it one .cc) —
    cutter's choice.
- **Tests:** none new — the suite must stay byte-for-byte green (only declarations moved). Fix
  any TU that relied on a lost transitive include by adding the explicit one.
- **Done when:** `world.hh` is just the `world` class + three includes, and each split header
  compiles standalone.

### Phase 15 — `world_image` source variant (model refactor + Flavor 1) — Track B ✅ DONE

Replace the `format`/`source`/`data` field-sniffing with `variant<image_from_disk,
image_from_memory, image_from_surface>` (§10). Behavior-preserving for disk/memory; adds the
decoded `surface` arm (Flavor 1) end to end.

**Done:** `world_common.hh` holds `world_image_source` + `world_image::empty()`; `decode_encoded_image`
and `image_identity` visit the variant; the `surface` arm is *borrowed* in `surface_for` (no
copy); the cache key folds `source.index()` (arm discriminator). Loader builds
`image_from_disk`/`image_from_memory` (dropped the dead `format`); `map_viewer` rewrites only the
disk arm; all tests migrated. `world_rect` deferred to Phase 17 (unused until then). A
no-identity `image_from_surface` is keyed by a content hash of the **canonical RGBA8888** the
packer uploads (`hash_surface_identity` converts then hashes, locking directly per the risks
note), *not* the surface address or raw source bytes — an address could be reused after the
surface is freed while a bundle lingers cold (stale pixels), and raw bytes under a different
format/palette render differently yet would collide. Suite 570/570 (+2), later +2 (no-identity
surfaces dedup by content; identical bytes in different formats key distinctly). map_viewer
verified (island, 2528 tiles).

- **Depends on:** 14 (edits `world_common.hh`).
- **Deliverables:**
  - `world_common.hh` — `world_image` holds `world_image_source`; add `using world_rect =
    sdlpp::rect<float>;` next to `world_point` while here (Track A needs it too).
  - `tileset_bundle.cc` — `decode_world_image` becomes a `std::visit`; `surface` arm wraps via
    `create_from_pixels` then **copies** (non-owning lifetime, surface.hh:720).
  - `resource_cache.cc` — `image_identity` becomes a `std::visit` (disk = stat-cache,
    memory = byte hash, surface = producer `content_key` or pixel hash); the key fold gains the
    arm discriminator so raw-vs-encoded byte-identical buffers cannot collide.
  - `tmx/image.cc` — `parse_image` builds `image_from_disk` (path) or `image_from_memory`
    (embedded `<data>`); **no** parse-time decode. Drop the now-dead `format` field.
  - `world_renderer.cc` `image_as_tileset`, `map_viewer.cc` `absolutize_image_paths` (touches
    `image_from_disk::source`), and any other `world_image` constructors updated.
- **Tests:** whole suite stays green (mechanical refactor). Add: a bundle built from an
  `image_from_surface` (raw pixels) resolves a valid visual; a `surface` and a `memory` image
  with byte-identical buffers get **distinct** cache keys (arm discriminator).
- **Done when:** no code constructs a `world_image` by setting fields; every consumer visits.

### Phase 16 — Animated image layers (Flavor 2) — Track B ✅ DONE

Let an image layer resolve an animation, reusing the Phase-11 shared-clock machinery.

**Done:** `world_image_layer` gains `std::vector<world_image_frame> frames`;
`image_layer_as_tileset` builds one tile per frame with tile 0 carrying the animation (so the
bundle exposes `state(0)`); `draw_layer(image_layer)` resolves `state(0)` first, else
`visual(0)`. The anchor uses the **current frame's** height (`current_image_visual` matches the
resolved visual back to its frame), so frames of different heights keep a common top-left rather
than jumping as the animation advances. Added `world_renderer::image_layer_handle` and
`image_layer_current_height` accessors. Suite 571/571 (+1: an animated image layer advances its
shared state 0→1, its anchor height tracks the frame 32→24, and it still draws).

- **Depends on:** 15 (frames are `world_image`s of any arm).
- **Deliverables:** `world_image_layer` gains optional animation frames + durations;
  `image_as_tileset` builds an animated single-tile bundle; `draw_layer(image_layer)` resolves
  `state(0)` first, falling back to `visual(0)` (world_renderer.cc:182 pattern).
- **Tests:** an animated image layer advances frames off the shared clock; a static image
  layer is unaffected.

### Phase 17 — `sprite_batch` + `world_rect` — Track A ✅ DONE

The pure draw sink (§9).

**Done:** `world_rect = sdlpp::rect<float>` in `world_common.hh`. New
`video/world/sprite_batch.{hh,cc}`: `sprite_batch{cam, viewport, plane}` with two `add`
overloads (`sprite_visual_ref` / `sprite_state_id`), `plan()` (stable-sort by depth → screen-space
`sprite_draw` list, position `to_screen` + viewport top-left, `scale × zoom`) and `flush()`
(plan → draw valid → clear). `flush` **skips invalid content** — an invalid `sprite_state_id`
trips `draw_sprite`'s registry enforcement, so validity is checked in the batch, not delegated.
Suite 574/574 (+3: stable depth sort with ties, transform, flush no-throw).

- **Depends on:** 14; `world_rect` (landed in 15; if Track A starts first, add the alias here).
- **Deliverables:** `sprite_batch{cam, viewport, plane}` with `add(pos, depth, visual|state,
  params)` and `flush()` — **stable** sort by depth ascending, draw back-to-front, position via
  `to_screen(cam, plane, viewport, pos)` + viewport top-left, `params.scale * cam.zoom`; a
  per-sprite content error is a no-op (draw_stats policy).
- **Tests:** depth ordering (back-to-front); **stable** ties keep call order (the property that
  lets one `add` cover unsorted); transform matches the tile anchor path; a bad visual doesn't
  throw.

### Phase 18 — `render_layer` + `layer_view` + renderer boundary seam — Track A ✅ DONE

The game-drawn layer interface and the hook the compositor drives.

**Done:** new `video/world/render_layer.hh` — `render_layer` base (`plane` + `virtual draw(const
layer_view&, sprite_batch&)`) and `layer_view{visible, alpha, viewport}`. `world_renderer::draw`
gains a `const std::function<void(world_layer_id)>& after_layer` overload (the no-arg form
delegates to it with an empty callback); it fires after each layer in map order via a
`world_layer_header`-visiting lambda. Suite 576/576 (+2: callback fires once per layer in order
with the right ids; a `render_layer` fills a batch through its interface).

- **Depends on:** 17.
- **Deliverables:** `render_layer` base (`plane` + `virtual void draw(const layer_view&,
  sprite_batch&)`); `layer_view{world_rect visible; float alpha;}`; a `world_renderer::draw`
  overload taking `const std::function<void(world_layer_id done)>& after_layer` fired after
  each map layer in order — the renderer stays actor-agnostic.
- **Tests:** the callback fires once per layer, in map order, with the right ids; a
  `render_layer` fed a batch draws its sprites.

### Phase 19 — `world_compositor` — Track A ✅ DONE

Interleave static layers and `render_layer`s by z-slot (§9).

**Done:** new `video/world/world_compositor.{hh,cc}` — `insert_after(world_layer_id, render_layer&)`
and `insert_bottom(render_layer&)` (backgrounds, before all layers). `draw(cam, viewport, alpha)`
wires the `after_layer` callback and, per slot, builds a `sprite_batch` on the plane, computes
`layer_view` (`visible` via the same `eval_layer_origin` as tile culling, plus viewport) and
flushes. Exposed `world_renderer::parallax_rest()` so the batches sit on the map's parallax
planes. Suite 579/579 (+3: slot placement/alpha, per-plane visible rect vs `eval_layer_origin`,
parallax planes cull differently / no-match never draws).

- **Depends on:** 18.
- **Deliverables:** `world_compositor{renderer}`; `insert_after(world_layer_id, render_layer&)`;
  `draw(cam, viewport, alpha)` wires the callback, and at each slot builds a `sprite_batch` on
  the slot's plane, computes `layer_view::visible` via the **same** `eval_layer_origin`
  transform tile culling uses (camera.hh:60 — no drift), calls `draw`, and flushes.
- **Tests:** an actor layer slotted between two tile layers draws above one and below the
  other; two layers at the same slot preserve insertion order; the per-plane `visible` rect
  matches the tile cull for parallax 1 and shifts correctly for a parallaxed plane.

### Phase 20 — Live / procedural background (Flavor 3) — needs A + B ✅ DONE

A `render_layer` that blits a game-owned streaming texture (§10).

**Done:** `layer_view` gained `viewport`; `world_compositor::insert_bottom` draws beneath the map.
New `video/world/texture_layer.hh` — `texture_layer : render_layer` blits a game-owned
`const sdlpp::texture*` (streaming; the game updates it) to fill the viewport via
`get_renderer().copy` (optional `source` sub-rect), ignoring the batch; a null texture is a
no-op. No separate `draw_texture` primitive was needed — `get_renderer().copy` is the blit.
Suite 581/581 (+2: a bottom slot draws before the after-slots and `layer_view` carries the
viewport; a `texture_layer` blits a real streaming texture with `failed==0`, null is a no-op).

- **Depends on:** 19 (slot/compositor) and a texture-blit primitive.
- **Deliverables:** if absent, a `draw_texture(dst, texture, src, ...)` primitive analogous to
  `draw_sprite` (the sprite path already blits through the renderer; expose the raw-texture
  case); an example `render_layer` driving a `texture_access::streaming` texture, filling the
  viewport and sampling the region from `layer_view::visible` (`texture_address_mode::wrap` for
  a tiled fill), slotted *below* the tile layers.
- **Tests:** a live background draws beneath the tiles; the sampled region tracks the camera
  under pan/zoom; updating the texture between frames changes the output (proving it bypasses
  the content cache).

### Build order

          ┌ 15 → 16                       (Track B)
    14 ───┤
          └ 17 → 18 → 19 ┐                (Track A)
                         └ 20

14 first (header split). Then 15 (Track B) ∥ 17 (Track A) → 16 on B, 18→19 on A → 20 once 19
and the blit primitive land.

### Plan-specific watch-items

- **Phase 14 is declarations-only.** Move code between headers; change no types or behavior, so
  the suite stays byte-for-byte green. The `world_image` variant is Phase 15, not here.
- **Phase 15 blast radius is wide but mechanical.** Land it behavior-preserving; the suite is
  the guardrail. Do not mix the refactor with new behavior beyond the `surface` arm.
- **`create_from_pixels` is non-owning** — the `surface` decode must copy (or guarantee the
  buffer outlives the pack); `build_bundle`'s decode→pack is synchronous, so a copy is the safe
  default.
- **`sprite_batch` sort must be stable** — the whole one-`add` design rests on ties keeping
  call order.
- **Compositor `visible` must reuse `eval_layer_origin`** — never recompute the plane
  transform, or actor and tile culling drift.
- **Live textures never enter `world_image`** — a mutable surface smuggled in as
  `image_from_surface` is keyed once and never updates. Enforce the immutable/mutable line.

---

## 6. Risks / watch-items

- **Silent wrong-texture on hash collision.** Mitigated by the length
  discriminator (Phase 1) and the stat-key primary path. Keep both.
- **sdlpp `surface::lock_guard` bug.** It misreads SDL3's `bool` return from
  `SDL_LockSurface` (treats success as failure). `cpu_texture_atlas.cc` already
  uses a correct local RAII; do **not** "reuse" `lock_guard` until fixed upstream.
- **Gutter/extrude omission** re-introduces tile bleed under scale/filtering —
  build it in Phase 3, don't defer.
- **Acquire-before-release ordering** is the one contract a level manager can get
  backwards; the Phase 8 switch helper must own the ordering, not call sites.
- **Render-context tests** need `test::test_application` (dummy video driver);
  pure-data phases (1, 3-pack, 5, 9) test without it.

---

## 7. Suggested build order

1 → 2 → 3 and 4 and 5 in parallel → 6 → 7 → 8 → 9 → 10 → 11 → 12 → 13.

Each phase ships with its tests green and the full suite passing before the next
begins.
