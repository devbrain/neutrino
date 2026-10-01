//
// Neutrino Tutorial 03: Moving with the Keyboard
//
// In this third step of the platformer tutorial, we introduce:
// 1. Reading user input from the per-frame `neutrino::input_snapshot`.
// 2. Continuous held state (`in.held(...)`) for horizontal running.
// 3. Discrete edge transitions (`in.pressed(...)`) for jump impulses.
// 4. Substep safety: why edges fire once per frame while held state persists.
// 5. Directing sprite visual facing with `neutrino::sprite_flip::horizontal`.
// 6. Switching and restarting animation clips with `sprite_instance`.
//

#include <neutrino/application.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/scene/scene_transitions.hh>
#include <neutrino/video/draw.hh>
#include <neutrino/video/sprites.hh>

#include <sdlpp/app/entry_point.hh>
#include <sdlpp/video/color.hh>

#include <algorithm>
#include <chrono>
#include <cmath>
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

    // Ground position where character feet rest.
    constexpr int ground_y = 300;

    // Visual scale factor for pixel-art sprites.
    constexpr float sprite_scale = 2.0f;

    // Half-width of player sprite (16px authored frame * scale = 32px on screen).
    constexpr float player_half_width = 16.0f * sprite_scale;

    // Kinematic movement constants.
    constexpr float run_speed = 180.0f;    // Horizontal speed in pixels per second.
    constexpr float jump_velocity = -420.0f; // Initial upward impulse.
    constexpr float gravity = 980.0f;      // Downward acceleration.

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
            .from_file(asset_path("arcade_platformer.png"), 352, 320)
            // Visual frames with bottom-center pivot origin {16, 32}
            .add_visual("player.idle",   neutrino::rect{0, 0, 32, 32},  neutrino::point{16, 32})
            .add_visual("player.walk.0", neutrino::rect{0, 0, 32, 32},  neutrino::point{16, 32})
            .add_visual("player.walk.1", neutrino::rect{32, 0, 32, 32}, neutrino::point{16, 32})
            .add_visual("player.walk.2", neutrino::rect{64, 0, 32, 32}, neutrino::point{16, 32})
            .add_visual("player.jump",   neutrino::rect{0, 32, 32, 32}, neutrino::point{16, 32})

            // Animation clips
            .add_clip("idle", {"player.idle"}, 1000ms, true)
            .add_clip("walk", {"player.walk.0", "player.walk.1", "player.walk.2", "player.walk.1"}, 100ms, true)
            .add_clip("jump", {"player.jump"}, 1000ms, false)
            .build();
    }

    // =========================================================================
    // The Scene: main_scene
    // =========================================================================
    class main_scene final : public neutrino::base_scene {
    public:
        main_scene() = default;

        void on_enter() override {
            std::cout << "[Tutorial 03] Acquiring player sprite set from cache...\n";
            m_set = neutrino::acquire_sprite(make_player_def());
            m_player = m_set.spawn("idle");
        }

        void fixed_update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override {
            // 1. Exit cleanly on Escape press
            if (in.pressed(sdlpp::scancode::escape)) {
                std::cout << "[Tutorial 03] Escape pressed - exiting\n";
                neutrino::pop_scene();
                return;
            }

            const float dt_sec = dt.count();

            // 2. Read continuous horizontal movement (Held State)
            // We check both Arrow keys and WASD keys.
            const bool move_left = in.held(sdlpp::scancode::left) || in.held(sdlpp::scancode::a);
            const bool move_right = in.held(sdlpp::scancode::right) || in.held(sdlpp::scancode::d);

            if (move_left != move_right) {
                const float dir = move_right ? 1.0f : -1.0f;
                m_player_x += dir * run_speed * dt_sec;
                m_facing_left = move_left;
            }

            // Clamp player within screen horizontal margins
            m_player_x = std::clamp(
                m_player_x, 
                player_half_width, 
                static_cast<float>(logical_width) - player_half_width
            );

            // 3. Read discrete jump impulse (Edge Transition)
            // in.pressed() returns true ONLY on the single frame the key was struck down.
            // If the key is held down continuously, in.pressed() returns false on subsequent ticks.
            const bool jump_pressed = in.pressed(sdlpp::scancode::space) 
                                   || in.pressed(sdlpp::scancode::up) 
                                   || in.pressed(sdlpp::scancode::w);

            if (jump_pressed && m_on_ground) {
                m_player_vy = jump_velocity;
                m_on_ground = false;
                m_player.restart("jump");
                m_current_clip = "jump";
            }

            // 4. Apply vertical integration and gravity when airborne
            if (!m_on_ground) {
                m_player_vy += gravity * dt_sec;
                m_player_y += m_player_vy * dt_sec;

                // Simple ground collision check (Step 4 introduces the real physics engine)
                if (m_player_y >= ground_y) {
                    m_player_y = ground_y;
                    m_player_vy = 0.0f;
                    m_on_ground = true;
                }
            }

            // 5. Manage animation clips based on state
            if (m_on_ground) {
                if (move_left != move_right) {
                    if (m_current_clip != "walk") {
                        m_current_clip = "walk";
                        m_player.switch_to("walk");
                    }
                } else {
                    if (m_current_clip != "idle") {
                        m_current_clip = "idle";
                        m_player.switch_to("idle");
                    }
                }
            }
        }

        void handle_action(const sdlpp::event&) override {}

        void render() override {
            // 1. Draw sky background
            neutrino::draw_rect_fill(
                neutrino::rect{0, 0, logical_width, logical_height},
                sdlpp::color{90, 160, 230, 255}
            );

            // 2. Draw ground strip
            constexpr int ground_height = logical_height - ground_y;
            neutrino::draw_rect_fill(
                neutrino::rect{0, ground_y, logical_width, 10},
                sdlpp::color{80, 185, 95, 255}
            );
            neutrino::draw_rect_fill(
                neutrino::rect{0, ground_y + 10, logical_width, ground_height - 10},
                sdlpp::color{110, 75, 45, 255}
            );

            // 3. Draw keybind instruction banner
            draw_instructions();

            // 4. Draw player sprite with horizontal flipping
            if (m_player.valid()) {
                const auto flip = m_facing_left 
                    ? neutrino::sprite_flip::horizontal 
                    : neutrino::sprite_flip::none;

                neutrino::draw_sprite(
                    neutrino::point{static_cast<int>(m_player_x), static_cast<int>(m_player_y)},
                    m_player.state(),
                    neutrino::sprite_draw_params{
                        .scale = sprite_scale,
                        .flip = flip,
                        .rotation_degrees = 0.0f,
                    }
                );
            }
        }

        [[nodiscard]] bool is_opaque() const override {
            return true;
        }

    private:
        void draw_instructions() const {
            // Top HUD frame showing active movement controls
            neutrino::draw_rect_fill(neutrino::rect{12, 12, 380, 24}, sdlpp::color{0, 0, 0, 160});
            neutrino::draw_rect(neutrino::rect{12, 12, 380, 24}, sdlpp::colors::white);

            // Small indicator lamps for active input
            neutrino::draw_rect_fill(
                neutrino::rect{20, 18, 12, 12}, 
                m_facing_left ? sdlpp::colors::yellow : sdlpp::colors::dark_gray
            );
            neutrino::draw_rect_fill(
                neutrino::rect{36, 18, 12, 12}, 
                !m_facing_left ? sdlpp::colors::yellow : sdlpp::colors::dark_gray
            );
            neutrino::draw_rect_fill(
                neutrino::rect{56, 18, 12, 12}, 
                !m_on_ground ? sdlpp::colors::cyan : sdlpp::colors::dark_gray
            );
        }

        neutrino::sprite_set_handle m_set;
        neutrino::sprite_instance   m_player;

        float m_player_x{120.0f};
        float m_player_y{ground_y};
        float m_player_vy{0.0f};

        bool m_facing_left{false};
        bool m_on_ground{true};
        std::string m_current_clip{"idle"};
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
            cfg.title = "Neutrino Tutorial 03 - Moving with the Keyboard";
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
