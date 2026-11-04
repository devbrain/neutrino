# Sprite layer redesign — implementation plan

Status: **agreed design, not started** (written 2026-11-03; revised 2026-11-04: one
sequence per `sprite_def`, D1 / D15). Trello: **[P3.7] Sprite layer
redesign**. Vocabulary and atlas model: [`images_atlases_sprites.md`](images_atlases_sprites.md)
(§3.3–3.4 atlas, §9 sprite decisions). This document is the *how* and the *order*.

The atlas foundation is in the tree (commit `7cfdaec`, WIP): an atlas is an indexed list of
pictures, a picture is addressed by `atlas_address {atlas, picture_idx}`, and the texture
registry maps that to `{page texture, rect}`. The tree does not build until the sprite
layer is moved onto it; milestone M0 fixes that first.

---

## 1. Why

`video/sprite/` is ~5,800 lines, 15 public headers, a manager and three registries. A usage
survey of every real user (KE, Luxor, `sprite_demo`, the tutorials, the world renderer)
found:

- **Most of the surface is unused.** Aseprite loading, trim, `bounding_size`, grid slicing,
  appearance flip/visibility, most animation free functions and bitmasks are used by tests
  only or by nothing.
- **Three picture types for one thing** (sheet / visual / appearance), and raw ids with no
  RAII: KE unregisters states by hand, and `render_bundle` exists only to tear down ids in
  the right order.
- **Animations can't be defined before upload**, because they reference uploaded visuals.
- **One clock for everything.** Sprite states advance in `application::on_update` with the
  frame `dt`, ignoring the scene stack: a covered gameplay scene keeps animating, and KE
  runs its own 70 Hz frame counters next to the engine's.
- **Pivots are everyone's own problem**: KE forces (0,0) and centres by hand or negates BOB
  offsets, tutorials hard-code them per visual, the world renderer computes its own.
- **Named multi-clip defs serve nobody.** Only `sprite_demo` and tutorial 03 put several
  named clips in one def and switch between them, and every switch restarts the clip
  (`switch_sprite_animation` resets elapsed time), so a switch to another def behaves the
  same. KE uses no clips: about 30 hand-registered single-sequence animations plus frames
  picked by index from game state. Luxor drops the `.spr` animation list. The world renderer
  has one animation per animated tile. Warlords has one sequence per key and draws only the
  first frame. brix's definition (`object_meta_data`) is exactly one frame range; the
  *instance* holds several sequences by index and switches between them, sometimes across
  unrelated definitions (a gem's slot 1 is the shared `GEM_EXPLOSION`).

Before deciding, the design was compared with brix (`~/proj/ares/brix`), Godot 4
(`SpriteFrames` / `AnimatedSprite2D`, tile animation slices), cocos2d-x (`SpriteFrame`,
`Animation` / `Animate`, `ActionManager`) and warlords (`~/proj/neutrino/warlords`). What
it took from them:

- **Class / instance split** (brix's model; Godot `SpriteFrames` vs `AnimatedSprite2D`,
  cocos `Animation` vs `Animate`).
- **Pivot per picture with a per-frame override** (cocos frame anchor, Unity sprite pivot);
  Godot shows a pivot only on the node is too weak when frame sizes differ.
- **Carry the leftover time** (Godot); brix and Cocos Creator reset it and drift.
- **Looping animations never "finish"** (Godot signals).
- **Animated tiles as a pure function of a clock** with an optional per-cell phase (Godot
  `TileMapLayer` animation slices, `RANDOM_START_TIMES`).
- **Tint/alpha** in every engine; warlords needs colour/alpha mod, dest-rect fit modes and
  an empty "no picture" value, and has its own lazily built, name-keyed atlas cache.
- **Source insets** replace brix's six `draw_tile_*` crop variants.

## 2. Decisions

| # | Decision | Status |
|---|---|---|
| D1 | **`sprite_def` is the class, `sprite` is the instance** (as in brix: Gem is a class instantiated many times). A `sprite_def` is immutable and holds **one** animation sequence: a list of frames + loop mode (D15). It is built from atlas addresses at load time, before any GPU. | decided |
| D2 | **A `sprite` only animates.** It holds the playhead (current def, elapsed time, speed). The game positions and draws it (immediate mode, as today). Retained-mode sprites (engine-drawn, with position and layer) are out of scope. | decided |
| D3 | **The engine advances sprites, not the game.** No `update(dt)` call in game code. | decided |
| D4 | **Sprites belong to a scene** and advance exactly when their scene receives `fixed_update`, with the same `dt` (simulation time, deterministic). | decided |
| D5 | **`scene_flags` bitfield replaces `is_opaque()`**: `hides_below` (scenes below not rendered) and `pauses_below` (scenes below not updated, so their sprites freeze). Default: both. | decided |
| D6 | **An application always has at least one scene.** `simplex` gets one built-in scene that forwards to its `on_update` / `on_render`. The scene-free code path is removed. | decided |
| D7 | **Events: polling plus optional callbacks.** `finished()`, `looped()`, `frame_changed()` for the last advance; optional `on_finished` / `on_looped` callbacks called by the engine during the scene's update. A looping animation never finishes. | decided |
| D8 | **Timing is exact.** The playhead carries the leftover time across frames (no drift, several frames per tick if needed); `frame_at(elapsed)` is a pure function over precomputed cumulative times (binary search). Durations are absolute milliseconds. | decided (from the research) |
| D9 | **Pivot: a per-picture default plus an optional per-frame override.** `image_bank::entry` gets an optional pivot that travels into the atlas placement and the registry; a frame may override it. Default (0,0) = top-left. | proposed |
| D10 | **Animated tiles are not sprite instances.** The world renderer evaluates the tile's `sprite_def` with the pure `frame_at`, against its scene's clock plus a per-cell phase. No per-tile state; tiles freeze when the map scene is paused. | proposed |
| D11 | **Draw parameters gain tint/alpha and a source inset; `draw_picture` gets a fit mode** (stretch / aspect-fit / centre). Tint: every engine studied has it, and warlords needs colour/alpha mod. Inset: brix's shrink / sunken / offset effects. Fit: warlords's portraits and shields. | proposed |
| D12 | **Lifetime is refcounted, not registry ids.** `register_atlas` returns an RAII `atlas_handle`; a `sprite_def` holds the handle of every atlas it uses; a `sprite` holds its `sprite_def`. The last holder unregisters the atlas. `render_bundle` goes. | proposed |
| D13 | **Asset layer = `atlas_cache` + a PNG convenience loader.** `atlas_cache`: name → lazily built `atlas_handle` via a game-supplied builder, with reload (warlords's `texture_cache`, KE, the world renderer). The loader: PNG + rects + sequences → one `sprite_def` per sequence, delivered into the game's own def slots through a game-supplied name → slot mapping (tutorials, `sprite_demo`). `sprite_set`, `sprite_cache`, `sprite_instance`, Aseprite, trim and grid slicing are dropped until a game asks. | proposed |
| D14 | **Migration keeps the tree green at every milestone.** M0 moves the *existing* sprite layer onto the new atlas (a bridge), the new layer is built next to it, users move one by one, and the old layer is deleted last. | proposed |
| D15 | **One sequence per `sprite_def`; switching = playing another def.** `sprite::play(def)` points the instance at a different def (restart, or keep the phase); any def, not only "its own" (brix's gem → shared explosion). Grouping ("the hero's animations") is the game's business: a plain struct of `sprite_def` members, declared ahead and typed by the compiler. No names, no animation ids, no enum keys inside the engine; names exist only in loaders, mapped once to the game's slots. Stills and frames picked from game state need no def: draw the `atlas_address` (KE, warlords). | decided |
| D16 | **`sprite_def` is a cheap value handle** over shared immutable data (internally refcounted). It can be a member of a game struct and copied freely; a `sprite` copies the handle on `play`, so no sprite outlives its def's data. Default-constructed = empty (not loaded yet); `play` / `spawn` of an empty def is an ENFORCE. | proposed |

Out of scope, recorded for later: ping-pong loop mode, per-frame user tags (cocos
`userInfo`), trimmed atlases (`original_size` + offset, mirrored on flip around the
original size), relative duration units, retained-mode sprites, a Nuklear bridge for
warlords, keeping CPU pixels after upload.

## 3. Target API (sketch)

Names are final where decided; signatures are indicative.

```cpp
// ---- atlas (done, plus D9) -----------------------------------------------------------
struct atlas_address { atlas_id atlas; std::size_t picture_idx; };   // invalid atlas = "no picture"
class  atlas_handle;                       // RAII, refcounted; unregisters on last release
atlas_handle register_atlas(image_atlas&&, atlas_texture_format = automatic);
dim   picture_size(atlas_address);         // also image_atlas::picture_size(i) before upload
point picture_pivot(atlas_address);        // D9, likewise on image_atlas

// ---- the class ---------------------------------------------------------------------
struct sprite_frame {
    atlas_address             picture;
    std::optional<point>      pivot;       // overrides the picture's pivot (D9)
    sprite_flip               flip{};
    std::chrono::milliseconds duration{};
};
enum class sprite_loop : std::uint8_t { once, loop };

class sprite_def {                         // one sequence (D15); value handle over shared data (D16)
    public:
        sprite_def() = default;            // empty: not loaded yet
        static sprite_def still(atlas_address);   // shortcut: one frame, never finishes

        [[nodiscard]] bool empty() const noexcept;
        [[nodiscard]] std::size_t frame_count() const;
        [[nodiscard]] sprite_loop loop() const;
        [[nodiscard]] std::chrono::milliseconds duration() const;
        [[nodiscard]] const sprite_frame& frame_at(std::chrono::milliseconds elapsed) const;  // pure
};

class sprite_def_builder {                 // builds ONE sequence
    public:
        sprite_def_builder& frame(atlas_address, std::chrono::milliseconds,
                                  sprite_flip = {}, std::optional<point> pivot = {});
        sprite_def_builder& frames(atlas_address first, std::size_t count,
                                   std::chrono::milliseconds each);     // consecutive pictures
        sprite_def_builder& loop(sprite_loop);                          // default: loop
        sprite_def build() &&;                                          // validates
};

// the game groups its defs itself, declared ahead, filled at load
struct hero_sprites { sprite_def idle, walk, jump; };

hero.walk = sprite_def_builder{}.frames({atlas, 4}, 4, 80ms).build();
hero.jump = sprite_def_builder{}.frame({atlas, 8}, 120ms).frame({atlas, 9}, 200ms)
                                .loop(sprite_loop::once).build();

// ---- the instance (D1-D4, D7, D15) ---------------------------------------------------
class sprite {                             // RAII handle into the owning scene's sprite pool; movable
    public:
        void play(const sprite_def&, keep_phase = keep_phase::no);   // switch to any def
        void restart();
        void set_speed(float);             // 1 = authored speed
        bool finished() const;             // once-animations only
        bool looped() const;               // during the last advance
        bool frame_changed() const;        // during the last advance
        const sprite_frame& frame() const;
        const sprite_def& def() const;     // what is playing now
        void on_finished(std::function<void(sprite&)>);
        void on_looped(std::function<void(sprite&)>);
};

class base_scene {
    protected:
        sprite spawn(const sprite_def&);                     // owned by this scene (D4)
    public:
        virtual scene_flags flags() const;                   // D5, replaces is_opaque()
};

// ---- drawing ---------------------------------------------------------------------------
struct sprite_draw_params {
    float       scale{1.0f};
    sprite_flip flip{};
    float       rotation_degrees{0.0f};
    color       tint{colors::white};       // D11, alpha included
    insets      crop{};                    // D11, source inset in picture pixels
};
void draw_sprite(position, const sprite&, const sprite_draw_params& = {});
void draw_sprite(position, const sprite_frame&, const sprite_draw_params& = {});
void draw_sprite(position, atlas_address, const sprite_draw_params& = {});   // a still, picture pivot
void draw_picture(const rect& dst, atlas_address, sprite_flip = {}, fit = fit::stretch);
// sprite_batch::add gains the same three sources
```

## 4. Milestones

Dependencies: **M0 → M2 → M3 → M4 → M5 → M6**; **M1** is independent of M0 and must land
before M2's scene integration. Every milestone ends with the whole tree building and
`neutrino_tests`, `simplex_entry_tests`, `luxor_unittest`, `game_format_tests` green.

### M0 — Green build on the new atlas (bridge) — **done 2026-11-04**

Goal: the WIP commit compiles and every test passes, with the *current* sprite API moved
onto `atlas_id` / `atlas_address`. No new sprite concepts yet. What was done:

1. Atlas tests: `.dest` / `page()` replaced by `picture_size` and a test-only
   `details::image_atlas_access` (`test/video/image_atlas_access.hh`, a friend of
   `image_atlas`) for the page / placement pixel checks.
2. `test_atlas_packer.cc` and `test_image_atlas.cc` deleted (they tested deleted code; the
   `atlas_frame_old` bitmask cases went with it). An `atlas_id` hashing / `operator<<` case
   and a `build_atlas(image)` case were added to `test_build_atlas.cc`.
3. `image_loader.hh`: the `atlas_packer.hh` include dropped.
4. `sprite_sheet` is bound to an `atlas_id`. `sprite_visual` gains `picture` (an index into
   the atlas); `texture_rect` is now *picture-local*, a sub-rect of that picture.
   `sprite_sheet(const image_atlas&)` gives one whole-picture visual per picture, and
   `register_sprite_sheet(image_atlas&&)` consumes the atlas. `sprites.hh` drops
   `image_atlas_old`.
5. `render_bundle`, `sprites_manager::uses`, `unregister_atlas`: `atlas_id`.
6. `draw.cc` resolves `{page texture, placement}` with `texture_registry::get(atlas,
   picture)` and offsets the visual's sub-rect by the placement; the diagonal-flip UVs use
   the page texture's size.
7. `build_sprite_set`: the def's image becomes a one-picture atlas (`build_atlas(image)`)
   and the authored rects stay sub-rects of it — no cropping, so overlapping rects and the
   trim metadata keep working unchanged.
8. `tileset_bundle`: tiles are cropped into an RGBA `image_bank` and packed by one
   `build_atlas` (gutter with edge extrusion); one atlas, one sheet, visual i = picture i.
   Its tests now expect one atlas and one sheet even when the atlas spans several pages.
9. KE: the sprite gallery and its font sheet use one-picture atlases with sub-rect visuals;
   `resources::backdrops` holds plain images.
10. The `neutrino_stb_rect_pack` target is gone (stb stays pinned for onyx_image).

Exit (met): full build; `neutrino_tests` 745/745, `luxor_unittest` 29/29,
`game_format_tests` 7/7, `simplex_entry_tests` 15/17 (the two card #19 draw-colour cases,
fixed separately afterwards: simplex no longer clears the canvas before `on_update`);
KE, Luxor, `sprite_demo`, tutorials 02 / 03, drawing 02 / 03 and the map viewer start and
draw headless without errors.

### M1 — Scene foundations (D5, D6)

1. `scene_flags { none, hides_below, pauses_below }` with bit operators (consider one
   generic enum-flags helper in `neutrino/detail/` shared with `sprite_flip` and
   `hotkey::modifier`).
2. `base_scene::flags()` replaces `is_opaque()`; default `hides_below | pauses_below`.
3. `scenes_manager`: render walks down to the first `hides_below`; `fixed_update` walks down
   to the first `pauses_below`; input still top-only. Tests for all four combinations
   (opaque, `busy`-style overlay, translucent pause menu, hidden-but-running).
4. Every application has a scene: `create_initial_scene()` must return one (ENFORCE);
   remove `m_had_scenes` and the scene-free branches in `application`; an empty stack
   quits.
5. `simplex`: the adapter pushes one built-in scene forwarding `fixed_update` → `on_update`
   and `render` → `on_render`; remove its `update_scene` / `render_scene` overrides.
   **Behaviour change:** `on_update` receives the fixed tick instead of the frame delta —
   note it in the simplex docs and fix `simplex_entry_tests`.
6. Tutorial 01's `is_opaque` comment → `flags()`.

Exit: scene tests cover the flag matrix; simplex runs through its scene.

### M2 — Sprite core (D1–D4, D7–D9, D12, D15, D16)

0. Rename the current asset-path `sprite_def` (and its builder) out of the way, e.g.
   `sprite_asset_def`, so the new names are free. Mechanical; the old type is deleted in M6.
1. **Pivots (D9):** optional `pivot` on `image_bank::entry` (and `image_bank_builder::add`);
   carried into the atlas placement and the registry; `picture_pivot(address)` /
   `image_atlas::picture_pivot(i)`.
2. **`atlas_handle` (D12):** `register_atlas` returns it; refcounted; the last release
   unregisters. `unregister_atlas(atlas_id)` stays for callers not yet ported.
3. **`sprite_def` + `sprite_def_builder`:** one sequence (D15) behind a value handle
   (D16): frames, loop mode, cumulative frame times, pure `frame_at`. The builder appends
   frames one by one (`frame`) or as a run of consecutive pictures (`frames`), sets the
   loop mode, and `build()` validates (no frames, a zero-duration frame in a multi-frame
   sequence; frames from unregistered atlases are allowed — defs are built before upload).
   `sprite_def::still(address)` is the one-frame shortcut. Holds the `atlas_handle`s of its atlases once
   they are registered (a def built before upload binds them at `register_atlas` or on
   first spawn — decide during implementation).
4. **Sprite pool per scene:** `base_scene` owns a pool of instance states; `spawn` returns a
   `sprite` handle; handles are movable, invalidated safely if the scene dies first (a
   generation-checked slot, not a dangling pointer).
5. **Advance:** `scenes_manager` advances a scene's pool immediately *before* calling its
   `fixed_update`, with the same `dt`, so the scene sees fresh `finished()` / `looped()`
   flags. Leftover time carries; speed scales `dt`; callbacks fire inside the advance.
6. **Playhead operations:** `play(def)` (switch to any def, restarting or keeping the
   phase), `restart`, `set_speed`, current frame, current def.
7. Tests: `frame_at` boundaries and binary search; remainder carry over many ticks; several
   frames in one tick; once vs loop (a loop never finishes); `looped` / `frame_changed`
   flags; callbacks; speed 0 / 2; switch to another def (restart and phase-keeping, the
   phase wrapped into the new def's duration); switching to the def already playing;
   empty def is an ENFORCE; a def copied into a game struct outliving the original; pause through `pauses_below`
   (sprites of a paused scene do not advance); handle outliving its scene; def outliving
   every sprite; atlas freed when the last def / sprite goes.

Exit: the new core is complete and tested in isolation; nothing uses it yet.

### M3 — Draw path (D11)

1. `draw_sprite(pos, sprite | sprite_frame | atlas_address, params)` on top of the existing
   draw implementation. Preserve: pixel snapping, scale about the pivot, flip mirroring the
   pivot, diagonal flip through `render_geometry`, rotation byte-identical at 0°,
   `DRAW_CHECK` for SDL failures, throwing on bad arguments.
2. `draw_picture(dst, address, flip, fit)`: the exact-integer dst path (seam-free tiles)
   plus fit modes.
3. `tint` (colour + alpha mod, restored after the draw) and `crop` (source inset) in
   `sprite_draw_params`; the dst size follows the cropped source.
4. Drawing an address whose atlas is not registered, or out of range, is a reported no-op
   through `DRAW_CHECK`; an invalid address (no picture) is a silent no-op.
5. `sprite_batch::add` overloads for `sprite`, `sprite_frame` and `atlas_address`, screen
   and world modes; keep the stable `(layer, depth, insertion)` sort.
6. Port the relevant `test_draw.cc` cases to the new entry points; pixel tests for tint,
   crop and each fit mode.

Exit: everything the old draw API can do, the new one can, plus D11.

### M4 — Port the users

In this order; each step builds and runs on its own.

1. **KE** (the most demanding user, validates the core):
   - sprite sets → `sprite_def`s built from its own BOB atlas (its decoder already returns
     a bank; BOB offsets become picture pivots, removing the hand centring);
   - effects and capsules: engine sprites (were `sprite_state_id`), one def each;
   - frames picked from game state (paddle by state/size, ball, bricks, digits) stay
     index-based: draw the `atlas_address`, no def;
   - enemy alive → death is `play(death_def)`; the shield keeps its shared phase through
     `keep_phase` or the scene clock;
   - paddle, enemies, shield, deaths: from 70 Hz tick counters to sprites with durations
     in ms, if the timing matches (check frame-exactness against the original at 70 Hz;
     keep the tick counter where it does not);
   - drawing stays in its screen-space `sprite_batch`;
   - delete the manual `unregister_*` teardown and the private `sprite_cache` member;
   - the sprite gallery scene moves too.
2. **World renderer:** `tileset_bundle` = `atlas_handle` + one `atlas_address` and pivot per
   tile + one `sprite_def` per animated tile; animated tiles use `frame_at(scene clock +
   phase(cell))` (D10) — the renderer needs the owning scene's elapsed time; seam-free
   orthogonal tiles use `draw_picture`; rotated / diagonal / hex / iso tiles use
   `draw_sprite`. `resource_cache` stays as the world's cache, holding `atlas_handle`s.
3. **`sprite_demo`, tutorials 02 / 03, drawing 02 / 03:** onto the PNG loader (M5 step 2 is
   needed first, or temporarily build the def by hand).
4. **Luxor:** it builds defs but draws nothing; port the loader and its tests. Its `.spr`
   animation list (e.g. a button's normal / hover / pressed / disabled) becomes one def
   per entry, which is what the parser already describes.

### M5 — Asset layer (D13)

1. `atlas_cache`: `acquire(name, builder) → atlas_handle` (built lazily on first use),
   `reload(name)`, `release_unused()`; keyed by the game's name.
2. PNG convenience loader: image + named rects (+ pivots) + named sequences → its atlas
   plus one `sprite_def` per sequence, written into the game's slots through a
   `{name → sprite_def&}` mapping (unknown and missing names are reported); replaces
   `sprite_def_builder::from_file … add_visual … add_clip`. No string survives the load.
3. Tutorials and `sprite_demo` finish their move onto it.

### M6 — Delete the old layer, update the docs

1. Delete: `sprite_sheet`, `sprite_visual` / `_id` / `_ref`, `sprite_appearance`,
   `sprite_animation` (+ id), `sprite_state` (+ id), `sprites_manager`,
   `resource_registry`, `render_bundle`, the old `sprite_asset_def` / builder / `sprite_set`
   / `sprite_cache` / `sprite_set_handle` / `sprite_instance`, `atlas_loader` (Aseprite),
   `image_identity` if unused, the `application::on_update` sprite tick, and their tests.
2. Docs: `images_atlases_sprites.md` (§3.5–3.9, §4, §5 rewritten for the new layer; §9
   becomes the reference), the `sprites.hh` vocabulary section, the tutorials' text,
   `sprite_asset_pipeline.md` (obsolete: delete or mark superseded),
   `world_sprite_integration.md`, the roadmap.

Exit: the old API is gone; `video/sprite/` contains only the new layer.

### M7 — Later, on demand

- **warlords** as the first new user: `atlas_cache` for its sets, `draw_picture` with fit
  modes, tint for fonts; a Nuklear bridge (`address → {texture, rect}`); keeping CPU pixels
  for the minimap (or computing it before upload).
- ping-pong loops, per-frame user tags, trimmed atlases, relative durations,
  `make_bitmask` collision on sprites (P3.3).

## 5. Risks and open points

| Risk / question | Plan |
|---|---|
| KE animations driven by 70 Hz ticks may not be frame-exact in ms on a 120 Hz tick | Compare frame sequences in a test; KE may set its fixed period to 1/70 s, or keep the counter for those animations |
| `sprite` handles outliving their scene | Generation-checked slots; a dead handle reports `finished()` and draws nothing |
| A `sprite_def` built before upload must still keep its atlas alive afterwards | Decide in M2.3: bind handles at `register_atlas` time (registry knows which defs reference an id) or require the def builder to receive the handle |
| `play(def)` accepts any def, so `hero.play(coin_spin)` compiles | Accepted (D15): switching across defs is a feature (brix's shared explosion); the game's typed struct of defs is the guard |
| Simplex `on_update` changes from frame delta to fixed tick | Document; adjust `simplex_entry_tests` (all green since the card #19 canvas fix, so any new failure is this change) |
| The world renderer needs its scene's clock | Pass elapsed scene time into `world_renderer::render` (or let the renderer read the current scene's clock) — decide in M4.2 |
| Ordering: sprites advance before or after the scene's `fixed_update` | Before (M2.5), so polling sees this tick's events; revisit if a game needs the reverse |
