//
// Neutrino Drawing Tutorial 03: Camera & Parallax
//
// Demonstrates:
// 1. 2D Look-at camera tracking with `neutrino::camera`.
// 2. Exponential smoothing / damping for cinematic tracking.
// 3. Zoom controls with positive-finite validation.
// 4. Drift-free multi-plane parallax anchored to `parallax_rest`.
// 5. Camera-aware batch projection with `sprite_batch`.
//

#include <neutrino/application.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/scene/scene_transitions.hh>
#include <neutrino/video/draw.hh>
#include <neutrino/video/sprites.hh>
#include <neutrino/video/world/camera.hh>
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
            .add_visual("coin",   neutrino::rect{80, 112, 16, 16}, neutrino::point{8, 16})
            .build();
    }

    class camera_parallax_scene final : public neutrino::base_scene {
    public:
        void on_enter() override {
            m_set = neutrino::acquire_sprite(make_assets_def());

            m_cam.target = {320.0f, 180.0f};
            m_cam.zoom = 1.0f;
            m_cam.parallax_rest = {0.0f, 0.0f};

            // Spawn trees across an expansive horizontal world (-600 to 1800)
            for (int x = -600; x <= 1800; x += 120) {
                m_trees.push_back(neutrino::world_point{static_cast<float>(x), 260.0f});
            }

            // Spawn collectible coins floating in the air
            for (int x = -500; x <= 1700; x += 180) {
                m_coins.push_back(neutrino::world_point{static_cast<float>(x), 190.0f});
            }
        }

        void fixed_update(neutrino::sim_duration dt, const neutrino::input_snapshot& in) override {
            if (in.pressed(sdlpp::scancode::escape)) {
                neutrino::pop_scene();
                return;
            }

            const float dt_sec = dt.count();
            constexpr float move_speed = 220.0f;

            // 1. Player horizontal & vertical controls
            if (in.held(sdlpp::scancode::right) || in.held(sdlpp::scancode::d)) m_player_pos.x += move_speed * dt_sec;
            if (in.held(sdlpp::scancode::left)  || in.held(sdlpp::scancode::a)) m_player_pos.x -= move_speed * dt_sec;
            if (in.held(sdlpp::scancode::down)  || in.held(sdlpp::scancode::s)) m_player_pos.y += move_speed * dt_sec;
            if (in.held(sdlpp::scancode::up)    || in.held(sdlpp::scancode::w)) m_player_pos.y -= move_speed * dt_sec;

            // 2. Zoom controls: Q to zoom out, E to zoom in
            if (in.held(sdlpp::scancode::q)) m_cam.zoom = std::max(0.5f, m_cam.zoom - 0.7f * dt_sec);
            if (in.held(sdlpp::scancode::e)) m_cam.zoom = std::min(2.5f, m_cam.zoom + 0.7f * dt_sec);

            // 3. Smooth camera tracking with exponential decay
            const float blend = 1.0f - std::exp(-6.0f * dt_sec);
            m_cam.target.x += (m_player_pos.x - m_cam.target.x) * blend;
            m_cam.target.y += (m_player_pos.y - m_cam.target.y) * blend;
        }

        void handle_action(const sdlpp::event&) override {}

        void render() override {
            const neutrino::rect viewport{0, 0, logical_width, logical_height};

            // Deep twilight sky (Plane 0: static background, parallax = 0.0)
            neutrino::draw_rect_fill(viewport, sdlpp::color{15, 20, 36, 255});

            // 1. Distant Mountains (Plane 1: parallax_x = 0.25, parallax_y = 0.05)
            neutrino::world_layer_header mountain_plane;
            mountain_plane.parallax_x = 0.25f;
            mountain_plane.parallax_y = 0.05f;
            render_mountains(mountain_plane, viewport);

            // 2. Midground Rolling Hills (Plane 2: parallax_x = 0.55, parallax_y = 0.15)
            neutrino::world_layer_header hills_plane;
            hills_plane.parallax_x = 0.55f;
            hills_plane.parallax_y = 0.15f;
            render_hills(hills_plane, viewport);

            // 3. Foreground Playfield Ground Strip (Plane 3: parallax = 1.0)
            neutrino::world_layer_header actor_plane; // parallax_x = 1.0, parallax_y = 1.0
            render_ground_strip(actor_plane, viewport);

            // 4. Camera-aware batch for foreground actors and world sprites
            auto player_vis = m_set.visual("player");
            auto tree_vis   = m_set.visual("tree");
            auto coin_vis   = m_set.visual("coin");

            neutrino::sprite_batch batch(m_cam, viewport, actor_plane);

            // Add trees to batch
            for (const auto& tree : m_trees) {
                batch.add(tree, tree.y, tree_vis, neutrino::sprite_draw_params{.scale = 2.0f});
            }

            // Add floating coins
            for (const auto& coin : m_coins) {
                batch.add(coin, coin.y, coin_vis, neutrino::sprite_draw_params{.scale = 1.8f});
            }

            // Add player
            batch.add(m_player_pos, m_player_pos.y, player_vis, neutrino::sprite_draw_params{.scale = 2.0f});

            // Projects world coordinates via to_screen(), applies zoom to scale, and draws
            batch.flush();

            // 5. Screen-Space HUD (no camera transform)
            render_hud();
        }

        [[nodiscard]] bool is_opaque() const override {
            return true;
        }

    private:
        void render_mountains(const neutrino::world_layer_header& plane, const neutrino::rect& viewport) {
            // Draw mountain peaks repeating across world coordinates
            for (int x = -1000; x <= 2200; x += 280) {
                const neutrino::world_point peak{static_cast<float>(x), 160.0f};
                const auto sp = neutrino::to_screen(m_cam, plane, viewport.dimensions(), peak);
                const int rad = static_cast<int>(120.0f * m_cam.zoom);
                neutrino::draw_circle_fill(neutrino::circle{sp, rad}, sdlpp::color{28, 38, 58, 255});
            }
        }

        void render_hills(const neutrino::world_layer_header& plane, const neutrino::rect& viewport) {
            for (int x = -800; x <= 2000; x += 180) {
                const neutrino::world_point peak{static_cast<float>(x), 220.0f};
                const auto sp = neutrino::to_screen(m_cam, plane, viewport.dimensions(), peak);
                const int rad = static_cast<int>(90.0f * m_cam.zoom);
                neutrino::draw_circle_fill(neutrino::circle{sp, rad}, sdlpp::color{38, 55, 75, 255});
            }
        }

        void render_ground_strip(const neutrino::world_layer_header& plane, const neutrino::rect& viewport) {
            // Ground strip spanning world space
            const neutrino::world_point g_left{-800.0f, 260.0f};
            const neutrino::world_point g_right{2200.0f, 260.0f};

            const auto p1 = neutrino::to_screen(m_cam, plane, viewport.dimensions(), g_left);
            const auto p2 = neutrino::to_screen(m_cam, plane, viewport.dimensions(), g_right);

            neutrino::draw_line_thick(p1, p2, 6.0f * m_cam.zoom, sdlpp::color{46, 204, 113, 255});
        }

        void render_hud() {
            // HUD panel showing camera position and zoom
            neutrino::draw_rect_fill(neutrino::rect{12, 12, 340, 24}, sdlpp::color{0, 0, 0, 180});
            neutrino::draw_rect(neutrino::rect{12, 12, 340, 24}, sdlpp::colors::white);

            // Small indicator lamps for zoom level
            const int zoom_bar_w = static_cast<int>((m_cam.zoom / 2.5f) * 60.0f);
            neutrino::draw_rect_fill(neutrino::rect{20, 18, zoom_bar_w, 12}, sdlpp::colors::cyan);
            neutrino::draw_rect(neutrino::rect{20, 18, 60, 12}, sdlpp::colors::light_gray);
        }

        neutrino::sprite_set_handle m_set;
        neutrino::camera m_cam;
        neutrino::world_point m_player_pos{320.0f, 260.0f};

        std::vector<neutrino::world_point> m_trees;
        std::vector<neutrino::world_point> m_coins;
    };

    class tutorial_app final : public neutrino::application {
    public:
        tutorial_app()
            : neutrino::application(make_config()) {
        }

    protected:
        std::unique_ptr<neutrino::base_scene> create_initial_scene() override {
            return std::make_unique<camera_parallax_scene>();
        }

    private:
        static neutrino::application_config make_config() {
            neutrino::application_config cfg;
            cfg.title = "Neutrino Drawing Tutorial 03 - Camera & Parallax";
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
