//
// Neutrino Tutorial 02: One Sprite on Screen
//
// In this second step of the platformer tutorial, we introduce:
// 1. Loading sprite assets from disk using `image_from_disk`.
// 2. Defining frame visuals and animation clips using `sprite_def`.
// 3. Anchoring pivots with bottom-center alignment for natural platform ground contacts.
// 4. Acquiring a resident GPU `sprite_set` using `sprite_cache` and RAII leases.
// 5. Spawning a per-entity `sprite_instance` playhead.
// 6. Drawing the animated sprite to the screen with `neutrino::draw_sprite`.
// 7. Enforcing safe destruction order in C++ scene classes.
//

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
    // Operating in 640x360 coordinates decoupled from physical monitor resolution.
    constexpr int logical_width = 640;
    constexpr int logical_height = 360;

    // Window dimensions at startup.
    constexpr int window_width = 1280;
    constexpr int window_height = 720;

    // Visual scale factor for pixel-art sprites.
    constexpr float sprite_scale = 2.0f;

    // Helper function to resolve asset paths across build and run environments.
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
    // A `sprite_def` is pure data with zero GPU dependency.
    // We construct it fluently using `neutrino::sprite_def_builder`.
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
            // Spawning an instance creates an independent animation clock and retains
            // an internal lease on the underlying GPU sprite set.
            m_player = m_set.spawn("idle");
        }

        void fixed_update(neutrino::sim_duration, const neutrino::input_snapshot&) override {
            // Note: We do NOT need to call an update method on m_player!
            // The Neutrino application loop automatically advances registered sprite
            // animation states during its update cycle.

            // Press Escape to pop the scene and cleanly exit.
            if (neutrino::hotkey{sdlpp::scancode::escape}.pressed()) {
                std::cout << "[Tutorial 02] Escape pressed - exiting\n";
                neutrino::pop_scene();
            }
        }

        void handle_action(const sdlpp::event&) override {
            // Discrete one-shot window and system events arrive here.
        }

        void render() override {
            // 1. Draw sky background.
            neutrino::draw_rect_fill(
                neutrino::rect{0, 0, logical_width, logical_height},
                sdlpp::color{90, 160, 230, 255}
            );

            // 2. Draw ground strip.
            constexpr int ground_y = 300;
            constexpr int ground_height = logical_height - ground_y;

            // Green grass layer
            neutrino::draw_rect_fill(
                neutrino::rect{0, ground_y, logical_width, 10},
                sdlpp::color{80, 185, 95, 255}
            );
            // Brown soil layer
            neutrino::draw_rect_fill(
                neutrino::rect{0, ground_y + 10, logical_width, ground_height - 10},
                sdlpp::color{110, 75, 45, 255}
            );

            // 3. Draw the player sprite standing on the ground!
            // Because our visual pivot is at the bottom-center {16, 32}, passing
            // y = ground_y anchors the character's feet directly on top of the grass.
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

        // Player position in logical render coordinates.
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

            // Fixed 640x360 virtual design resolution, letterboxed to the window.
            cfg.logical_size = neutrino::dim{logical_width, logical_height};
            cfg.scale = neutrino::scale_mode::letterbox;

            return cfg;
        }
    };

} // namespace

// SDL entry point macro linking the application to SDL's main loop.
SDLPP_MAIN(tutorial_app)
