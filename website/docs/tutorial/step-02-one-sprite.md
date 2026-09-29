---
sidebar_position: 2
title: "Step 2: One sprite on screen"
description: Load a spritesheet, define visual frames and animation clips, and render an animated character standing on the ground.
---

# Step 2: One sprite on screen

In [Step 1: A window and an empty scene](./step-01-window-and-scene.md), we created our platformer application with a sky-blue background and a ground strip.

In this step, we bring our platformer character into the world: loading the spritesheet, defining animation frames, and rendering our hero standing on the ground.

By the end of this step, you will have an animated character standing naturally on the ground, breathing with an idle animation, and ready for player movement controls in Step 3.

:::tip[Concept pages]
For the deep architectural reasoning behind Neutrino's sprite system and coordinate transformations, see:

- [Sprites](../concepts/sprites.md) — pure data definitions (`sprite_def`), GPU atlases (`sprite_set`), and automatic cache leasing (`sprite_cache`)
- [Coordinate spaces](../concepts/coordinate-spaces.md) — logical render coordinates and pivot placement

:::

---

## What we are building

In Neutrino, rendering a 2D animated character involves three decoupled constructs:

1. **`sprite_def` (Pure CPU Data):** describes the asset on disk, frame rectangles, pivot origins, and animation sequences with zero GPU dependency.
2. **`sprite_cache` & `sprite_set_handle` (GPU Resources & Leasing):** uploads the texture atlas to the GPU and issues an RAII lease guaranteeing the texture remains in VRAM while needed.
3. **`sprite_instance` (Per-Entity Playhead):** holds the runtime animation clock and frame state for a specific character in the scene.

Here is the complete source code for Step 2, located in `tutorials/02_one_sprite/main.cc`:

```cpp
#include <neutrino/application.hh>
#include <neutrino/input/hotkey.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/scene/scene_transitions.hh>
#include <neutrino/video/draw.hh>
#include <neutrino/video/sprites.hh>

#include <sdlpp/app/entry_point.hh>
#include <sdlpp/video/color.hh>

#include <chrono>

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

using namespace std::chrono_literals;

namespace {

    // Logical design resolution for our platformer.
    constexpr int logical_width = 640;
    constexpr int logical_height = 360;

    // Window dimensions at startup.
    constexpr int window_width = 1280;
    constexpr int window_height = 720;

    // Visual scale factor for pixel-art sprites.
    constexpr float sprite_scale = 2.0f;

    // Helper to resolve asset paths across build and run environments.
    [[nodiscard]] std::filesystem::path asset_path(const std::string& filename) {
#ifdef NEUTRINO_TUTORIAL_ASSET_DIR
        auto p = std::filesystem::path{NEUTRINO_TUTORIAL_ASSET_DIR} / filename;
        if (std::filesystem::exists(p)) {
            return p;
        }
#endif
        if (std::filesystem::exists("tutorials/assets/" + filename)) {
            return "tutorials/assets/" + filename;
        }
        if (std::filesystem::exists("assets/" + filename)) {
            return "assets/" + filename;
        }
        return filename;
    }

    // =========================================================================
    // Pure Asset Definition (CPU Data): make_player_def
    // =========================================================================
    [[nodiscard]] neutrino::sprite_def make_player_def() {
        return neutrino::sprite_def_builder{}
            // 1. The Atlas Image
            // Point to the spritesheet on disk with declared atlas dimensions.
            .from_file(asset_path("arcade_platformer.png"), 352, 320)

            // 2. Named Visuals (Frame Rectangles and Pivots)
            // Each frame occupies a 32x32 cell in the atlas.
            // We set origin = {16, 32} (bottom-center pivot).
            // By placing the pivot at the bottom center of the sprite, placing the sprite
            // at (x, ground_y) rests the character's feet directly on the ground.
            .add_visual("player.idle",   neutrino::rect{0, 0, 32, 32},  neutrino::point{16, 32})
            .add_visual("player.walk.0", neutrino::rect{0, 0, 32, 32},  neutrino::point{16, 32})
            .add_visual("player.walk.1", neutrino::rect{32, 0, 32, 32}, neutrino::point{16, 32})
            .add_visual("player.walk.2", neutrino::rect{64, 0, 32, 32}, neutrino::point{16, 32})

            // 3. Named Animation Clips
            // Clips bind visual frames into timed sequences.
            .add_clip("idle", {"player.idle"}, 1000ms, true)
            .add_clip("walk", {"player.walk.0", "player.walk.1", "player.walk.2", "player.walk.1"}, 100ms, true)
            .build();
    }

    // =========================================================================
    // The Scene: main_scene
    // =========================================================================
    class main_scene final : public neutrino::base_scene {
    public:
        main_scene() = default;

        void on_enter() override {
            std::cout << "[Tutorial 02] Acquiring player sprite set from cache...\n";

            // 1. Acquire the GPU resources from the engine sprite cache.
            // If the set is already resident in GPU memory, acquire_sprite() shares it instantly;
            // if not, it decodes the image and uploads the atlas to the GPU.
            m_set = neutrino::acquire_sprite(make_player_def());

            // 2. Spawn a runtime playhead for our player actor.
            m_player = m_set.spawn("idle");
        }

        void fixed_update(neutrino::sim_duration, const neutrino::input_snapshot&) override {
            if (neutrino::hotkey{sdlpp::scancode::escape}.pressed()) {
                std::cout << "[Tutorial 02] Escape pressed - exiting\n";
                neutrino::pop_scene();
            }
        }

        void handle_action(const sdlpp::event&) override {
            // Discrete one-shot window and system events arrive here.
        }

        void render() override {
            // 1. Sky background
            neutrino::draw_rect_fill(
                neutrino::rect{0, 0, logical_width, logical_height},
                sdlpp::color{90, 160, 230, 255}
            );

            // 2. Ground strip
            constexpr int ground_y = 300;
            constexpr int ground_height = logical_height - ground_y;

            neutrino::draw_rect_fill(
                neutrino::rect{0, ground_y, logical_width, 10},
                sdlpp::color{80, 185, 95, 255}
            );
            neutrino::draw_rect_fill(
                neutrino::rect{0, ground_y + 10, logical_width, ground_height - 10},
                sdlpp::color{110, 75, 45, 255}
            );

            // 3. Draw the player sprite standing on the ground!
            if (m_player.valid()) {
                neutrino::draw_sprite(
                    neutrino::point{static_cast<int>(m_player_x), ground_y},
                    m_player.state(),
                    neutrino::sprite_draw_params{
                        .scale = sprite_scale,
                        .flip = neutrino::sprite_flip::none,
                        .rotation_degrees = 0.0f,
                    }
                );
            }
        }

        [[nodiscard]] bool is_opaque() const override {
            return true;
        }

    private:
        // The sprite cache is managed globally at the application level by neutrino::application.
        // Scenes only hold lightweight RAII handles and instances:
        // 1. m_set holds an RAII lease on the uploaded GPU sprite atlas.
        // 2. m_player is an active playhead bound to an animation clip.
        // Because the cache lives at the engine service level, scenes are free of fragile
        // member destruction ordering dependencies.
        neutrino::sprite_set_handle m_set;
        neutrino::sprite_instance   m_player;

        float m_player_x{120.0f};
    };

    // =========================================================================
    // The Application: tutorial_app
    // =========================================================================
    class tutorial_app final : public neutrino::application {
    public:
        tutorial_app()
            : neutrino::application(make_config()) {
        }

    protected:
        std::unique_ptr<neutrino::base_scene> create_initial_scene() override {
            return std::make_unique<main_scene>();
        }

    private:
        static neutrino::application_config make_config() {
            neutrino::application_config cfg;
            cfg.title = "Neutrino Tutorial 02 - One Sprite on Screen";
            cfg.width = window_width;
            cfg.height = window_height;
            cfg.flags = sdlpp::window_flags::resizable;
            cfg.logical_size = neutrino::dim{logical_width, logical_height};
            cfg.scale = neutrino::scale_mode::letterbox;
            return cfg;
        }
    };

} // namespace

SDLPP_MAIN(tutorial_app)
```

---

## Step-by-step walkthrough

### 1. Describing art fluently (`sprite_def_builder`)

Notice `make_player_def()`:
```cpp
return neutrino::sprite_def_builder{}
    .from_file(asset_path("arcade_platformer.png"), 352, 320)
    .add_visual("player.idle",   neutrino::rect{0, 0, 32, 32},  neutrino::point{16, 32})
    .add_visual("player.walk.0", neutrino::rect{0, 0, 32, 32},  neutrino::point{16, 32})
    .add_visual("player.walk.1", neutrino::rect{32, 0, 32, 32}, neutrino::point{16, 32})
    .add_visual("player.walk.2", neutrino::rect{64, 0, 32, 32}, neutrino::point{16, 32})
    .add_clip("idle", {"player.idle"}, 1000ms, true)
    .add_clip("walk", {"player.walk.0", "player.walk.1", "player.walk.2", "player.walk.1"}, 100ms, true)
    .build();
```

A `sprite_def` is a plain C++ data structure containing **no GPU state** (no OpenGL textures, no Vulkan handles).
You can construct it, serialize it, or test it anywhere — even in headless command-line tools without an active window.

The fluent `sprite_def_builder` simplifies configuring image sources, uniform grid slicing, custom frame rects, and animation clips. In addition to disk files (`.from_file()`), it can also load from in-memory byte buffers (`.from_memory()`, e.g. extracted from a PAK archive or resource bundle) and stream inputs (`.from_stream()`).

### 2. The Bottom-Center Pivot Trick (`origin = {16, 32}`)

In 2D platformers, setting sprite pivots to the top-left corner (`0, 0`) leads to tedious math: every time the character plays an animation with a slightly different height, their feet sink into the ground or float in the air.

```cpp
neutrino::sprite_visual_def{"player.idle", neutrino::rect{0, 0, 32, 32}, neutrino::point{16, 32}}
```

By placing the pivot at `(16, 32)` — the horizontal center and vertical bottom of the 32x32 frame — the sprite's anchor represents **the contact point between the character's feet and the floor**.

When drawing:
```cpp
neutrino::draw_sprite(
    neutrino::point{static_cast<int>(m_player_x), ground_y},
    m_player.state(),
    {.scale = sprite_scale}
);
```
Passing `y = ground_y` guarantees the feet rest directly on top of the grass, regardless of scale!

### 3. Acquiring via engine sprite cache in `on_enter()` (`neutrino::acquire_sprite`)

In `main_scene::on_enter()`, we acquire the asset through the engine sprite cache:

```cpp
m_set = neutrino::acquire_sprite(make_player_def());
```

`neutrino::application` owns a shared `sprite_cache` service. `acquire_sprite()` automatically computes a 64-bit content hash of the definition:
- If the texture atlas is already uploaded, `acquire_sprite()` shares the resident GPU texture instantly.
- If not, it uploads the texture to VRAM, registers the animation curves, and hands back an RAII lease (`sprite_set_handle`).

### 4. Spawning the player playhead (`sprite_instance`)

```cpp
m_player = m_set.spawn("idle");
```

`m_set.spawn("idle")` creates a [`sprite_instance`](pathname:///api/). It bundles:
1. An internal copy of the `sprite_set_handle` lease, keeping the texture resident in VRAM as long as the player exists.
2. A unique runtime playback state (`sprite_state_id`) with its own animation clock.

### 5. Automatic animation ticking

Notice what is missing from `fixed_update()`:

```cpp
void fixed_update(neutrino::sim_duration, const neutrino::input_snapshot&) override {
    // We do NOT need to call m_player.update(dt)!
}
```

In Neutrino, you do not manually pass delta time to your sprites.
The engine's main loop **automatically ticks all registered runtime sprite states** during `application::on_update`.
When `draw_sprite()` is called in `render()`, the engine automatically resolves the visual frame for the current point in time.

### 6. Scene member lifecycle and application-level cache

Take note of the private member declarations in `main_scene`:

```cpp
private:
    neutrino::sprite_set_handle m_set;     // RAII lease on the uploaded GPU atlas
    neutrino::sprite_instance   m_player;  // Active playhead actor
```

In Neutrino, `sprite_cache` is an engine-level service owned by `neutrino::application`, not individual scenes. This provides major architectural advantages:
1. **Asset Sharing Across Scenes:** If Scene A and Scene B use the same player sprites, transitioning between them does not re-upload or destroy the textures. Idle assets safely rest in the cache's cold pool.
2. **Order Independence:** Scenes do not own the cache, eliminating fragile member destruction ordering bugs. Even if `m_player` is declared before `m_set` or vice-versa, `sprite_instance` unregisters its playback state before releasing its internal lease, guaranteeing safe and clean teardown by construction.

---

## Building and running

To build Tutorial Step 2:

```bash
# 1. Configure the project with tutorials enabled:
cmake -B build -DNEUTRINO_NEUTRINO_BUILD_TUTORIALS=ON

# 2. Compile the Step 2 executable:
cmake --build build --target neutrino_tutorial_02_one_sprite

# 3. Run the tutorial:
./build/tutorials/02_one_sprite/neutrino_tutorial_02_one_sprite
```

You will see the 640x360 window open, with the blue sky, green grass ground, and our hero character standing on the ground, breathing with their idle animation.

Press <kbd>Escape</kbd> to exit cleanly.

---

## Next step

Now that our character is in the world, in **[Step 3: Moving it with the keyboard](./index.md)** we will wire up player controls:
- Inspecting the input snapshot in `fixed_update`
- Running left and right with <kbd>A</kbd> / <kbd>D</kbd> or arrow keys
- Flipping the sprite horizontally based on movement direction
- Switching between `"idle"` and `"walk"` animation clips
