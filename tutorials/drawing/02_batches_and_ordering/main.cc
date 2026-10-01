//
// Neutrino Drawing Tutorial 02: Batches & Ordering Bands
//
// Demonstrates:
// 1. Depth-sorted rendering with `neutrino::sprite_batch`.
// 2. Coarse category ordering with `neutrino::draw_layer`.
// 3. Y-sorting within bands for natural 2.5D overlap.
// 4. Stable tie-breaking that eliminates Z-fighting.
// 5. Comparing sorted batching vs naive unsorted draw order.
//

#include <neutrino/application.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/scene/scene_transitions.hh>
#include <neutrino/video/draw.hh>
#include <neutrino/video/sprites.hh>
#include <neutrino/video/world/sprite_batch.hh>

#include <sdlpp/app/entry_point.hh>
#include <sdlpp/video/color.hh>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>

namespace {

    constexpr int logical_width = 640;
    constexpr int logical_height = 360;
    constexpr int window_width = 1280;
    constexpr int window_height = 720;

    enum class render_band : int {
        shadows      = 0,
        actors       = 1,
        projectiles  = 2,
        floating_ui  = 3,
    };

    [[nodiscard]] constexpr neutrino::draw_layer to_layer(render_band b) noexcept {
        return neutrino::draw_layer{static_cast<int>(b)};
    }

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

    [[nodiscard]] neutrino::sprite_def make_assets_def() {
        return neutrino::sprite_def_builder{}
            .from_file(asset_path("arcade_platformer.png"), 352, 320)
            .add_visual("player", neutrino::rect{0, 0, 32, 32},   neutrino::point{16, 32})
            .add_visual("tree",   neutrino::rect{128, 0, 32, 32}, neutrino::point{16, 32})
            .add_visual("spark",  neutrino::rect{192, 0, 16, 16}, neutrino::point{8, 8})
            .add_visual("coin",   neutrino::rect{80, 112, 16, 16}, neutrino::point{8, 16})
            .build();
    }

    struct actor_entity {
        neutrino::world_point position;
        float base_x = 0.0f;
        float base_y = 0.0f;
        float speed = 1.0f;
        float phase = 0.0f;
    };

    class batch_demo_scene final : public neutrino::base_scene {
    public:
        void on_enter() override {
            m_set = neutrino::acquire_sprite(make_assets_def());

            // Initialize 3 static obstacles (trees)
            m_trees = {
                neutrino::world_point{200.0f, 180.0f},
                neutrino::world_point{340.0f, 180.0f},
                neutrino::world_point{480.0f, 180.0f},
            };

            // Initialize 3 moving characters walking up and down across the trees' Y line
            m_actors = {
                actor_entity{.position = {200.0f, 140.0f}, .base_x = 200.0f, .base_y = 180.0f, .speed = 1.2f, .phase = 0.0f},
                actor_entity{.position = {340.0f, 220.0f}, .base_x = 340.0f, .base_y = 180.0f, .speed = 1.5f, .phase = 2.0f},
                actor_entity{.position = {480.0f, 150.0f}, .base_x = 480.0f, .base_y = 180.0f, .speed = 1.0f, .phase = 4.0f},
            };
        }

        void fixed_update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override {
            if (in.pressed(sdlpp::scancode::escape)) {
                neutrino::pop_scene();
                return;
            }

            // Toggle between sorted sprite_batch and naive unsorted order
            if (in.pressed(sdlpp::scancode::tab) || in.pressed(sdlpp::scancode::space)) {
                m_use_batch = !m_use_batch;
            }

            m_elapsed_sec += dt.count();

            // Animate actors moving up and down across the tree line (Y = 180)
            for (auto& a : m_actors) {
                a.position.y = a.base_y + 45.0f * std::sin(m_elapsed_sec * a.speed + a.phase);
                // Slight horizontal sway
                a.position.x = a.base_x + 15.0f * std::cos(m_elapsed_sec * 0.5f + a.phase);
            }

            // Animate orbiting spark projectile
            m_spark_pos.x = 340.0f + 160.0f * std::cos(m_elapsed_sec * 2.0f);
            m_spark_pos.y = 180.0f + 70.0f * std::sin(m_elapsed_sec * 2.0f);
        }

        void handle_action(const sdlpp::event&) override {}

        void render() override {
            // Isometric-style tile floor background
            render_floor_grid();

            auto player_vis = m_set.visual("player");
            auto tree_vis   = m_set.visual("tree");
            auto spark_vis  = m_set.visual("spark");

            if (m_use_batch) {
                // =============================================================
                // PATH A: neutrino::sprite_batch with depth sorting & layers
                // =============================================================
                neutrino::sprite_batch batch;

                // 1. Shadows (Layer 0) - always under actors and trees!
                for (const auto& a : m_actors) {
                    // Draw oval ground shadow
                    batch.add(
                        neutrino::world_point{a.position.x, a.position.y},
                        to_layer(render_band::shadows),
                        a.position.y,
                        spark_vis,
                        neutrino::sprite_draw_params{.scale = 1.8f}
                    );
                }

                // 2. Trees (Layer 1) - Y-sorted at Y = 180
                for (const auto& t : m_trees) {
                    batch.add(
                        t,
                        to_layer(render_band::actors),
                        t.y, // Y-depth
                        tree_vis,
                        neutrino::sprite_draw_params{.scale = 2.5f}
                    );
                }

                // 3. Characters (Layer 1) - Y-sorted!
                // When actor.y < 180, actor is north of tree -> draws UNDER tree.
                // When actor.y > 180, actor is south of tree -> draws OVER tree.
                for (const auto& a : m_actors) {
                    batch.add(
                        a.position,
                        to_layer(render_band::actors),
                        a.position.y,
                        player_vis,
                        neutrino::sprite_draw_params{.scale = 2.0f}
                    );
                }

                // 4. Orbiting spark projectile (Layer 2) - always ON TOP of actors!
                batch.add(
                    m_spark_pos,
                    to_layer(render_band::projectiles),
                    m_spark_pos.y,
                    spark_vis,
                    neutrino::sprite_draw_params{.scale = 2.5f}
                );

                // Flush: stable sorts (layer, depth, call_order) and draws back-to-front
                batch.flush();
            } else {
                // =============================================================
                // PATH B: Naive Unsorted Painting (Demonstrates Painter's Flaw)
                // =============================================================
                // If we draw actors first, trees always draw over them even when actor is south!
                for (const auto& a : m_actors) {
                    if (player_vis) {
                        (void) neutrino::draw_sprite(
                            neutrino::point{static_cast<int>(a.position.x), static_cast<int>(a.position.y)},
                            *player_vis,
                            neutrino::sprite_draw_params{.scale = 2.0f}
                        );
                    }
                }
                for (const auto& t : m_trees) {
                    if (tree_vis) {
                        (void) neutrino::draw_sprite(
                            neutrino::point{static_cast<int>(t.x), static_cast<int>(t.y)},
                            *tree_vis,
                            neutrino::sprite_draw_params{.scale = 2.5f}
                        );
                    }
                }
            }

            render_hud();
        }

        [[nodiscard]] bool is_opaque() const override {
            return true;
        }

    private:
        void render_floor_grid() {
            neutrino::draw_rect_fill(neutrino::rect{0, 0, logical_width, logical_height}, sdlpp::color{40, 50, 60, 255});

            // Grid lines to visualize ground depth
            for (int y = 80; y <= 300; y += 40) {
                neutrino::draw_line(neutrino::point{40, y}, neutrino::point{600, y}, sdlpp::color{55, 68, 82, 255});
            }
            for (int x = 80; x <= 560; x += 60) {
                neutrino::draw_line(neutrino::point{x, 80}, neutrino::point{x, 300}, sdlpp::color{50, 62, 75, 255});
            }
        }

        void render_hud() {
            // Mode banner
            const sdlpp::color banner_col = m_use_batch
                ? sdlpp::color{39, 174, 96, 220} // Green: Sorted Batch
                : sdlpp::color{192, 57, 43, 220}; // Red: Naive Unsorted

            neutrino::draw_rect_fill(neutrino::rect{16, 16, 360, 28}, banner_col);
            neutrino::draw_rect(neutrino::rect{16, 16, 360, 28}, sdlpp::colors::white);

            // Instructions text box
            neutrino::draw_rect_fill(neutrino::rect{16, 320, 420, 24}, sdlpp::color{0, 0, 0, 180});
            neutrino::draw_rect(neutrino::rect{16, 320, 420, 24}, sdlpp::colors::light_gray);
        }

        neutrino::sprite_set_handle m_set;
        std::vector<neutrino::world_point> m_trees;
        std::vector<actor_entity> m_actors;
        neutrino::world_point m_spark_pos{340.0f, 180.0f};

        float m_elapsed_sec = 0.0f;
        bool m_use_batch = true;
    };

    class tutorial_app final : public neutrino::application {
    public:
        tutorial_app()
            : neutrino::application(make_config()) {
        }

    protected:
        std::unique_ptr<neutrino::base_scene> create_initial_scene() override {
            return std::make_unique<batch_demo_scene>();
        }

    private:
        static neutrino::application_config make_config() {
            neutrino::application_config cfg;
            cfg.title = "Neutrino Drawing Tutorial 02 - Batches & Ordering Bands";
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
