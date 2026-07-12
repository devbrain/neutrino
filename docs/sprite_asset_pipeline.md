# Sprite Asset Pipeline — `sprite_def` → `sprite_set` (design)

Make hand-authored / loaded sprites first-class, cacheable assets, on the same shape as
the world tile pipeline. Today assembling one animated character is ~40 lines of imperative
registration plus a strict reverse-dependency teardown (`examples/sprite_demo/sprite_demo.cc`
lines 106–162: `register_atlas → sprite_sheet → N× add_visual → register_sprite_sheet → 4×
(make+register animation) → 3× create_sprite_state`, then nine `unregister_*` in order), and
everyone reinvents a struct of ids. This replaces that with a pure value the cache owns.

## 1. Architecture — the world parallel, made shared

The design mirrors the world subsystem exactly, and **shares its machinery** rather than
copying it:

| World (existing) | Sprites (this doc) |
| --- | --- |
| `world_tileset` (pure data) | **`sprite_def`** (pure data) |
| `build_bundle` → `tileset_bundle` | **`build_sprite_set`** → **`sprite_set`** |
| `resource_cache` (content-key + refcount + cold LRU) | **`sprite_cache`** (same engine) |
| `bundle_handle` | **`sprite_set_handle`** |

Ownership lives in the cache (content-keyed, refcounted), never in a caller-held RAII value:
the builder just populates the `sprite_def`. Two things are genuinely shared, not duplicated:

- **`render_bundle`** — the RAII owner of `{atlases, sheets, animations, states}` with
  dependency-ordered teardown (`states → animations → sheets → atlases`), lifted out of
  `tileset_bundle`. `tileset_bundle` and `sprite_set` are **facades** over it.
- **The cache engine** — content-key + refcount + cold-LRU + image stat-cache, lifted out of
  `resource_cache` into a reusable core that both the tile cache and the sprite cache use.

### Facades over one owner

```
render_bundle { atlases[], sheets[], animations[], states[] }   // owns + tears down in order
  ├── tileset_bundle : + visuals[]/states[] by LOCAL TILE ID  (gid facade; shared states)
  └── sprite_set     : + name→sprite_visual_ref / name→sprite_animation_id  (name facade)
```

The one honest difference is **state ownership**, so they stay separate facades rather than
one type:

- **Tiles bake *shared* states** — every instance of an animated tile plays in lockstep off
  one `sprite_state` the bundle owns (`render_bundle.states` populated).
- **Sprites use *per-instance* states** — `sprite_set` owns **animations, not states**
  (`render_bundle.states` empty); each actor spawns and owns its own playhead from a clip.

## 2. Key models (locked)

### `sprite_def` — pure, loadable value

Two authoring modes, both carried as compact **data** (grid expanded at build, like
`world_tileset` keeps `tile_width/columns` and computes `tile_rect`), so defs and content
keys stay small and dedup naturally:

```cpp
enum class sprite_origin_rule { top_left, center, bottom_center /* … */ };

struct sprite_grid {                       // uniform sheet: auto-generates frames "0".."N-1"
    unsigned          cell_w, cell_h;
    unsigned          columns{}, count{};  // count 0 => derive from image (as tilesets do)
    unsigned          margin{}, spacing{};
    sprite_origin_rule origin{sprite_origin_rule::top_left};
};

struct sprite_visual_def {                 // an explicit / named frame (packed atlas)
    std::string          name;
    rect                 src;              // sub-rect in the atlas image
    point                origin{0, 0};     // per-visual pivot
    // Trim metadata (exporters trim transparent margins): the frame's ORIGINAL size and
    // the trimmed rect's offset within it, so trimmed frames still align. Unset = untrimmed.
    std::optional<dim>   source_size;
    std::optional<point> trim_offset;
};

struct sprite_frame_def {                  // one step of a clip; references a visual by name
    std::string               visual;      // a grid frame ("3") or an explicit name
    sprite_animation_duration duration;
    sprite_flip               flip{sprite_flip::none};
};

struct sprite_clip_def {                   // a named animation
    std::string                   name;    // "idle", "walk", "jump"
    std::vector<sprite_frame_def> frames;
    bool                          loop{true};
};

struct sprite_def {
    world_image                    image;   // reuse the disk/memory/surface source variant
    std::optional<sprite_grid>     grid;    // optional uniform slicing
    std::vector<sprite_visual_def> visuals; // explicit named frames (may override/alias grid)
    std::vector<sprite_clip_def>   clips;
};
```

- **Per-visual origin** (pivot) — explicit on atlas frames; for grid mode a single
  `origin_rule` applies to every cell (with per-cell override via a `visuals` entry).
- **Trim** — `source_size` + `trim_offset` on atlas frames, so any exported (TexturePacker /
  Aseprite) atlas aligns; unset means untrimmed. In v1.
- **Clip frames reference visuals by name**; grid frames are named by index (`"0"`, `"1"`, …)
  so a clip can list names or a range helper can expand `range(4, 3)` → `{"4","5","6"}`.

### `sprite_set` — the built facade (cache-owned)

```cpp
class sprite_set {                                    // reached through a sprite_set_handle
public:
    [[nodiscard]] std::optional<sprite_visual_ref>   visual(std::string_view name) const;
    [[nodiscard]] std::optional<sprite_animation_id> clip(std::string_view name) const;
private:
    render_bundle m_bundle;                          // owns atlas/sheet/animations
    /* name -> visual_ref, name -> animation_id */
};
```

The caller never sees an atlas or sheet id, and never a bare `sprite_set` — only a
`sprite_set_handle` from the cache. Spawning lives on the handle so an instance can hold a
lease (below), not on the bare set.

### Per-instance state — a leased `sprite_instance` (lifetime contract)

`sprite_set` owns the *animations*; an actor owns its *playhead* (`sprite_state`). The hazard:
if the cache evicts a `sprite_set` while a spawned state is still alive, the animation is
unregistered under a live state. The contract closes it: **a spawned instance holds a lease on
its set**, so the set cannot be evicted while any instance lives.

```cpp
class sprite_instance {                              // per-entity; created by the handle
public:
    [[nodiscard]] sprite_state_id state() const;     // draw via draw_sprite(pos, state())
    // switch/restart clip helpers route through here
    ~sprite_instance();                              // unregisters the state, then drops the lease
private:
    sprite_set_handle m_lease;                       // keeps the set resident (refcount)
    sprite_state_id   m_state;                       // this entity's playhead
};

sprite_instance sprite_set_handle::spawn(std::string_view clip) const; // retains a lease
```

- **Ownership order per instance:** the state is unregistered *before* the lease drops, so the
  animation it references is always still resident at teardown.
- **Cache eviction:** a `sprite_set` with outstanding leases is refcount > 0 and never cold, so
  it cannot be evicted under a live instance — the same refcount rule the tile cache already
  uses, extended to spawned states.
- **Implication for S2:** the shared cache handle must be a **refcounting lease** (copy retains,
  destroy releases), so `sprite_instance` holding a copy keeps the entry alive. (The world's
  `bundle_handle` is manual acquire/release today; the shared core makes the handle the lease.)

### Build semantics — names and trim (deterministic)

**Name resolution (order-defined, so the content key is stable):** grid frames are inserted
first, named `"0".."N-1"`; then explicit `visuals` are applied in declared order.

- An explicit visual whose name matches a **grid-generated** name **overrides** it (replaces
  its `src`/`origin`/trim). Allowed.
- Two explicit visuals with the **same** name → **build error** (duplicate explicit visual).
- Two clips with the **same** name → **build error**.

The content key folds `image` identity + `grid` params + `visuals[]` + `clips[]` in **declared
order** (sequences, not the resolved map), so it never depends on map iteration order.

**Trim/origin — one formula.** The author gives the pivot in the **untrimmed** frame; the baked
`sprite_visual.origin` is in **packed (trimmed) local** pixels, because `draw_sprite` places the
origin within the packed rect. With `trim_offset` = the trimmed rect's top-left inside the
untrimmed frame (default `(0,0)`) and `source_size` = the untrimmed size (default = `src` size):

```
logical_pivot  (untrimmed coords)  = explicit: sprite_visual_def.origin
                                     grid:     origin_rule over source_size
                                               (top_left=(0,0), center=(W/2,H/2), bottom_center=(W/2,H))
baked sprite_visual.origin (packed) = logical_pivot - trim_offset
```

Untrimmed frames (`trim_offset == (0,0)`) reduce to `origin == logical_pivot`, so the trimmed and
untrimmed paths agree and no frame jitters as an animation crosses trimmed/untrimmed frames.

## 3. Design decisions (agreed)

- **The cache owns; the builder is transient.** No caller-held RAII owner for *definitions*;
  ownership is the content-keyed, refcounted cache, exactly as `resource_cache` does for tiles.
- **Shared core, two facades.** Extract `render_bundle` (ordered RAII teardown) and the cache
  engine (key + refcount + cold LRU + image stat-cache); `tileset_bundle`/`sprite_set` and the
  tile/sprite caches are thin front-ends. One caching implementation, one teardown discipline.
- **Shared-vs-per-instance state is the reason they stay two facades.** Tiles bake shared
  lockstep states; sprites hand out `animation_id`s that instances turn into their own states.
- **Spawned states are lifetime-safe by lease.** `sprite_instance` retains a `sprite_set_handle`,
  so a set with live instances is never evicted; the instance unregisters its state before
  dropping the lease. This makes the shared cache handle a refcounting lease (copy retains).
- **Deterministic build semantics.** Explicit visuals may override grid names; duplicate explicit
  visual names or duplicate clip names are build errors; the content key folds `visuals`/`clips`
  in declared order, never map order.
- **One trim/origin formula** — `baked origin = logical_pivot(untrimmed) − trim_offset`, so
  trimmed and untrimmed frames align and don't jitter.
- **Both visual modes in v1** — grid *and* named sub-rects, per-visual origins + a grid origin
  rule.
- **Trim in v1** — `source_size` + `trim_offset`, so exported atlases load correctly.
- **`sprite_def` reuses `image_from_disk/memory/surface`** as its atlas source → sprites load
  from file, embedded bytes, or a procedural surface, and are cacheable, for free.

## 4. Phase plan

Continues the world plan's spirit: each phase ships behavior-preserving where it is a
refactor, with the suite green before the next.

### Phase S1 — extract `render_bundle` (refactor) — prerequisite ✅ DONE

Lift `{atlases, sheets, animations, states}` + the dependency-ordered teardown out of
`tileset_bundle` into a `render_bundle`; make `tileset_bundle` hold one and keep its
local-id `visuals`/`states` facade. Pure refactor.
- **Tests:** whole suite green; bundle teardown order unchanged (existing RAII tests cover it).

**Done:** new `video/sprite/render_bundle.{hh,cc}` (it aggregates sprite ids and is shared by
the sprite *and* world facades, so it lives with the sprite primitives, not under `world`) owns
`{atlases, sheets, animations, states}` with the ordered `release()`
(states→animations→sheets→atlases), move-by-swap, RAII destructor.
`tileset_bundle` now **inherits** `render_bundle` and adds only `visuals` + the `visual`/`state`
facade — so every existing `bundle.atlases/…` access still resolves (zero call-site churn).
`destroy_bundle` routes through `release()` + `visuals.clear()`. Its move ops are **explicit**
(delegate the owned ids to the base's swap-move, then swap `visuals` — and clear ours first on
move-assign, since the base clears only its own vectors), so a moved-from bundle is *provably*
empty rather than in `std::vector`'s valid-but-unspecified state, keeping the "empty after move"
contract uniform (portable — not dependent on libstdc++ emptying the vector). Suite 583/583.

### Phase S2 — extract the cache engine (refactor) ✅ DONE

Lift content-key map + refcount + cold-LRU + image stat-cache from `resource_cache` into a
reusable core (a `resource_cache_core<Bundle>` template, or a shared engine object); make the
tile cache a thin front-end. Also lift `image_identity` (currently anon-ns in
`resource_cache.cc`) to a shared spot — the sprite cache needs the same disk/memory/surface
identity. Decide one-unified-cache vs two-instances here (lean: two instances of the core,
one shared budget later if needed). The core's handle becomes a **refcounting lease** (copy
retains, destroy releases) so a `sprite_instance` can hold one (§Per-instance state); the tile
front-end keeps its current acquire/release behavior on top.
- **Tests:** whole suite green (the tile cache behaves identically); a copied handle retains
  the entry and its destruction releases (the lease semantics S6 relies on).

**Done:** new `video/sprite/image_identity.{hh,cc}` — `image_identifier` (disk/memory/surface
identity + the file stat-cache), reusable by both caches. New internal template
`src/.../video/sprite/resource_cache_core.hh` — `resource_cache_core<Bundle>` (entries map +
refcount + cold LRU + tokens, `acquire(key, build)`/`retain`/`release`/counts) and
`resource_lease<Bundle>` (RAII: adopt-on-construct, **copy-retains, destroy-releases**, move).
`resource_cache` is now a thin front-end: `impl` holds `resource_cache_core<tileset_bundle>` +
an `image_identifier`, `key_for` uses `identifier.identity`, and acquire/release/counts
delegate — `bundle_handle` and its manual acquire/release are unchanged (tile behavior
identical). Went with **two core instances** (sprite cache instantiates
`resource_cache_core<sprite_set>` in S5). Suite 586/586 (+3: core miss/hit/cool, lease
copy-retain + move, cold-pool overflow evicts LRU and destructs exactly once — via a
GPU-free fake bundle). map_viewer still links.

### Phase S3 — `sprite_def` model + `key_for(sprite_def)` ✅ DONE

The pure data types (§2), a `sprite_grid` expander (grid → indexed `sprite_visual_def`s with
the origin rule), and a content key folding image identity + every visual (name, src, origin,
trim) + every clip (name, frames, durations, loop). No GPU.
- **Tests:** grid expansion (counts, rects, origin rule, margin/spacing); key stability and
  sensitivity (distinct visuals/clips/trim ⇒ distinct keys; identical ⇒ equal).

**Done:** new `video/sprite/sprite_def.{hh,cc}` — the model (`sprite_origin_rule` [9-grid] +
`origin_for`, `sprite_grid`, `sprite_visual_def` with `source_size`/`trim_offset`,
`sprite_frame_def`, `sprite_clip_def`, `sprite_def`), `expand_grid` (row-major, derives
columns/count from the image like a uniform tileset, bakes the rule origin over the cell), and
`key_for(sprite_def, image_identifier&)` folding image identity + grid + visuals + clips **in
declared order** (float durations via `bit_cast`, strings via `content_hash`). Also lifted a
shared `fold_image_identity` into `image_identity` and routed the **tile** `key_for` through it
(de-dup; tile key verified unchanged by the existing cache tests). Suite 590/590 (+4: origin
rules, grid slicing/derivation/margin-spacing, key stability + per-field sensitivity, trim
changes the key).

### Phase S4 — `build_sprite_set` + `sprite_set` facade ✅ DONE

Build from a `sprite_def`: decode `image` (the shared decode path), pack/upload the atlas,
create a sheet with named visuals (grid-expanded + explicit, with the trim/origin formula
baked), register one animation per clip (resolve frame `visual` names → refs → appearances).
Apply the deterministic name rules (grid-then-explicit; reject duplicate explicit visual / clip
names). Populate the `render_bundle` (no states) + the name maps. `visual(name)` / `clip(name)`.
- **Tests:** a def with a grid + a couple of clips builds; `visual`/`clip` resolve by name;
  an unknown name is `nullopt`; an explicit visual overrides a grid name; a **duplicate explicit
  visual name and a duplicate clip name each fail the build**; a trimmed frame's baked origin ==
  `logical_pivot − trim_offset`; teardown is order-clean (RAII).

**Done:** new `video/sprite/sprite_set.{hh,cc}` — `sprite_set` inherits `render_bundle`, adds
`visuals_by_name`/`clips_by_name` + `visual(name)`/`clip(name)`. `build_sprite_set`: decodes the
image (owned surface; the sheet is already packed so it uploads **whole** and references authored
sub-rects in place — no repack), `resolve_visuals` merges grid-then-explicit with override/reject,
bakes `origin − trim_offset`, and registers one `sprite_animation(frames, loop)` per clip — no
states (per-instance). RAII-records into the base as it registers, so a mid-build throw tears down
in order. Added `baked_visual_origin` to `sprite_def` (testable formula). Suite 595/595 (+5:
formula, build/resolve by name, grid override, duplicate visual/clip + unknown-visual build errors,
RAII teardown+rebuild). *Note:* the def→surface decode is a small deliberate duplicate of the tile
path's (which borrows the surface arm; sprites need an owned upload) — a candidate for later
unification.

### Phase S5 — `sprite_cache` ✅ DONE

`sprite_cache::acquire(sprite_def) → sprite_set_handle` over the shared engine (refcount, cold
pool, `switch_to`-style delta). Identical `sprite_def`s share one `sprite_set`.
- **Tests:** acquire-twice shares; release drops to cold and stays resolvable; cold eviction
  tears down in order; two defs with byte-identical images but different clips/visuals do not
  share.

**Done:** new `video/sprite/sprite_cache.{hh,cc}` — `sprite_cache` holds a
`resource_cache_core<sprite_set>` + an `image_identifier`; `acquire(def)` keys via
`key_for(def, ident)` and builds via `build_sprite_set` on a miss. `sprite_set_handle` is a
**RAII refcounting lease** (bespoke over the cache's private `retain`/`release`, so no
template/lru leaks into the public header): the acquire adopts one ref, **copy retains, move
transfers, destruction releases**, and it forwards `visual(name)`/`clip(name)` to the leased
set. Suite 600/600 (+5: identical defs share / distinct content builds new; a lease copy keeps
the set resident after the original drops; cold resurrect; move transfers without a refcount
change; cold-pool overflow evicts LRU).

### Phase S6 — `sprite_instance` (leased per-instance state) ✅ DONE

`sprite_set_handle::spawn(clip)` → `sprite_instance` holding a **lease** (a `sprite_set_handle`)
plus its own `sprite_state_id`; the destructor unregisters the state, then drops the lease. The
switch/restart helpers route through it. This is what a `render_layer` actor holds.
- **Tests:** an instance spawns, animates, and unregisters cleanly; two instances of one clip
  advance independently; **a set with a live instance is not evicted, and its animation stays
  resident, even after the owner releases its own handle** (the lease keeps it); dropping the
  last instance lets the set go cold.

**Done:** `sprite_instance` (co-located in `sprite_cache.{hh,cc}` to keep `spawn` on the handle
without a circular include) owns a `sprite_state_id` + a `sprite_set_handle` lease.
`sprite_set_handle::spawn(name)` resolves the clip, `create_sprite_state`s a playhead, and hands
back an instance holding a **copy** of the lease (unknown clip → invalid instance). Move-only;
the destructor `unregister_sprite_state`s **first** (animation still resident via the lease),
then the lease member releases. `switch_to`/`restart` route through the lease's `clip()` to
`switch_sprite_animation`/`restart_sprite_animation`. Suite 603/603 (+3: spawn + switch/restart +
unknown-clip; independent playheads + clean move; **the lease keeps the set resident after the
owner handle drops, its animation still switchable, and the set cools only when the instance
drops**).

### Phase S7 — atlas loader + demo payoff (fixture in v1) ✅ DONE

A loader that populates `sprite_def` from an exported atlas, and a `sprite_demo` rewrite using
`sprite_cache` + `sprite_instance` — the ~40-line assembly + nine unregisters collapse to a def
+ `acquire` + `spawn`. **v1-complete requires one small real fixture** (an Aseprite or
TexturePacker export) proving an exported atlas becomes a `sprite_def` with no hand code, even
if broader format support waits.
- **Tests:** the fixture atlas (frames, trim, clips) loads into a def that builds and resolves
  its clips.

**Done:** new `video/sprite/atlas_loader.{hh,cc}` — `load_aseprite_atlas` parses Aseprite's
`--format json-array` export (via nlohmann/json) into a `sprite_def`: frames → named visuals
`"0".."N-1"` with trim from `sourceSize`/`spriteSourceSize`, `meta.frameTags` → named clips over
frame indices (forward/reverse) with per-frame durations, `image_from_disk{meta.image}` +
`meta.size`. `sprite_demo` rewritten: `make_demo_def()` (data) → `m_cache.acquire(def)` →
`spawn("idle"/"torch"/"coin")`, with `switch_to`/`restart` for state changes and RAII members
(cache → handle → instances) replacing all nine `unregister_*`. Suite 605/605 (+2: a real
Aseprite export parses frames/trim/tag-clips; a loaded atlas builds through the cache and spawns
an instance with no hand code). `sprite_demo` builds + runs headless.

---

**v1 complete (S1–S7).** Sprites are now first-class loadable, cacheable assets on the same
shape as tiles: `sprite_def` (pure) → `sprite_cache` (content-keyed, refcounted) → `sprite_set`
(name facade over the shared `render_bundle`) → leased `sprite_instance` playheads. Deferred:
broader atlas formats (TexturePacker / json-hash), unifying the def→surface decode with the tile
path's borrow, and a per-visual pivot in the loader (basic exports carry none).

### Build order

    S1 → S2 → S3 → S4 → S5 → S6 → S7

S1/S2 are behavior-preserving refactors the rest builds on; S3–S6 are the new pipeline; S7's
one-fixture loader is part of v1 (the loadable-asset claim), broader format support deferred.
