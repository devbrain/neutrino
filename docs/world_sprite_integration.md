# World ↔ Sprite Integration — Implementation Plan

Status: **design agreed, not yet implemented**
Owner: igor
Last updated: 2026-07-06

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

## Phase 1 — Content hash utility

**Goal.** A fast, stable 64-bit content hash plus a length discriminator, built
from two seeded `SDL_murmur3_32`.

**Deliverables.** (byte-hash primitive already landed as `details::hash_bytes`.)
- `details::hash_bytes(const void*, size_t)` / `hash_bytes(std::span<T>)` —
  composes two seeded `SDL_murmur3_32` into a fixed 64-bit digest, identical on
  every platform. **Done.**
- `struct content_key { std::uint64_t hash; std::uint64_t length; auto operator<=>() = default; };`
  with `content_key content_hash(std::span<const std::byte>) = { hash_bytes(bytes), bytes.size() }`.
- A `std::hash<content_key>` specialization for hash-table bucketing.

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

**Done when.** Tests pass; util is usable from any TU.

---

## Phase 2 — Image decode → `sdlpp::surface`

**Goal.** The missing bridge: turn a file path or byte span into an
`sdlpp::surface`, using onyx_image. Generic, no world dependency.

**Deliverables.**
- New `src/neutrino/video/image_loader.{hh,cc}` (public header under
  `include/neutrino/video/`):
  - `sdlpp::expected<sdlpp::surface, std::string> decode_surface(std::span<const std::byte>);`
  - `sdlpp::expected<sdlpp::surface, std::string> load_surface(const std::filesystem::path&);`
- onyx_image decodes to its own surface abstraction → convert to `sdlpp::surface`.
- CMake: add source; link onyx_image to the neutrino target if not already.

**Design notes.**
- `load_surface` reads bytes then calls `decode_surface` — one read, reusable by
  the cache (which also wants the bytes to hash).
- Return `expected`, not throw, so callers choose graceful degradation.
- Format auto-detected by onyx_image; no format hint needed.

**Tests** (`test/video/test_image_loader.cc`):
- Fixture: add a tiny known PNG under `test/video/data/`, **or** synthesize
  bytes at test time (encode a procedural surface via onyx_image / stb_image_write).
- Decode → surface has expected width/height/format and a known pixel value.
- Malformed bytes → `expected` error, no crash.
- `load_surface` on a missing path → error.

**Done when.** A known image round-trips to a surface with correct dimensions
and a sampled pixel.

---

## Phase 3 — Rectangle packer + surface compositing

**Goal.** Given N sized inputs, assign non-overlapping positions with gutter and
multi-page spill (stb_rect_pack), and composite N surfaces into one
`cpu_texture_atlas`.

**Deliverables.**
- New `src/neutrino/video/atlas_packer.{hh,cc}`:
  - Low level: `pack(std::span<const dim>, int max_page_w, int max_page_h, int gutter) -> pack_result`
    where `pack_result` has, per input, `{page index, rect}`; may span pages.
  - High level: `pack_surfaces(std::span<sdlpp::surface>, options) -> std::vector<cpu_texture_atlas>`
    (one atlas per page), compositing via `blit_to` with gutter + edge-extrude.
- Depends on Phase 2 (surfaces) and `stb_rect_pack.h`.

**Design notes.**
- Sort inputs by height-then-width before packing (better fill, stable output).
- Gutter: inflate each rect by N px before packing; extrude border pixels into
  the gutter after blitting (prevents bleed under scale/linear filter).
- Cap page size at a safe max (query renderer max texture size; default 2048/4096).
- Multi-page: an input resolves to `(page, rect)`; keep the page even though most
  tilesets fit one page.

**Tests** (`test/video/test_atlas_packer.cc`):
- Pack rects → assert no two placed rects overlap (accounting for gutter).
- All inputs placed exactly once across pages; oversized-set spills to page 2.
- Composite two solid-color surfaces → sampled pixels land in the right rects;
  gutter pixels are the extruded edge color, not a neighbor's.
- Deterministic output for the same input order.

**Done when.** Packing is overlap-free and complete; compositing places pixels
correctly with a bleed-safe gutter.

---

## Phase 4 — Draw-time rotation (transform superset)

**Goal.** Add `rotation` to the sprite draw transform and wire it through both
draw paths; this serves hex-120 tiles and object rotation.

**Deliverables.**
- `sprite_draw_params { float scale{1}; sprite_flip flip{none}; float rotation_degrees{0}; };`
- Wire rotation into:
  - the `copy_ex` (non-diagonal) path — SDL already takes an angle;
  - the `render_geometry` (diagonal) path — fold rotation into the vertex transform.
- Rotation pivots around the visual origin/anchor (same anchor scale/flip use).

**Design notes.**
- Keep the anchor semantics consistent: scale, flip, and rotation all resolve
  around the visual origin so a rotated tile stays pinned to its cell anchor.
- hex-120 is just `rotation_degrees = 120` composed with the cell's flip.

**Tests** (extend `test/video/test_draw.cc`, needs app/render context):
- `draw_sprite(..., {.rotation_degrees = 90})` returns success for both a
  non-diagonal and a diagonal-flipped sprite.
- 0° is byte-identical to no-rotation (regression guard).
- A rotation + flip combination returns success (composition doesn't assert).
- If the harness supports geometry readback, assert a corner lands where a 90°
  rotation about the origin predicts.

**Done when.** Rotation renders through both paths; existing draw tests still pass.

---

## Phase 5 — Neutral tile-resolution interface on `world`

**Goal.** Expose the format-neutral, render-neutral resolution the bridge
depends on. Pure data — no sprite/texture/GPU types.

**Deliverables.**
- On `world_tileset` / `world` (in `include/neutrino/world/world.hh`), a
  descriptor + methods (illustrative):
  - `struct tile_drawable { const world_image* image; rect src; point origin; bool animated; };`
  - `world_tileset::drawable(world_local_tile_id) const -> tile_drawable`
    — uniform: `image = &this->image, src = tile_rect(id)`; collection:
    `image = &tile(id)->image, src = {0,0,w,h}`.
  - `world_tileset::animation_of(world_local_tile_id) const -> const std::vector<world_tile_animation_frame>*`
    (nullptr if not animated).
  - Cell resolution already exists: `world::tileset_for(gid)` + `to_local(gid)` +
    `world_tile_cell{flip, rotated_hex_120}`.

**Design notes.**
- The descriptor references a `world_image` (path/bytes) — the *bridge* turns
  that into a texture. World never touches the GPU.
- This is where collection-of-images stops being a special case: `drawable`
  hides whether the source is a shared atlas image or a per-tile image.
- No behavioral change to existing methods; this is additive.

**Tests** (extend `test/world/test_tmx_loader.cc`, no render context):
- Uniform tileset fixture: `drawable(N).src == tile_rect(N)`, origin correct.
- Collection tileset fixture: `drawable(N).image` is the per-tile image, `src`
  covers the whole tile image.
- `animation_of` returns the frame list for an animated tile, nullptr otherwise.
- `tileset_for(gid)` / `to_local` round-trips across multiple tilesets.

**Done when.** Both tileset kinds resolve through one descriptor; existing world
tests still pass.

---

## Phase 6 — Tileset → render-resource bundle builder

**Goal.** Given one `world_tileset` (+ loader + packer), produce a registered
bundle: atlas(es), sheet, and one animation+state per animated tile. No cache
yet.

**Deliverables.**
- New `src/neutrino/video/world/tileset_bundle.{hh,cc}`:
  - `struct tileset_bundle { std::vector<gpu_texture_atlas_id> atlases; sprite_sheet_id sheet; std::unordered_map<world_local_tile_id, sprite_animation_id> animations; std::unordered_map<world_local_tile_id, sprite_state_id> states; };`
  - `tileset_bundle build_bundle(const world_tileset&);`
  - Teardown helper that unregisters in order: states → animations → sheet →
    atlases.
- Uniform tileset: `load_surface(image.source)` → `register_atlas` → sheet whose
  visual *i* = `drawable(i)`.
- Collection tileset: decode per-tile images → `pack_surfaces` → atlas(es) →
  sheet.
- Animated tiles: build `sprite_animation` from `animation_of`, one shared
  `sprite_state` each.

**Design notes.**
- Visual index == local id for uniform tilesets (identity), so cell resolution
  stays a direct index.
- Origin from the tileset `offset`/tile origin flows into `sprite_visual::origin`.
- Reuse Phase 2/3/5; do not duplicate loading or packing here.

**Tests** (`test/video/world/test_tileset_bundle.cc`, app context):
- Uniform fixture → sheet visual count == tile count; a resolved visual draws.
- Collection fixture → packed atlas(es); each tile resolves to a visual with the
  right size.
- Animated fixture → a state per animated local id; `sprite_state_appearance` of
  a state resolves to a frame visual.
- Teardown unregisters cleanly (no `uses()` violation, no leak).

**Done when.** A tileset of each kind builds a drawable bundle and tears down
cleanly.

---

## Phase 7 — Content-keyed resource cache

**Goal.** The heart of "load/unload only the delta": a cache mapping tileset
identity → bundle, with refcounts and an LRU cold pool.

**Deliverables.**
- New `src/neutrino/video/world/resource_cache.{hh,cc}`:
  - `content_key key_for(const world_tileset&)` — file-backed: stat
    `(path,mtime,size)`, look up cached hash, recompute (Phase 1) on change;
    collection/embedded: hash of inputs + pack params.
  - `bundle_handle acquire(const world_tileset&)` — hit: `++refcount`, resurrect
    from cold pool if needed; miss: `build_bundle` (Phase 6), insert at 1.
  - `void release(bundle_handle)` — `--refcount`; at 0 → move to cold pool.
  - Cold pool: budgeted LRU; on overflow, evict (tear down bundle).
- Stat-key → content-key map cached, invalidated on mtime/size change.

**Design notes.**
- **Acquire-before-release** is the caller's contract (Phase 8 enforces it).
- Cold bundles stay registered and keep ticking (animated states cost ~nothing),
  so re-acquire is instant and in-phase.
- Refcount is *intent*; the manager's `uses()` guard remains the *safety net*.
- Budget is a knob (bytes or bundle count); start with a count.

**Tests** (`test/video/world/test_resource_cache.cc`, app context):
- Acquire same tileset twice → one build, refcount 2 (assert via a build counter
  or by observing a single sheet id).
- Release once → still resident; release twice → in cold pool (still resolvable),
  not yet unregistered.
- Exceed cold-pool budget → LRU bundle unregistered.
- Two tilesets referencing the same image (same content key) → shared bundle.
- Change mtime (touch fixture) → new content key → new bundle.
- **Switch delta:** acquire {A,B}, then acquire {B,C} before releasing {A,B};
  assert B never rebuilt, A cold/evicted, C built once.

**Done when.** The switch-delta test proves shared bundles survive and only the
difference loads/unloads.

---

## Phase 8 — `world_renderer` binding (lifecycle only)

**Goal.** A per-level RAII object that acquires bundles for every tileset in a
world and releases them on destruction. No drawing yet.

**Deliverables.**
- New `src/neutrino/video/world/world_renderer.{hh,cc}`:
  - `world_renderer(const world&, resource_cache&)` — acquires a bundle per
    tileset; holds a `tileset index → bundle_handle` table.
  - Destructor releases all handles.
  - Non-copyable, movable.
- A switch helper that constructs the new renderer before dropping the old
  (encodes acquire-before-release so call sites can't get it backwards).

**Design notes.**
- The renderer owns only its *view* (handles + draw scratch), never the bundles
  themselves — those live in the cache.
- Move semantics must not double-release.

**Tests** (`test/video/world/test_world_renderer.cc`, app context):
- Construct over a world → all tilesets acquired (cache refcounts bumped).
- Destroy → all released.
- Two renderers over worlds sharing a tileset; destroy the first → shared bundle
  stays resident (refcount held by the second). This is the level-switch guarantee.
- Move a renderer → no double-release on destruction.

**Done when.** Renderer lifetime drives cache refcounts correctly, including the
shared-tileset switch case.

---

## Phase 9 — Camera + culling (finite orthogonal)

**Goal.** Minimal camera and the visible-cell-range computation for finite
orthogonal layers. Pure math, no drawing.

**Deliverables.**
- `struct camera { point offset; float zoom{1}; };` (+ world-pixel → screen).
- `visible_cell_range(const world&, const world_tile_layer&, const camera&, screen_size) -> {x0,y0,x1,y1}`.

**Design notes.**
- Zoom multiplies into `sprite_draw_params.scale`.
- Clamp the range to layer bounds; account for tile size, layer offset, parallax.

**Tests** (`test/video/world/test_world_camera.cc`, no render context):
- A camera over a large layer yields a cell range matching the viewport rect.
- Offset/zoom shift and shrink/grow the range as predicted.
- Ranges clamp at layer edges (no negative / out-of-bounds indices).
- Parallax/offset shift the range correctly.

**Done when.** Culling returns exactly the on-screen cell rectangle for varied
camera states.

---

## Phase 10 — Tile-layer draw path (static tiles)

**Goal.** Render finite orthogonal tile layers of static tiles.

**Deliverables.**
- `world_renderer::draw(const camera&, screen_size)`:
  - For each visible tile layer, cull (Phase 9), iterate cells; per cell:
    gid → `tileset_for` → `to_local` → bundle sheet → `visual_ref` → `draw_sprite`
    at the computed screen position with `{scale=zoom, flip=cell.flip,
    rotation = cell.rotated_hex_120 ? 120 : 0}`.
- Layer opacity/visible/tint honored (skip invisible; opacity later if needed).

**Design notes.**
- Empty cells (gid 0) skipped.
- Resolve services once per draw (the sprite API already supports this).
- Draw order per the world's render order.

**Tests** (`test/video/world/test_world_draw.cc`, app/offscreen context):
- Render a small finite world → all draws return success, no throw.
- A flipped cell and a hex-rotated cell draw without asserting.
- If pixel readback is available: a known tile lands at the predicted screen
  position under a given camera.
- An out-of-range/garbage gid is skipped or errors gracefully (no scene kill).

**Done when.** A finite orthogonal map renders its static tiles at correct
positions under camera transforms.

---

## Phase 11 — Animated tiles (shared-clock draw integration)

**Goal.** Draw animated tiles via the shared clock, resolved once per frame.

**Deliverables.**
- In the draw pass, before iterating cells: for each animated local id in the
  visible tilesets, resolve the bundle state's current appearance once into a
  `local_id → current visual` table.
- Cell resolution consults the table: static → identity visual, animated →
  current visual; flip composes per cell.
- States tick via the existing `sprites_manager::update` (nothing new to tick).

**Design notes.**
- One resolve per animated tile per frame, not per cell.
- Flip is orthogonal to the clock: the clock picks the frame, the cell picks the
  mirror — one state serves cells of differing flips.

**Tests** (extend `test/video/world/test_world_draw.cc`):
- Advance the app several frames → an animated tile's resolved visual changes at
  frame boundaries; matches `total_duration` wrap.
- Two cells of the same animated tile resolve to the identical current visual.
- A flipped animated cell composes flip with the current frame correctly.
- Phase survives a simulated level switch that keeps the tileset resident
  (no reset).

**Done when.** Animated tiles play in lockstep, cost O(distinct animated tiles),
and survive resident-tileset switches without a phase reset.

---

## Phase 12 — Tile objects + image layers

**Goal.** Render object-layer tile objects (gid) and image layers.

**Deliverables.**
- Tile objects: `world_object_base.gid` resolves exactly like a cell (+ object
  `rotation` into the same rotation field, + flip).
- Image layers: build a one-visual bundle for `world_image_layer.image`; draw as
  a single sprite at the layer position.

**Design notes.**
- Non-gid objects (shapes/text) are out of scope for sprite rendering (debug/UI
  layer handles them).
- Image-layer images can optionally go through the packer or stand alone.

**Tests** (extend world draw tests):
- A world with a tile object → draws at object position with rotation/flip.
- An image layer → draws once at the right position.

**Done when.** Tile objects and image layers render alongside tile layers.

---

## Phase 13 — Infinite / chunked maps

**Goal.** Support infinite maps by culling to intersecting chunks.

**Deliverables.**
- Chunk-aware culling: iterate only `world_tile_chunk`s intersecting the camera
  rect; within each, the Phase 10/11 cell path.

**Tests.**
- An infinite-map fixture → only chunks intersecting the camera are visited;
  cells render at correct world positions.

**Done when.** An infinite map renders only its visible chunks.

---

## 5. Backlog (explicitly deferred)

- **Async load / preload.** Decode on a worker; upload on the render thread.
  Preload the next level's *new* tilesets during a transition (the refcount model
  already supports acquiring early).
- **Parsed-`world` CPU cache** keyed by map path — avoid re-parsing/decompressing
  on revisit. Separate from the GPU resource cache.
- **Megatexture uniform tilesets** into shared pages for batching — only if
  draw-call count is measured to matter; doesn't change the resolution interface.
- **Texture-vs-sheet key split** — share a texture across different tile-size
  uses of the same image. Only if a real case needs it.
- **Layer opacity/tint** compositing beyond visible/skip.
- **LDtk loader** — populates the same `world` + neutral resolution interface.

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
