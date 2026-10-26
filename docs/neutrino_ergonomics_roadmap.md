# Roadmap: Ergonomic & Holistic Neutrino Engine

> **Mission**: Transform Neutrino from a collection of loosely coupled helper libraries (`sdlpp`, `euler`, `musac`, raw SDL) into a unified, ergonomic, and holistic 2D game engine. Game developers building with Neutrino should only need `#include <neutrino/neutrino.hh>` and should never have to manually convert between disparate math, color, or input types.

---

## 1. Executive Summary & Problem Diagnosis

Neutrino possesses excellent low-level engine capabilities: deterministic fixed-timestep simulation, continuous swept SAT collision, dynamic AABB broadphase trees, GPU sprite caching with atlas packing, and multi-channel audio streaming.

However, client code currently feels glued together because the seams of underlying libraries leak across the public boundary:

```
┌────────────────────────────────────────────────────────────────────────┐
│                          Current Client Code                           │
└────────┬───────────────────┬───────────────────┬───────────────────────┘
         │                   │                   │
         ▼                   ▼                   ▼
    sdlpp types         euler types         neutrino types
  • sdlpp::color      • euler::vec2       • world_pos / render_pos
  • sdlpp::scancode   • physics::aabb     • draw_* free functions
  • sdlpp::event      • method accessors  • base_scene lifecycle
  • SDLPP_MAIN          (.x(), .y())      • member accessors (.x, .y)
```

### Specific Friction Points

1. **The Math & Geometry Fragmentation**:
   * **Screen space**: `neutrino::point`, `rect`, `circle` (aliases of `sdlpp::point<int>`, member access `.x`, `.y`).
   * **World space**: `neutrino::world_point`, `world_rect` (aliases of `sdlpp::point<float>`).
   * **Physics geometry**: `neutrino::physics::vec` (`euler::vec2<float>`, method access `.x()`, `.y()`), `physics::aabb` (min/max vec), `physics::circle`.
   * **Strong coordinate types**: `world_pos`, `world_delta`, `world_velocity` in `coordinates.hh`.
   * *Impact*: Passing an actor's position to drawing requires `static_cast<int>` or explicit `to_world_point()`. Physics shapes cannot be debug-rendered without manually destructuring each shape into primitive draw calls.

2. **Leaky Dependencies in Public API**:
   * Colors require `#include <sdlpp/video/color.hh>` and `sdlpp::color{...}` or `sdlpp::colors::black`.
   * Input queries require `#include <sdlpp/events/keyboard_codes.hh>` and `in.pressed(hotkey{sdlpp::scancode::escape})`.
   * Application startup requires `#include <sdlpp/app/entry_point.hh>` and the `SDLPP_MAIN` macro.
   * Event handling forces every `base_scene` to implement pure virtual `handle_action(const sdlpp::event&)`.
   * Drawing calls return `sdlpp::expected<void, std::string>`, a result no caller checks (0 of 147 call sites in games, demos, examples and tutorials) and that also leaks the `sdlpp::` spelling.

3. **Subsystem Islands**:
   * Physics and visual sprites are disconnected. Visual frames authored in `sprite_def_builder` cannot declare hitboxes, and physics colliders have no built-in visual debug rendering.
   * Screen clearing has no native function (`neutrino::clear(...)` is missing, forcing users to call `get_renderer().clear()`).

4. **Header Sprawl**:
   * Writing a basic scene in `tutorials/01_window_and_scene` requires **7 separate `#include`s** across both `neutrino` and `sdlpp`.

---

## 2. Design Principles

1. **Zero Mandatory Dependency Leakage**: Client code should never need to include `<sdlpp/...>` or `<euler/...>` for standard gameplay, graphics, input, or audio.
2. **One Front Door**: `#include <neutrino/neutrino.hh>` gives you everything needed to build a 2D game.
3. **Float-First Gameplay & Graphics**: Gameplay math lives in floating-point world units. Drawing functions accept float render-space positions natively, rounding to pixels only at the final submission boundary; world positions are drawn through a camera.
4. **Explicit Space Crossings, Native Floats**: Same-space calls accept floats directly; crossings between world, render and physics spaces stay explicit (camera or named helper), never implicit conversions.
5. **Integrated Subsystems**: Physics, sprites, and rendering must cross-connect: built-in debug drawing for colliders, hitbox definitions in sprite defs, and scene-level audio/transition helpers.

---

## 3. Phased Roadmap

```mermaid
flowchart TD
    P1["Phase 1: Sealing the Perimeter<br/>(Drop third-party leaks, umbrella headers, entry point)"]
    P2["Phase 2: Typed Coordinate Ergonomics<br/>(Float render overloads, camera-aware world draw, physics helpers)"]
    P3["Phase 3: Subsystem Integration<br/>(Physics debug draw, sprite hitboxes, screen clearing)"]
    P4["Phase 4: Scene Ergonomics & Migration<br/>(Default scene stubs, tutorial & game migration)"]

    P1 --> P2
    P2 --> P3
    P3 --> P4
```

---

### Phase 1: Sealing the Perimeter (Quick Wins, High Impact)

**Goal**: Eradicate all mandatory `sdlpp` headers and syntax from client game code.

#### 1.1 First-Class Engine Colors (`neutrino::color`)
* **Location**: `<neutrino/video/color.hh>` (and re-exported by `<neutrino/neutrino.hh>`).
* **Implementation**:
  ```cpp
  namespace neutrino {
      using color = sdlpp::color;
      using fcolor = sdlpp::fcolor;

      namespace colors {
          inline constexpr color transparent{0, 0, 0, 0};
          inline constexpr color black{0, 0, 0, 255};
          inline constexpr color white{255, 255, 255, 255};
          inline constexpr color red{255, 0, 0, 255};
          inline constexpr color green{0, 255, 0, 255};
          inline constexpr color blue{0, 0, 255, 255};
          inline constexpr color yellow{255, 255, 0, 255};
          inline constexpr color cyan{0, 255, 255, 255};
          inline constexpr color magenta{255, 0, 255, 255};
          inline constexpr color dark_gray{64, 64, 64, 255};
          inline constexpr color gray{128, 128, 128, 255};
          inline constexpr color light_gray{192, 192, 192, 255};
          inline constexpr color cornflower_blue{100, 149, 237, 255};
      }
  }
  ```
* **Client Impact**: Clients write `neutrino::colors::white` and `neutrino::color{r, g, b, a}` without including `sdlpp/video/color.hh`.

#### 1.2 Native Input Enums & Fluent Queries
* **Location**: `<neutrino/input/keys.hh>` and `<neutrino/input/input_snapshot.hh>`.
* **Implementation**:
  * Provide `neutrino::key` (aliasing `sdlpp::scancode` for physical layout-independent keys) and `neutrino::mouse_button`.
  * Add direct convenience queries on `input_snapshot`:
    ```cpp
    // Direct key queries without wrapping in hotkey{}
    [[nodiscard]] bool key_pressed(key k) const noexcept;
    [[nodiscard]] bool key_held(key k) const noexcept;
    [[nodiscard]] bool key_released(key k) const noexcept;

    // Overloaded pressed/held/released
    [[nodiscard]] bool pressed(key k) const noexcept { return key_pressed(k); }
    [[nodiscard]] bool held(key k) const noexcept { return key_held(k); }
    [[nodiscard]] bool released(key k) const noexcept { return key_released(k); }

    // Direct mouse queries
    [[nodiscard]] bool mouse_pressed(mouse_button b) const noexcept;
    [[nodiscard]] bool mouse_held(mouse_button b) const noexcept;
    [[nodiscard]] bool mouse_down(mouse_button b) const noexcept { return mouse_held(b); }
    [[nodiscard]] render_pos mouse_pos() const noexcept { return pointer().render; }
    ```
* **Client Impact**:
  * Old: `in.pressed(neutrino::hotkey{sdlpp::scancode::escape})`
  * New: `in.pressed(neutrino::key::escape)` or `in.key_pressed(key::escape)`

#### 1.3 Engine Entry Point Macro (`NEUTRINO_MAIN`)
* **Location**: `<neutrino/entry_point.hh>`.
* **Implementation**:
  ```cpp
  #pragma once
  #include <sdlpp/app/entry_point.hh>

  #define NEUTRINO_MAIN(AppClass) SDLPP_MAIN(AppClass)
  ```
* **Client Impact**: Tutorial and game entry files include `<neutrino/entry_point.hh>` and invoke `NEUTRINO_MAIN(MyGame)`. No mention of `sdlpp`.

#### 1.4 Default Stubs in `base_scene`
* Make `handle_action` and `is_opaque` non-pure with sensible defaults (ignore raw events; opaque). Details in §4.1.

#### 1.5 The Umbrella "Front Door" (`<neutrino/neutrino.hh>`)
* **Location**: `include/neutrino/neutrino.hh`.
* **Contents**:
  * Core application: `<neutrino/application.hh>`, `<neutrino/scene/base_scene.hh>`, `<neutrino/scene/scene_transitions.hh>`
  * Geometry & types: `<neutrino/video/geometry_types.hh>`, `<neutrino/video/color.hh>`, `<neutrino/coordinates.hh>`
  * Graphics & sprites: `<neutrino/video/draw.hh>`, `<neutrino/video/sprites.hh>`, `<neutrino/video/globals.hh>`
  * Input: `<neutrino/input/input_snapshot.hh>`, `<neutrino/input/keys.hh>`
  * Audio: `<neutrino/audio/audio.hh>`
* **Client Impact**: An entire game scene can be written with a single `#include <neutrino/neutrino.hh>`.

#### 1.6 Draw Calls Return `void`
* **Decision**: drawing is fire-and-forget. Draw primitives and `draw_sprite` return `void`; state getters return their value directly. This removes every `expected` from neutrino's public API, so no `neutrino::expected` alias is needed. (An alias was the original plan; it was dropped once it turned out `draw.hh` is the only public header that returns `expected`.)
* **Why `expected` buys nothing here**:
  * **No caller uses it.** 0 of 147 draw calls in games, demos, examples and tutorials look at the result. The only consumers are three engine-internal call sites in `world_renderer.cc` that bump `stats.failed`, which tests assert is 0 and `map_viewer` prints.
  * **Programmer errors never reach it.** A bad scale or rotation already fails `ENFORCE`; an invalid sprite reference or empty destination is a silent no-op.
  * **What remains is SDL's own return value** from `SDL_RenderLine`, `SDL_RenderFillRect`, `SDL_RenderGeometry`, etc. In SDL3 these fail only on a broken renderer, a texture from another renderer, or out-of-memory while queuing. None of these can be handled per call, mid-frame.
  * **It is unenforced.** The `void`-valued draws aren't `[[nodiscard]]`, so results are silently dropped; adding `[[nodiscard]]` would only spread `(void)` casts across 147 call sites.
  * **Getters wrap engine-owned state.** `get_draw_color()`, `get_draw_blend_mode()` and `get_line_style()` return `expected` for state the engine itself set.
* **New contract** (`include/neutrino/video/draw.hh`):
  ```cpp
  NEUTRINO_EXPORT void draw_line(const point& a, const point& b, const color& c);
  NEUTRINO_EXPORT void draw_sprite(const point& position, sprite_state_id state,
                                   const sprite_draw_params& params = {});
  NEUTRINO_EXPORT void set_draw_color(const color& c);
  NEUTRINO_EXPORT [[nodiscard]] color get_draw_color();
  ```
* **Failures stay visible, not silent**: when SDL reports an error, the engine logs it, rate-limited per call site (the 1st, 1001st, ... failure of each `DRAW_CHECK` site, logged with that site's line), and increments `draw_failure_count()`, a running total since startup. Tests and tools compare the count before and after a span of draws; `world_renderer`'s `stats.failed` does exactly that per tile. (Per-call-site limiting and a running total replaced the earlier "first failure per frame" / per-frame counter idea: neither needs a frame-start reset hook in `draw.cc`.)
* **Scope of `expected` vs. other error channels** (unchanged, now documented as policy):
  * Draw calls: `void` + log/counter (this item).
  * Loading images: throws (`image_loader.hh`); a missing asset is fatal for the caller.
  * Optional resources: `std::optional` (`render_texture::create`).
  * If a future public API has a failure the caller can genuinely act on, introduce `<neutrino/expected.hh>` (aliasing `failsafe::expected`) at that point, not before.
* **Client impact**: none for normal code. A call written as a statement compiles unchanged when its return type becomes `void`; only code that inspects the result (`.has_value()`, `return draw_...(...)`) must change, and none exists outside `world_renderer.cc`. Phase 2's float `render_pos` overloads (§2.1) are specified as `void` already and stay that way.

---

### Phase 2: Typed Coordinate Ergonomics

**Goal**: Remove the casting noise at the draw and physics boundaries *without* collapsing the strong coordinate vocabulary. `coordinates.hh` deliberately separates `world_pos` / `world_delta` / `world_velocity`, and `render_pos` / `window_pos` exist so that mixing spaces is a compile error. Phase 2 keeps that: every crossing between spaces stays explicit, but becomes short, and every same-space call accepts floats natively.

> **Rejected: a universal `vec2<T>` adapter (former Option A).** A struct cannot declare both a data member `x` and a member function `x()`, so the "dual accessor" design does not compile. Bidirectional implicit conversions between `vec2`, `euler::vector`, `euler::point2` and `sdlpp::point` cause overload ambiguity in the existing `draw_*` sets, and re-aliasing `point` / `rect` breaks every sdlpp API taking `sdlpp::point<int>`. Most importantly, an implicit `world_pos → vec2f` conversion plus a `draw_sprite(world_pos)` overload would silently treat world coordinates as render coordinates (no camera applied) -- exactly the bug class the strong types were introduced to prevent. The measured friction is small (KE has 7 `to_world_point` calls, all feeding `sprite_batch::add`), so it is fixed at those boundaries instead.

#### 2.1 Float Render-Space Drawing Overloads
* **Location**: `<neutrino/video/draw.hh>`, `src/neutrino/video/draw.cc`.
* Add overloads of the immediate-mode primitives and `draw_sprite` that take float **render-space** coordinates:
  ```cpp
  // render_pos already exists in coordinates.hh: "what scenes draw in".
  void draw_point(render_pos p, const color& c);
  void draw_line(render_pos a, render_pos b, const color& c);
  void draw_rect(const render_rect& r, const color& c);       // render_rect: float {x, y, w, h}
  void draw_rect_fill(const render_rect& r, const color& c);
  void draw_circle(render_pos centre, float radius, const color& c);
  void draw_circle_fill(render_pos centre, float radius, const color& c);

  void draw_sprite(render_pos position, sprite_state_id state, const sprite_draw_params& params = {});
  void draw_sprite(render_pos position, sprite_visual_ref visual, const sprite_draw_params& params = {});
  void draw_sprite(render_pos position, const sprite_appearance& a, const sprite_draw_params& params = {});
  ```
* **Rounding happens once**, at submission (`std::lround`), in one helper shared by all overloads. Game code never writes `static_cast<int>(x)`.
* Brace-init stays unambiguous: `draw_sprite({x, y}, ...)` must keep resolving to one overload -- add a compile test for it, and prefer `render_pos` over a second untyped float point to avoid an `int`/`float` brace ambiguity.
* Integer `point` overloads remain; nothing is removed.

#### 2.2 Camera-Aware World Drawing
* World positions reach the screen only through a camera. Provide that path explicitly instead of an implicit conversion:
  ```cpp
  // <neutrino/video/world/camera.hh>
  [[nodiscard]] render_pos to_render(const camera& cam, dim viewport, world_pos p) noexcept;

  // <neutrino/video/world/sprite_batch.hh> -- the call site behind every KE to_world_point()
  // world_pos mirror of each existing add(world_point, ...) overload, e.g.:
  void add(world_pos pos, draw_layer layer, float depth, sprite_state_id state, sprite_draw_params params = {});
  ```
* `sprite_batch::add(world_pos, ...)` removes all current `to_world_point()` calls in `games/ke/scenes/play_game_scene.cc`.
* The existing `to_world_point()` / `to_world_pos()` helpers stay as the explicit escape hatch; there is **no** implicit `world_pos ↔ world_point` conversion and **no** `draw_*(world_pos)` overload without a camera.

#### 2.3 Physics Boundary Conversions
* `physics::vec` stays `euler::vec2f` internally; the physics `world` API already speaks `world_pos` (`move_to`, `position_of`).
* For the remaining shape-construction sites (e.g. KE `mechanics.cc` building `aabb` / `circle` from floats and `world_velocity`), add named, constexpr helpers in `<neutrino/physics/geometry/types.hh>`:
  ```cpp
  [[nodiscard]] constexpr vec to_vec(world_pos p) noexcept;
  [[nodiscard]] constexpr vec to_vec(world_delta d) noexcept;
  [[nodiscard]] constexpr vec to_vec(world_velocity v) noexcept;
  [[nodiscard]] constexpr world_pos to_world_pos(vec v) noexcept;
  [[nodiscard]] constexpr aabb to_aabb(world_bounds b) noexcept;
  ```
* No change to `.x()` vs `.x` access: physics internals keep euler syntax, gameplay code keeps the strong types, and the two meet only through these helpers.

---

### Phase 3: Subsystem Integration (Bridging the Islands)

**Goal**: Connect graphics, physics, and screen management into a cohesive ecosystem.

#### 3.1 First-Class Screen & Canvas Operations
* Add global screen operations to `draw.hh` / `globals.hh`:
  ```cpp
  /// @brief Clear the active render target / screen with color (defaults to black).
  void clear(const color& c = colors::black);

  /// @brief Clear the screen with a specific background color and reset the viewport.
  void clear_screen(const color& c);
  ```
* Automatically works whether rendering to the window, an offscreen `render_texture`, or a `simplex::application` canvas.

#### 3.2 Physics Debug Rendering
* **Location**: `<neutrino/physics/debug_draw.hh>`.
* **API**:
  ```cpp
  namespace neutrino::physics {
      /// Draw an individual collision shape (AABB, circle, segment, triangle)
      void debug_draw(const aabb& box, const color& c = colors::green);
      void debug_draw(const circle& circ, const color& c = colors::green);
      void debug_draw(const segment& seg, const color& c = colors::green);
      void debug_draw(const triangle& tri, const color& c = colors::green);
      void debug_draw(const shape_t& shape, const color& c = colors::green);

      /// Draw all active colliders, sensors, and velocities in the physics world
      void debug_draw(const world& phys_world, const camera& cam,
                      const debug_draw_options& options = {});
  }
  ```
* **Impact**: Eliminates hundreds of lines of bespoke collider-visualization code across user games and tests.

#### 3.3 Authoring Colliders on Visual Sprites
* Allow defining frame hitboxes directly inside `sprite_def_builder`:
  ```cpp
  auto def = sprite_def_builder{}
      .from_file("player.png", 320, 240)
      .add_visual("player.idle", rect{0, 0, 32, 32}, point{16, 32})
      .add_collider("player.idle", aabb{ {-10.0f, -32.0f}, {10.0f, 0.0f} }) // relative to pivot!
      .build();
  ```
* Expose `sprite_metrics::collider()` so games can automatically configure physics bodies from sprite metadata.

---

### Phase 4: Scene Ergonomics & Migration

**Goal**: Remove unnecessary ceremony and migrate all tutorials, games, and tests.

#### 4.1 Streamlining `base_scene`
* Provide default implementations for pure virtual stubs:
  ```cpp
  class base_scene {
  public:
      virtual ~base_scene() = default;

      // Pure virtual hooks that scenes MUST implement:
      virtual void fixed_update(sim_duration dt, const input_snapshot& in) = 0;
      virtual void render() = 0;

      // Sensible defaults (no longer pure virtual):
      virtual void handle_action(const sdlpp::event&) {} // Default: ignore raw SDL events
      [[nodiscard]] virtual bool is_opaque() const { return true; } // Default: opaque
  };
  ```
* Provide scene-level convenience methods:
  ```cpp
  void pop_this_scene();
  void push_next_scene(std::unique_ptr<base_scene> scene);
  dim scene_render_size() const;
  ```

#### 4.2 Migration of Tutorials and Games
1. Update `tutorials/01_window_and_scene`:
   * Replace 7 `#include`s with `#include <neutrino/neutrino.hh>`.
   * Replace `SDLPP_MAIN` with `NEUTRINO_MAIN`.
   * Delete empty `handle_action` stub.
   * Use `neutrino::colors::*` and `in.pressed(key::escape)`.
2. Update `tutorials/02_one_sprite`:
   * Remove `static_cast<int>(m_player_x)` from `draw_sprite`.
   * Pass float coordinates directly.
3. Update `games/ke` (`krypton_egg`):
   * Replace `to_world_point()` calls with `sprite_batch::add(world_pos, ...)`.
   * Adopt `neutrino::clear(...)` and direct physics debug drawing.

---

## 4. Work Breakdown & Effort Matrix

| Phase | Task | Effort | Files Affected | Breaking Change? |
| :--- | :--- | :---: | :--- | :---: |
| **P1.1** | `neutrino::color` & `colors::*` | **S** (½ day) | `include/neutrino/video/color.hh`, `draw.hh` | No (type alias) |
| **P1.2** | `key::*` enum & fluent `input_snapshot` queries | **S** (½ day) | `include/neutrino/input/keys.hh`, `input_snapshot.hh` | No (additive) |
| **P1.3** | `NEUTRINO_MAIN` macro | **S** (1 hr) | `include/neutrino/entry_point.hh` | No (additive) |
| **P1.4** | Default stubs in `base_scene` | **S** (1 hr) | `include/neutrino/scene/base_scene.hh` | No (additive) |
| **P1.5** | Umbrella header `<neutrino/neutrino.hh>` | **S** (2 hrs) | `include/neutrino/neutrino.hh` | No (additive) |
| **P1.6** | Draw calls return `void`; getters return values; failures logged + counted | **S** (½ day) | `include/neutrino/video/draw.hh`, `src/neutrino/video/draw.cc`, `video/world/world_renderer.*` | No for callers that ignore results (all client code) |
| **P2.1** | Float `render_pos` drawing overloads | **M** (1 day) | `include/neutrino/video/draw.hh`, `src/neutrino/video/draw.cc` | No (overloads) |
| **P2.2** | Camera-aware world drawing (`to_render`, `sprite_batch::add(world_pos)`) | **S** (½ day) | `video/world/camera.hh`, `video/world/sprite_batch.hh` | No (additive) |
| **P2.3** | Physics boundary helpers (`to_vec`, `to_aabb`) | **S** (2 hrs) | `include/neutrino/physics/geometry/types.hh` | No (additive) |
| **P3.1** | Screen clear functions (`neutrino::clear`) | **S** (2 hrs) | `include/neutrino/video/draw.hh`, `globals.hh` | No (additive) |
| **P3.2** | Physics debug renderer | **M** (2 days) | `include/neutrino/physics/debug_draw.hh`, `src/...` | No (additive) |
| **P3.3** | Sprite def hitbox integration | **M** (2 days) | `sprite_def_builder.hh`, `sprite_sheet.hh` | No (additive) |
| **P4.1** | Migrate tutorials 01–03 & simplex | **S** (½ day) | `tutorials/*` | No |
| **P4.2** | Migrate `games/ke` and demos | **M** (1 day) | `games/ke/*`, `demos/*` | No |

*Total Estimated Effort*: ~1.5 to 2 weeks of focused implementation.

---

## 5. Verification & Testing Strategy

1. **Unit Testing**:
   * Test float draw overloads round once at submission, and that `draw_sprite({x, y}, ...)` brace-init resolves unambiguously.
   * Static-assert that `world_pos` does **not** implicitly convert to `world_point`, `render_pos` or `physics::vec`.
   * Test `input_snapshot` fluent methods against synthetic input events.
   * Test `neutrino::clear(color)` on both window renderer and offscreen `render_texture`.
2. **Headless & Display Regression Testing**:
   * Ensure `cmake-build-release/bin/neutrino_tests` (623 cases / 293,523 assertions) remains 100% green at every step.
   * Verify all existing games (`games/ke`, `games/luxor`) compile and run without regression.
3. **Ergonomic Audit ("The Blank File Test")**:
   * Create a new test tutorial file from scratch using only `#include <neutrino/neutrino.hh>` and `NEUTRINO_MAIN`.
   * Verify that not a single `sdlpp` or `euler` identifier is required to open a window, update game state, play sound, and render sprites.
