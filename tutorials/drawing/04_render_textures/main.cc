//
// Neutrino Drawing Tutorial 04: Offscreen Render Textures
//
// Demonstrates:
// 1. Allocating offscreen GPU targets with `neutrino::render_texture::create`.
// 2. The RAII `compose()` pattern for safe, leak-free target binding.
// 3. Baking complex procedural scenery into VRAM once.
// 4. Blitting baked textures as single draw calls.
// 5. Creating a high-performance cached minimap.
//

#include <neutrino/application.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/scene/scene_transitions.hh>
#include <neutrino/video/draw.hh>
#include <neutrino/video/render_texture.hh>

#include <sdlpp/app/entry_point.hh>
#include <sdlpp/video/color.hh>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

namespace {

    constexpr int logical_width = 640;
    constexpr int logical_height = 360;
    constexpr int window_width = 1280;
    constexpr int window_height = 720;

    class render_texture_demo_scene final : public neutrino::base_scene {
    public:
        void on_enter() override {
            // 1. Allocate full-screen background render texture
            m_backdrop = neutrino::render_texture::create(neutrino::dim{logical_width, logical_height});
            bake_scenery();

            // 2. Allocate 120x120 cached minimap target
            m_minimap = neutrino::render_texture::create(neutrino::dim{120, 120});
            bake_minimap();

            // Initialize moving entities (patrolling drones)
            m_drones = {
                neutrino::point{140, 100},
                neutrino::point{320, 220},
                neutrino::point{460, 120},
            };
        }

        void fixed_update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override {
            if (in.pressed(sdlpp::scancode::escape)) {
                neutrino::pop_scene();
                return;
            }

            // Spacebar: Re-compose the procedural scenery with new random stars!
            if (in.pressed(sdlpp::scancode::space)) {
                m_star_seed += 13;
                bake_scenery();
            }

            const float dt_sec = dt.count();
            m_elapsed_sec += dt_sec;

            // Move player with arrow keys or WASD
            constexpr float move_speed = 180.0f;
            if (in.held(sdlpp::scancode::right) || in.held(sdlpp::scancode::d)) m_player_pos.x += move_speed * dt_sec;
            if (in.held(sdlpp::scancode::left)  || in.held(sdlpp::scancode::a)) m_player_pos.x -= move_speed * dt_sec;
            if (in.held(sdlpp::scancode::down)  || in.held(sdlpp::scancode::s)) m_player_pos.y += move_speed * dt_sec;
            if (in.held(sdlpp::scancode::up)    || in.held(sdlpp::scancode::w)) m_player_pos.y -= move_speed * dt_sec;

            m_player_pos.x = std::clamp(m_player_pos.x, 16.0f, static_cast<float>(logical_width) - 16.0f);
            m_player_pos.y = std::clamp(m_player_pos.y, 16.0f, static_cast<float>(logical_height) - 16.0f);

            // Animate orbiting drones
            m_drones[0].x = static_cast<int>(140 + 40 * std::cos(m_elapsed_sec * 1.5f));
            m_drones[0].y = static_cast<int>(100 + 40 * std::sin(m_elapsed_sec * 1.5f));

            m_drones[1].x = static_cast<int>(320 + 80 * std::cos(m_elapsed_sec * 1.0f));
            m_drones[1].y = static_cast<int>(220 + 30 * std::sin(m_elapsed_sec * 2.0f));

            m_drones[2].x = static_cast<int>(460 + 50 * std::sin(m_elapsed_sec * 1.2f));
            m_drones[2].y = static_cast<int>(120 + 50 * std::cos(m_elapsed_sec * 1.2f));
        }

        void handle_action(const sdlpp::event&) override {}

        void render() override {
            // =================================================================
            // PASS 1: Blit the pre-baked static scenery (1 single blit call!)
            // =================================================================
            if (m_backdrop) {
                m_backdrop->blit();
            }

            // =================================================================
            // PASS 2: Render dynamic entities in the world
            // =================================================================
            // Draw patrolling drones
            for (const auto& drone : m_drones) {
                neutrino::draw_circle_fill(neutrino::circle{drone, 10}, sdlpp::color{231, 76, 60, 255});
                neutrino::draw_circle(neutrino::circle{drone, 10}, sdlpp::colors::white);
                neutrino::draw_circle(neutrino::circle{drone, 16}, sdlpp::color{231, 76, 60, 100});
            }

            // Draw player avatar (emerald diamond)
            const neutrino::point p{static_cast<int>(m_player_pos.x), static_cast<int>(m_player_pos.y)};
            neutrino::draw_rect_fill(neutrino::rect{p.x - 12, p.y - 12, 24, 24}, sdlpp::color{46, 204, 113, 255});
            neutrino::draw_rect(neutrino::rect{p.x - 12, p.y - 12, 24, 24}, sdlpp::colors::white);
            neutrino::draw_cross(p, 4, 1.0f, sdlpp::colors::black);

            // =================================================================
            // PASS 3: Render Cached Minimap at Top-Right
            // =================================================================
            render_minimap();

            // =================================================================
            // PASS 4: Render Screen HUD & Instructions
            // =================================================================
            render_hud();
        }

        [[nodiscard]] bool is_opaque() const override {
            return true;
        }

    private:
        void bake_scenery() {
            if (!m_backdrop) return;

            // Use compose() to bind the texture as render target, clear, and draw:
            m_backdrop->compose([&] {
                // 1. Deep space background
                neutrino::draw_rect_fill(neutrino::rect{0, 0, logical_width, logical_height}, sdlpp::color{12, 16, 28, 255});

                // 2. Procedural starfield
                std::srand(m_star_seed);
                for (int i = 0; i < 250; ++i) {
                    const int x = std::rand() % logical_width;
                    const int y = std::rand() % (logical_height - 60);
                    const uint8_t b = static_cast<uint8_t>(140 + std::rand() % 115);
                    const sdlpp::color star_col{b, b, static_cast<uint8_t>(std::min(255, b + 20)), 255};

                    if (i % 8 == 0) {
                        neutrino::draw_cross(neutrino::point{x, y}, 2, 1.0f, star_col);
                    } else {
                        neutrino::draw_point(x, y, star_col);
                    }
                }

                // 3. Layered mountain silhouettes
                for (int x = -60; x <= logical_width + 60; x += 140) {
                    neutrino::draw_circle_fill(
                        neutrino::circle{neutrino::point{x, logical_height + 40}, 160},
                        sdlpp::color{20, 26, 45, 255}
                    );
                }
                for (int x = 20; x <= logical_width + 40; x += 180) {
                    neutrino::draw_circle_fill(
                        neutrino::circle{neutrino::point{x, logical_height + 60}, 180},
                        sdlpp::color{15, 20, 36, 255}
                    );
                }

                // 4. Ground landing strip
                neutrino::draw_rect_fill(neutrino::rect{0, logical_height - 30, logical_width, 30}, sdlpp::color{30, 40, 60, 255});
                neutrino::draw_line(neutrino::point{0, logical_height - 30}, neutrino::point{logical_width, logical_height - 30}, sdlpp::colors::cyan);
            });
        }

        void bake_minimap() {
            if (!m_minimap) return;

            // Bake static room borders into the minimap texture
            m_minimap->compose([&] {
                // Semi-transparent charcoal tray
                neutrino::draw_rect_fill(neutrino::rect{0, 0, 120, 120}, sdlpp::color{10, 12, 18, 210});

                // Static room zones
                neutrino::draw_rect(neutrino::rect{10, 10, 35, 30}, sdlpp::color{120, 130, 150, 255});
                neutrino::draw_rect(neutrino::rect{45, 20, 40, 45}, sdlpp::color{120, 130, 150, 255});
                neutrino::draw_rect(neutrino::rect{70, 60, 40, 40}, sdlpp::color{120, 130, 150, 255});

                // Minimap outer border
                neutrino::draw_rect(neutrino::rect{0, 0, 120, 120}, sdlpp::color{200, 210, 230, 255});
            });
        }

        void render_minimap() {
            if (!m_minimap) return;

            // 1. Blit pre-baked minimap texture at top-right
            const neutrino::rect minimap_dst{504, 16, 120, 120};
            m_minimap->blit(minimap_dst);

            // 2. Draw live player blip (green dot)
            const float scale_x = 120.0f / static_cast<float>(logical_width);
            const float scale_y = 120.0f / static_cast<float>(logical_height);

            const int px = minimap_dst.x + static_cast<int>(m_player_pos.x * scale_x);
            const int py = minimap_dst.y + static_cast<int>(m_player_pos.y * scale_y);
            neutrino::draw_circle_fill(neutrino::circle{neutrino::point{px, py}, 2}, sdlpp::colors::lime_green);

            // 3. Draw live drone blips (red dots)
            for (const auto& drone : m_drones) {
                const int dx = minimap_dst.x + static_cast<int>(drone.x * scale_x);
                const int dy = minimap_dst.y + static_cast<int>(drone.y * scale_y);
                neutrino::draw_circle_fill(neutrino::circle{neutrino::point{dx, dy}, 2}, sdlpp::colors::crimson);
            }
        }

        void render_hud() {
            // Instructions banner at top-left
            neutrino::draw_rect_fill(neutrino::rect{16, 16, 360, 28}, sdlpp::color{0, 0, 0, 180});
            neutrino::draw_rect(neutrino::rect{16, 16, 360, 28}, sdlpp::colors::white);

            // Seed indicator
            neutrino::draw_circle_fill(neutrino::circle{neutrino::point{30, 30}, 5}, sdlpp::colors::cyan);
        }

        std::optional<neutrino::render_texture> m_backdrop;
        std::optional<neutrino::render_texture> m_minimap;

        neutrino::world_point m_player_pos{320.0f, 180.0f};
        std::vector<neutrino::point> m_drones;

        float m_elapsed_sec = 0.0f;
        unsigned int m_star_seed = 42;
    };

    class tutorial_app final : public neutrino::application {
    public:
        tutorial_app()
            : neutrino::application(make_config()) {
        }

    protected:
        std::unique_ptr<neutrino::base_scene> create_initial_scene() override {
            return std::make_unique<render_texture_demo_scene>();
        }

    private:
        static neutrino::application_config make_config() {
            neutrino::application_config cfg;
            cfg.title = "Neutrino Drawing Tutorial 04 - Offscreen Render Textures";
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
