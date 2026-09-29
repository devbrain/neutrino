---
sidebar_position: 1
title: "Step 1: A window and an empty scene"
description: Start building a 2D platformer by creating a window, configuring presentation scaling, and implementing your first scene.
---

# Step 1: A window and an empty scene

In this first step, we create the platformer executable from scratch: an opening window, a fixed simulation loop, and an active scene on the stack.

By the end of this step, you will have a running window showing a letterboxed design canvas, an active scene receiving simulation ticks, and a clean shutdown path on <kbd>Escape</kbd>.

:::tip[Concept pages]
This tutorial is the fast, hands-on path through the engine. For in-depth design rationale and edge cases, see:

- [The frame](../concepts/the-frame.md) — simulation vs. presentation, the accumulator, and fixed-timestep guarantees
- [Scenes](../concepts/scenes.md) — the scene stack, opaque vs. overlay, and lifecycle transitions

:::

---

## What we are building

Every game in Neutrino consists of two primary constructs:

1. **The Application (`neutrino::application`)**: owns the OS window, renderer context, display pacing, input sampling, and the scene stack.
2. **The Scene (`neutrino::base_scene`)**: represents a screen or mode of your game (gameplay, pause overlay, title screen). Scenes live on a stack managed by the engine.

Here is the complete source code for Step 1, located in `tutorials/01_window_and_scene/main.cc`:

```cpp
#include <neutrino/application.hh>
#include <neutrino/input/hotkey.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/scene/scene_transitions.hh>
#include <neutrino/video/draw.hh>

#include <sdlpp/app/entry_point.hh>
#include <sdlpp/video/color.hh>

#include <iostream>
#include <memory>

namespace {

    // Logical design resolution for our platformer.
    constexpr int logical_width = 640;
    constexpr int logical_height = 360;

    // Window dimensions at startup.
    constexpr int window_width = 1280;
    constexpr int window_height = 720;

    class main_scene final : public neutrino::base_scene {
    public:
        main_scene() = default;

        void on_enter() override {
            std::cout << "[Tutorial 01] main_scene::on_enter() - scene activated\n";
        }

        void on_exit() override {
            std::cout << "[Tutorial 01] main_scene::on_exit() - scene closing\n";
        }

        void on_resize(neutrino::dim size) override {
            std::cout << "[Tutorial 01] main_scene::on_resize() - size: "
                      << size.width << "x" << size.height << "\n";
        }

        void fixed_update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override {
            m_sim_ticks++;
            m_sim_time += dt.count();

            if (in.pressed(neutrino::hotkey{sdlpp::scancode::escape})) {
                std::cout << "[Tutorial 01] Escape pressed - popping scene to exit\n";
                neutrino::pop_scene();
            }
        }

        void render() override {
            // Draw a calm sky-blue background across the logical screen.
            neutrino::draw_rect_fill(
                neutrino::rect{0, 0, logical_width, logical_height},
                sdlpp::color{90, 160, 230, 255}
            );

            // Draw a placeholder ground strip.
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
        }

        void handle_action([[maybe_unused]] const sdlpp::event& ev) override {
            // Discrete one-shot events.
        }

        [[nodiscard]] bool is_opaque() const override {
            return true;
        }

    private:
        uint64_t m_sim_ticks{0};
        float m_sim_time{0.0f};
    };

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
            cfg.title = "Neutrino Tutorial 01: Window and Scene";
            cfg.width = window_width;
            cfg.height = window_height;
            cfg.flags = sdlpp::window_flags::resizable;

            // Design resolution and scale mode:
            cfg.logical_size = neutrino::dim{logical_width, logical_height};
            cfg.scale = neutrino::scale_mode::letterbox;

            return cfg;
        }
    };

} // namespace

SDLPP_MAIN(tutorial_app)
```

---

## Anatomy of the program

Let's dissect each part to understand why it is structured this way.

### 1. Window and presentation configuration

Neutrino passes an `application_config` struct to the `application` base constructor:

```cpp
neutrino::application_config cfg;
cfg.title = "Neutrino Tutorial 01: Window and Scene";
cfg.width = 1280;
cfg.height = 720;
cfg.flags = sdlpp::window_flags::resizable;

cfg.logical_size = neutrino::dim{640, 360};
cfg.scale = neutrino::scale_mode::letterbox;
```

#### Reflow vs. Logical presentation

There are two fundamental ways 2D games handle arbitrary display sizes:
- **Reflow mode** (default when `logical_size` is unset): The scene renders directly at native window pixels. When the window gets larger, you see more of the world. This is common in desktop applications and strategy games.
- **Logical presentation mode** (when `logical_size` is set): You pick a fixed virtual canvas (here, `640x360` — a standard 16:9 widescreen pixel-art resolution). All game positioning and drawing are expressed in this coordinate system. The engine automatically scales the virtual canvas to fit the window using `scale_mode::letterbox` (or `integer_scale`), adding crisp pillarboxes or letterboxes as needed.

### 2. The scene interface

`neutrino::base_scene` enforces four pure virtual methods:

| Method | Role |
|---|---|
| `fixed_update(dt, in)` | Fixed-timestep simulation logic. Physics, motion, and continuous input live here. |
| `render()` | Presentation rendering. Draws the current state. |
| `handle_action(ev)` | Discrete event dispatch (e.g. text entry, window events). |
| `is_opaque()` | Returns `true` if this scene covers the full display, allowing Neutrino to skip updating and rendering covered scenes. |

#### Why `fixed_update` receives a constant `dt`

Notice that `fixed_update` does not receive real frame deltas. In Neutrino, the simulation runs at a fixed frequency (default `120 Hz`, or `1/120 s ≈ 8.33 ms` per tick).

If the display runs at 60 Hz, the engine executes two substeps per frame. If a frame takes longer due to a background spike, the accumulator runs catch-up ticks. Because every tick advances time by the exact same amount, physics and platforming calculations are completely deterministic across different machines.

### 3. Transitions and quitting

```cpp
if (in.pressed(neutrino::hotkey{sdlpp::scancode::escape})) {
    neutrino::pop_scene();
}
```

The scene stack is manipulated through free functions like `neutrino::push_scene()` and `neutrino::pop_scene()`.

These calls do not mutate the stack immediately inline; they post an event to the SDL queue to take effect cleanly at the start of the next cycle. When `pop_scene()` pops the last active scene from the stack, the engine notices the empty stack and initiates a graceful application shutdown.

### 4. Application entry point

```cpp
SDLPP_MAIN(tutorial_app)
```

SDL3 requires managing platform-specific entry points (e.g. standard `main` on POSIX, `WinMain` on Windows, or custom lifecycle callbacks on Android/iOS/WebAssembly). `SDLPP_MAIN` handles this boilerplate transparently.

---

## Building and running

To build the tutorial from the project root:

```bash
cmake --build build --target neutrino_tutorial_01_window_and_scene -j
```

Then run the executable:

```bash
./build/bin/neutrino_tutorial_01_window_and_scene
```

You should see an 1280x720 window presenting a 640x360 canvas with a blue sky and green ground strip. Resizing the window preserves the 16:9 aspect ratio with black bars. Pressing <kbd>Escape</kbd> exits cleanly.

---

## Next step

In **Step 2: One sprite on screen**, we will introduce sprite sheets, frame definitions, sprite sets, and batch rendering.
