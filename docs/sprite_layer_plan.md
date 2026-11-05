# Sprite layer redesign — implementation plan

Status: **agreed design, not started** (written 2026-11-03; revised 2026-11-04: one
sequence per animation, D1 / D15; names `animation` / `sprite`, D17; drawing moved to the
painter, D11; atlases application-lifetime, D12). Trello: **[P3.7] Sprite layer redesign**. Vocabulary and atlas model:
[`images_atlases_sprites.md`](images_atlases_sprites.md) (§3.3–3.4 atlas, §9 sprite
decisions). The sprite layer is a *producer*: it paints through the painter described in
[`render_model.md`](render_model.md), which owns everything about placing a picture on
screen. This document is the *how* and the *order* of the sprite layer itself.

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
| D1 | **`animation` is the class, `sprite` is the instance** (as in brix: Gem is a class instantiated many times). An `animation` is immutable and holds **one** sequence: a list of frames + loop mode (D15). It is built from atlas addresses at load time, before any GPU. | decided |
| D2 | **A `sprite` only animates.** It holds the playhead (current animation, elapsed time, speed) and **has no position**. The game decides where it goes and paints it through the painter ([`render_model.md`](render_model.md)). Retained-mode sprites (engine-painted, with position and layer) are out of scope. | decided |
| D3 | **The engine advances sprites, not the game.** No `update(dt)` call in game code. | decided |
| D4 | **Sprites belong to a scene** and advance exactly when their scene receives `fixed_update`, with the same `dt` (simulation time, deterministic). | decided |
| D5 | **`scene_flags` bitfield replaces `is_opaque()`**: `hides_below` (scenes below not rendered) and `pauses_below` (scenes below not updated, so their sprites freeze). Default: both. | decided |
| D6 | **An application always has at least one scene.** `simplex` gets one built-in scene that forwards to its `on_update` / `on_render`. The scene-free code path is removed. | decided |
| D7 | **Events: polling plus optional callbacks.** `finished()`, `looped()`, `frame_changed()` for the last advance; optional `on_finished` / `on_looped` callbacks called by the engine during the scene's update. A looping animation never finishes. | decided |
| D8 | **Timing is exact.** The playhead carries the leftover time across frames (no drift, several frames per tick if needed); `frame_at(elapsed)` is a pure function over precomputed cumulative times (binary search). Durations are absolute milliseconds. | decided (from the research) |
| D9 | **Pivot: a per-picture default plus an optional per-frame override.** `image_bank::entry` gets an optional pivot that travels into the atlas placement and the registry; a frame may override it. Default (0,0) = top-left. | proposed |
| D10 | **Animated tiles are not sprites.** The world renderer evaluates the tile's `animation` with the pure `frame_at`, against its scene's clock plus a per-cell phase. No per-tile state; tiles freeze when the map scene is paused. | proposed |
| D11 | **Placing a picture belongs to the painter, not to sprites:** tint/alpha, source inset, fit modes, pixel snapping, scale about the pivot, flip. Moved to [`render_model.md`](render_model.md) §3.2 (quad parameters) and milestone RM1. | moved |
| D12 | **Atlases live for the application; no handles.** The use case is *load, run, exit* — no current user releases an atlas mid-run. `register_atlas` is internal (loaders, `tileset_bundle`, `atlas_cache`, text); games see only `atlas_id` / `atlas_address`. `animation` and `sprite` hold plain addresses and own nothing. Ids are never reused, so painting from a missing atlas is an ENFORCE at the call site. Replacement in place, group release and device reset are the painter's ([`render_model.md`](render_model.md) §8). `render_bundle` goes. | decided |
| D13 | **Asset layer = `atlas_cache` + a PNG convenience loader.** `atlas_cache`: name → lazily built `atlas_id` via a game-supplied builder, with reload as replacement in place (warlords's `texture_cache`, KE, the world renderer). The loader: PNG + rects + sequences → one `animation` per sequence, delivered into the game's own slots through a game-supplied name → slot mapping (tutorials, `sprite_demo`). `sprite_set`, `sprite_cache`, `sprite_instance`, Aseprite, trim and grid slicing are dropped until a game asks. | proposed |
| D14 | **Migration keeps the tree green at every milestone.** M0 moves the *existing* sprite layer onto the new atlas (a bridge), the new layer is built next to it, users move one by one, and the old layer is deleted last. | proposed |
| D15 | **One sequence per `animation`; switching = playing another animation.** `sprite::play(a)` points the sprite at a different animation (restart, or keep the phase); any animation, not only "its own" (brix's gem → shared explosion). Grouping ("the hero's animations") is the game's business: a plain struct of `animation` members, declared ahead and typed by the compiler. No names, no animation ids, no enum keys inside the engine; names exist only in loaders, mapped once to the game's slots. Stills and frames picked from game state need no animation: paint the `atlas_address` (KE, warlords). | decided |
| D16 | **`animation` is a cheap value handle** over shared immutable data (internally refcounted). It can be a member of a game struct and copied freely; a `sprite` copies the handle on `play`, so no sprite outlives its animation's data. Default-constructed = empty (not loaded yet); `play` / `spawn` of an empty animation is an ENFORCE. | proposed |
| D17 | **Names.** `animation`, `animation_builder`, `animation_frame`, `animation_loop` for the class side; `sprite` for the instance. `sprite_flip` becomes the painter's `flip`, `sprite_draw_params` its `quad_params`. A neutrino sprite has no position — that is the one difference from SFML / libGDX sprites. The new names do not collide with the old layer (`sprite_def`, `sprite_animation`, `sprite_instance`), so both can coexist until M6. | decided |
| D18 | **No global sprite manager.** `sprites_manager` and its `service_locator` slot go. Its jobs split as in §2.1: picture geometry → the atlas; animations → value handles; playheads → a per-scene `sprite_pool` advanced by `scenes_manager`; the atlas registry → the painter. | proposed |

Out of scope, recorded for later: ping-pong loop mode, per-frame user tags (cocos
`userInfo`), trimmed atlases (`original_size` + offset, mirrored on flip around the
original size), relative duration units, retained-mode sprites. (A Nuklear bridge for warlords became the painter's RM8. Keeping CPU pixels after upload moved to the painter: it is how device reset
re-uploads, `render_model.md` §8.3.)

### 2.1 What replaces `sprites_manager`

Today `sprites_manager` (`src/neutrino/video/sprite/sprites_manager.hh`) is a private,
application-wide service: `application` owns it next to the `texture_registry`, publishes
it through `service_locator` (`require_sprites_manager()` / `maybe_sprites_manager()`), and
ticks it once per frame in `application::on_update` with the frame `dt`. It holds three
id-keyed `resource_registry`s and does five jobs. None of them needs a global manager in
the new layer (D18):

| Job today | API today | New layer |
|---|---|---|
| sheet registry: frame geometry over an atlas | `create / get / erase(sprite_sheet_id)`, `uses(atlas_id)` | gone: a picture is an `atlas_address`; its size and pivot come from the atlas (D9). The atlas registry itself stays, internal, owned by the painter (`render_model.md` §8) |
| animation registry | `create / get / erase(sprite_animation_id)` | gone: `animation` is a value handle over shared immutable data (D16); no ids, no registry |
| state registry: playheads | `create(appearance / animation)`, `set_*`, `switch_animation`, `finished`, `erase(sprite_state_id)` | **per-scene `sprite_pool`** (D4, M2.3): `spawn` returns a `sprite` handle into it; generation-checked slots |
| the tick | `update(dt)` from `application::on_update`, frame `dt`, all states | `scenes_manager` advances each updated scene's pool before its `fixed_update`, fixed `dt` (D3, D4, M2.4); paused scenes do not advance |
| lifetime bookkeeping | `uses(...)`, `erase` order, `render_bundle` | gone: atlases live for the application (D12); a sprite dies with its handle or its scene |
| validation | `validate_sheet / _appearance / _animation` | `animation_builder::build()` (frames, durations) and the painter's record-time resolution (addresses) |

`sprite_pool` is internal: a scene owns one, `scenes_manager` advances it, games only see
`sprite` handles and `base_scene::spawn`. It is the per-scene equivalent of the manager's
state registry and nothing more — no sheets, no animations, no atlases.

## 3. Target API (sketch)

Names are final where decided; signatures are indicative.

```cpp
// ---- atlas (done, plus D9) -----------------------------------------------------------
struct atlas_address { atlas_id atlas; std::size_t picture_idx; };   // invalid atlas = "no picture"
// register_atlas is internal (D12): loaders and caches call it; atlases live until exit
dim   picture_size(atlas_address);         // also image_atlas::picture_size(i) before upload
point picture_pivot(atlas_address);        // D9, likewise on image_atlas

// ---- the class ---------------------------------------------------------------------
struct animation_frame {
    atlas_address             picture;
    std::optional<point>      pivot;       // overrides the picture's pivot (D9)
    flip                      mirror{};    // painter's flip (D17)
    std::chrono::milliseconds duration{};
};
enum class animation_loop : std::uint8_t { once, loop };

class animation {                          // one sequence (D15); value handle over shared data (D16)
    public:
        animation() = default;             // empty: not loaded yet
        static animation still(atlas_address);    // shortcut: one frame, never finishes

        [[nodiscard]] bool empty() const noexcept;
        [[nodiscard]] std::size_t frame_count() const;
        [[nodiscard]] animation_loop loop() const;
        [[nodiscard]] std::chrono::milliseconds duration() const;
        [[nodiscard]] const animation_frame& frame_at(std::chrono::milliseconds elapsed) const;  // pure
};

class animation_builder {                  // builds ONE sequence
    public:
        animation_builder& frame(atlas_address, std::chrono::milliseconds,
                                 flip = {}, std::optional<point> pivot = {});
        animation_builder& frames(atlas_address first, std::size_t count,
                                  std::chrono::milliseconds each);      // consecutive pictures
        animation_builder& loop(animation_loop);                         // default: loop
        animation build() &&;                                            // validates
};

// the game groups its animations itself, declared ahead, filled at load
struct hero_animations { animation idle, walk, jump; };

hero.walk = animation_builder{}.frames({atlas, 4}, 4, 80ms).build();
hero.jump = animation_builder{}.frame({atlas, 8}, 120ms).frame({atlas, 9}, 200ms)
                               .loop(animation_loop::once).build();

// ---- the instance (D1-D4, D7, D15) ---------------------------------------------------
class sprite {                             // RAII handle into the owning scene's sprite pool; movable
    public:
        void play(const animation&, keep_phase = keep_phase::no);   // switch to any animation
        void restart();
        void set_speed(float);             // 1 = authored speed
        bool finished() const;             // once-animations only
        bool looped() const;               // during the last advance
        bool frame_changed() const;        // during the last advance
        const animation_frame& frame() const;
        const animation& playing() const;  // what is playing now
        void on_finished(std::function<void(sprite&)>);
        void on_looped(std::function<void(sprite&)>);
};

class base_scene {
    protected:
        sprite spawn(const animation&);                      // owned by this scene (D4)
    public:
        virtual scene_flags flags() const;                   // D5, replaces is_opaque()
};

// ---- painting (render_model.md) -------------------------------------------------------
// The sprite layer adds two convenience overloads on top of the painter's quad API;
// position, depth and quad_params (scale, flip, rotation, tint, crop) are the painter's.
list.paint(position, const animation_frame&, quad_params = {}, depth = {});  // frame pivot + flip
list.paint(position, const sprite&,          quad_params = {}, depth = {});  // = its frame()

struct hero { sprite body; point pos; };   // a "sprite with position" is the game's struct
list.paint(h.pos, h.body);
```

## 4. Milestones

Dependencies: **M0 → M2 → M3 → M4 → M5 → M6**; **M1** is independent of M0 and must land
before M2's scene integration. **M3 depends on painter milestone RM1**
([`render_model.md`](render_model.md) §14), which carries everything about placing a
picture on screen; M1 and M2 do not depend on the painter and can proceed in parallel with
it. Every milestone ends with the whole tree building and `neutrino_tests`,
`simplex_entry_tests`, `luxor_unittest`, `game_format_tests` green.

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

### M1 — Scene foundations (D5, D6) — **done 2026-11-04** (website docs pending)

What was done:

1. `scene_flags { none, hides_below, pauses_below }` (`base_scene.hh`). The operators come
   from a new generic opt-in helper, `neutrino/detail/enum.hh` (`d_NEUTRINO_PROMOTE_ENUM`,
   `flags_enum`, `has`; tested in `test/detail/test_enum.cc`). `flip` and `hotkey::modifier`
   were not moved onto it.
2. `base_scene::flags()` replaces `is_opaque()`; default `hides_below | pauses_below`. A
   scene's flags must not change while it is on the stack.
3. `scenes_manager`: render walks down to the nearest `hides_below`, `fixed_update` down to
   the nearest `pauses_below`; input stays top-only. `on_pause` / `on_resume` follow
   `pauses_below` only (an overlay without it does not pause the scene below), including the
   rollback when a pushed scene's `on_enter` throws. Tests: a new "scenes_manager flags"
   suite (flag matrix, draw order, independent walks, deep walks, event routing, pause /
   resume matrix, rollback).
4. Every application has a scene: `create_initial_scene()` is pure virtual; nullptr fails
   startup; `m_had_scenes` and the scene-free branches are gone; an empty stack quits (also
   when the first scene's `on_enter` throws). `update_scene` / `render_scene` are no longer
   hooks: private, non-virtual engine internals. Overridable hooks left: `ready`,
   `teardown`, `event`, `resize_scene`, the gamepad callbacks, `create_initial_scene`. The
   test harnesses install an idle scene; the tests that replaced update / render went.
5. `simplex`: `create_initial_scene()` installs `simplex::detail::simplex_scene`, whose
   `fixed_update` calls the adapter's `run_update` (bind the canvas, `on_update`, restore)
   and whose `render` draws the canvas. The adapter keeps the canvas and lifecycle in
   `ready()` / `teardown()`, so a failing `on_create` still fails startup. Returning false
   skips the frame's remaining ticks and quits. Callback and canvas-presentation failures
   are recorded by the scene (the engine would log and pop) and `SIMPLEX_MAIN` reports them
   as `SDL_APP_FAILURE` after the iteration (`rethrow_failure()`), instead of overriding
   `update_scene` / `render_scene`. **Behaviour change:** `on_update` gets the fixed tick
   (1/120 s), 0..N times per frame, edges on the first tick only — documented in
   `simplex/application.hh`, `NEUTRINO_PLAYGROUND_DESIGN.md` and the website's input / frame
   pages; `simplex_entry_tests` rewritten for it.
6. Tutorial 01's `is_opaque` comment → `flags()`.

Exit (met): full build; `neutrino_tests` 763/763, `simplex_entry_tests` 17/17 (stable over
15 runs), `luxor_unittest` 29/29, `game_format_tests` 7/7, `simplex_entry_smoke`,
`sdlpp_seal`.

Left: `website/docs/concepts/scenes.md` and `website/docs/tutorial/step-01-window-and-scene.md`
still describe `is_opaque()`.

### M2 — Sprite core (D1–D4, D7–D9, D12, D15–D18)

1. **Pivots (D9):** optional `pivot` on `image_bank::entry` (and `image_bank_builder::add`);
   carried into the atlas placement and the registry; `picture_pivot(address)` /
   `image_atlas::picture_pivot(i)`.
2. **`animation` + `animation_builder`:** one sequence (D15) behind a value handle
   (D16): frames, loop mode, cumulative frame times, pure `frame_at`. The builder appends
   frames one by one (`frame`) or as a run of consecutive pictures (`frames`), sets the
   loop mode, and `build()` validates (no frames, a zero-duration frame in a multi-frame
   sequence; frames from unregistered atlases are allowed — animations are built before
   upload). `animation::still(address)` is the one-frame shortcut. It holds plain
   addresses and owns no atlas (D12).
3. **Sprite pool per scene (D18):** `base_scene` owns an internal `sprite_pool` of sprite states; `spawn` returns a
   `sprite` handle; handles are movable, invalidated safely if the scene dies first (a
   generation-checked slot, not a dangling pointer).
4. **Advance:** `scenes_manager` advances a scene's pool immediately *before* calling its
   `fixed_update`, with the same `dt`, so the scene sees fresh `finished()` / `looped()`
   flags. Leftover time carries; speed scales `dt`; callbacks fire inside the advance.
5. **Playhead operations:** `play(animation)` (switch to any animation, restarting or
   keeping the phase), `restart`, `set_speed`, current frame, current animation.
6. Tests: `frame_at` boundaries and binary search; remainder carry over many ticks; several
   frames in one tick; once vs loop (a loop never finishes); `looped` / `frame_changed`
   flags; callbacks; speed 0 / 2; switch to another animation (restart and phase-keeping,
   the phase wrapped into the new animation's duration); switching to the animation
   already playing; empty animation is an ENFORCE; an animation copied into a game struct
   outliving the original; pause through `pauses_below` (sprites of a paused scene do not
   advance); handle outliving its scene; animation outliving every sprite.

Exit: the new core is complete and tested in isolation; nothing uses it yet.

### M3 — Painting sprites (needs RM1)

Everything about placing a picture — pixel snapping, scale about the pivot, flip mirroring
the pivot, diagonal flip, rotation, tint, crop, fit modes, error policy — is the painter's
(RM1). What is left here:

1. `paint_list::paint(position, const animation_frame&, …)`: the frame's picture with its
   pivot override and flip, forwarded to the painter's quad.
2. `paint_list::paint(position, const sprite&, …)` = `paint(position, s.frame(), …)`.
3. Tests: frame pivot override and frame flip combined with `quad_params`; a sprite paints
   its current frame after an advance.

Exit: a sprite can be painted anywhere a quad can.

### M4 — Port the users

In this order; each step builds and runs on its own.

1. **KE** (the most demanding user, validates the core):
   - sprite sets → `animation`s built from its own BOB atlas (its decoder already returns
     a bank; BOB offsets become picture pivots, removing the hand centring);
   - effects and capsules: engine sprites (were `sprite_state_id`), one animation each;
   - frames picked from game state (paddle by state/size, ball, bricks, digits) stay
     index-based: paint the `atlas_address`, no animation;
   - enemy alive → death is `play(death)`; the shield keeps its shared phase through
     `keep_phase` or the scene clock;
   - paddle, enemies, shield, deaths: from 70 Hz tick counters to sprites with durations
     in ms, if the timing matches (check frame-exactness against the original at 70 Hz;
     keep the tick counter where it does not);
   - painting goes to its screen-space layer;
   - delete the manual `unregister_*` teardown and the private `sprite_cache` member;
   - the sprite gallery scene moves too.
2. **World renderer:** `tileset_bundle` = `atlas_id` + one `atlas_address` and pivot per
   tile + one `animation` per animated tile; animated tiles use `frame_at(scene clock +
   phase(cell))` (D10) — the renderer needs the owning scene's elapsed time; seam-free
   orthogonal tiles use the painter's exact-integer `paint_picture`; rotated / diagonal /
   hex / iso tiles use the painter's quad. `resource_cache` stays as the world's cache,
   holding `atlas_id`s.
3. **`sprite_demo`, tutorials 02 / 03, drawing 02 / 03:** onto the PNG loader (M5 step 2 is
   needed first, or temporarily build the animations by hand).
4. **Luxor:** it builds defs but draws nothing; port the loader and its tests. Its `.spr`
   animation list (e.g. a button's normal / hover / pressed / disabled) becomes one
   animation per entry, which is what the parser already describes.

### M5 — Asset layer (D13)

1. `atlas_cache`: `acquire(name, builder) → atlas_id` (built lazily on first use),
   `reload(name)` (replacement in place under the same id); keyed by the game's name. No
   release (D12).
2. PNG convenience loader: image + named rects (+ pivots) + named sequences → its atlas
   plus one `animation` per sequence, written into the game's slots through a
   `{name → animation&}` mapping (unknown and missing names are reported); replaces
   `sprite_def_builder::from_file … add_visual … add_clip`. No string survives the load.
3. Tutorials and `sprite_demo` finish their move onto it.

### M6 — Delete the old layer, update the docs

1. Delete: `sprite_sheet`, `sprite_visual` / `_id` / `_ref`, `sprite_appearance`,
   `sprite_animation` (+ id), `sprite_state` (+ id), `sprites_manager` (and its
   `service_locator` slot, `require_sprites_manager` / `maybe_sprites_manager`),
   `resource_registry`, `render_bundle`, the old `sprite_def` / `sprite_def_builder` /
   `sprite_set` / `sprite_cache` / `sprite_set_handle` / `sprite_instance`, `atlas_loader`
   (Aseprite), `image_identity` if unused, the `application::on_update` sprite tick, and
   their tests.
2. Docs: `images_atlases_sprites.md` (§3.5–3.9, §4, §5 rewritten for the new layer; §9
   becomes the reference), the `sprites.hh` vocabulary section, the tutorials' text,
   `sprite_asset_pipeline.md` (obsolete: delete or mark superseded),
   `world_sprite_integration.md`, the roadmap.

Exit: the old API is gone; `video/sprite/` contains only the new layer.

### M7 — Later, on demand

- **warlords** as the first new user: `atlas_cache` for its sets, the painter's
  `paint_picture` with fit modes, fonts through the painter's `font` (`render_model.md`
  §9), Nuklear as a geometry producer (`render_model.md` RM8, postponed), keeping CPU pixels for the
  minimap (or computing it before upload).
- ping-pong loops, per-frame user tags, trimmed atlases, relative durations,
  `make_bitmask` collision on sprites (P3.3).

## 5. Risks and open points

| Risk / question | Plan |
|---|---|
| KE animations driven by 70 Hz ticks may not be frame-exact in ms on a 120 Hz tick | Compare frame sequences in a test; KE may set its fixed period to 1/70 s, or keep the counter for those animations |
| `sprite` handles outliving their scene | Generation-checked slots; a dead handle reports `finished()` and paints nothing |
| A game that generates atlases per session ("new game" loop) accumulates them | Accepted for now (D12); group release is the painter's later option (`render_model.md` §8.2) |
| `play(a)` accepts any animation, so `hero.play(coin_spin)` compiles | Accepted (D15): switching across animations is a feature (brix's shared explosion); the game's typed struct of animations is the guard |
| `animation` may later be wanted for tweens or skeletal animation | Accepted (D17); a tween system would take another name |
| Simplex `on_update` changes from frame delta to fixed tick | Done in M1: documented, `simplex_entry_tests` rewritten |
| The world renderer needs its scene's clock | Pass elapsed scene time into `world_renderer::render` (or let the renderer read the current scene's clock) — decide in M4.2 |
| Mesh effects (`render_model.md` §6) need a clock too | Same answer as sprites: the owning scene's time |
| Ordering: sprites advance before or after the scene's `fixed_update` | Before (M2.4), so polling sees this tick's events; revisit if a game needs the reverse |
