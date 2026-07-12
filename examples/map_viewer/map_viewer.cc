//
// Interactive tile-map viewer: loads a Tiled map and lets you pan and zoom the
// camera across it. Exercises the full world -> cache -> bundle -> renderer path.
//

#include <neutrino/application.hh>
#include <neutrino/input/hotkey.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/scene/scene_transitions.hh>
#include <neutrino/video/draw.hh>
#include <neutrino/video/globals.hh>
#include <neutrino/video/world/camera.hh>
#include <neutrino/video/world/world_renderer.hh>
#include <neutrino/world/tmx_loader.hh>
#include <neutrino/world/world.hh>

#include <sdlpp/app/entry_point.hh>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>

namespace fs = std::filesystem;
using namespace neutrino;

namespace {
    constexpr int window_width = 1024;
    constexpr int window_height = 640;
    constexpr float pan_speed = 500.0f; // world px per second at zoom 1
    constexpr float key_zoom_rate = 1.8f;
    constexpr float wheel_zoom_step = 1.15f;
    constexpr float min_zoom = 0.15f;
    constexpr float max_zoom = 12.0f;

    // Default to the bundled island map (3 stacked tile layers + an object layer +
    // animated water). NEUTRINO_MAP_VIEWER_MAP can point at any other bundled map:
    //   assets/forest/forest.tmx           -- parallax object layers + animation
    //   assets/orthogonal-outside.tmx      -- a large two-layer outdoor map
    //   assets/isometric/isometric_grass_and_water.tmx -- isometric (diamond) map
    //   assets/hex/hexagonal-mini.tmx      -- hexagonal (staggered) map
    fs::path map_path() {
        if (const char* env = std::getenv("NEUTRINO_MAP_VIEWER_MAP")) {
            return fs::path{env};
        }
        return fs::path{NEUTRINO_MAP_VIEWER_ASSET_DIR} / "island" / "island.tmx";
    }

    // The TMX loader keeps external image sources as raw relative paths (with empty
    // bytes); build_bundle then decodes them via load_image() relative to the current
    // directory. Rewrite each to an absolute path against the map's directory so the
    // viewer runs from anywhere.
    void absolutize_image_paths(world& w, const fs::path& base) {
        auto fix = [&](world_image& img) {
            // Only a disk-backed image carries a rewritable path; memory/surface images
            // already hold their pixels.
            if (auto* disk = std::get_if <image_from_disk>(&img.source)) {
                if (!disk->source.empty() && disk->source.is_relative()) {
                    disk->source = (base / disk->source).lexically_normal();
                }
            }
        };
        for (world_tileset& ts : w.tilesets()) {
            if (ts.image) {
                fix(*ts.image);
            }
            for (world_tile& t : ts.tiles) {
                if (t.image) {
                    fix(*t.image);
                }
            }
        }
        for (world_layer& layer : w.layers()) {
            if (auto* image_layer = std::get_if <world_image_layer>(&layer)) {
                if (image_layer->image) {
                    fix(*image_layer->image);
                }
            }
        }
    }

    class map_viewer_scene final : public base_scene {
        public:
            void on_enter() override {
                const fs::path path = map_path();
                m_world = load_tmx_world_file(path);
                absolutize_image_paths(m_world, path.parent_path());
                m_renderer = std::make_unique <world_renderer>(m_world); // app-wide cache
                reset_camera();
                // vsync is on by default via application_config, so no tearing on pan.

                std::cerr << "map_viewer: loaded " << path.string() << " ("
                          << m_world.width() << "x" << m_world.height() << " cells)\n"
                          << "  WASD/arrows pan  -  wheel or +/- zoom  -  R reset  -  Esc quit\n";
            }

            void on_exit() override {
                m_renderer.reset(); // release bundles while the render services are still live
            }

            void update_physics(frame_duration dt) override {
                // Edge-triggered actions belong here (once per frame), not in
                // handle_action (once per event): a per-event pressed() check posts one
                // request per queued event, and popping the last scene more than once
                // trips the scenes_manager's non-empty enforcement.
                if (!m_quitting && hotkey{sdlpp::scancode::escape}.pressed()) {
                    m_quitting = true;
                    pop_scene();
                    return;
                }
                if (hotkey{sdlpp::scancode::r}.pressed()) {
                    reset_camera();
                }

                const float seconds = dt.count() / 1000.0f;
                const float pan = pan_speed * seconds / m_camera.zoom; // constant screen-space speed

                if (key_held(sdlpp::scancode::a, sdlpp::scancode::left)) {
                    m_camera.target.x -= pan;
                }
                if (key_held(sdlpp::scancode::d, sdlpp::scancode::right)) {
                    m_camera.target.x += pan;
                }
                if (key_held(sdlpp::scancode::w, sdlpp::scancode::up)) {
                    m_camera.target.y -= pan;
                }
                if (key_held(sdlpp::scancode::s, sdlpp::scancode::down)) {
                    m_camera.target.y += pan;
                }
                if (key_held(sdlpp::scancode::equals, sdlpp::scancode::kp_plus)) {
                    zoom_about(std::pow(key_zoom_rate, seconds), viewport_center());
                }
                if (key_held(sdlpp::scancode::minus, sdlpp::scancode::kp_minus)) {
                    zoom_about(std::pow(1.0f / key_zoom_rate, seconds), viewport_center());
                }
            }

            void render(frame_duration) override {
                clamp_camera(); // after this frame's pan (update_physics) and wheel zoom (handle_action)
                draw_rect_fill(rect{0, 0, window_width, window_height}, sdlpp::color{22, 22, 28, 255});
                const draw_stats stats = m_renderer->draw(m_camera, viewport());
                if (!m_logged_first_frame) {
                    m_logged_first_frame = true;
                    std::cerr << "map_viewer: first frame drew " << stats.drawn << " tiles ("
                              << stats.skipped << " skipped, " << stats.failed << " failed)\n";
                }
            }

            void handle_action(const sdlpp::event& ev) override {
                // Only genuinely event-carried input here (the wheel delta + cursor);
                // edge-triggered keys are handled once per frame in update_physics.
                if (const auto* wheel = ev.as <sdlpp::mouse_wheel_event>()) {
                    if (wheel->y != 0.0f) {
                        const float factor = wheel->y > 0.0f ? wheel_zoom_step : 1.0f / wheel_zoom_step;
                        zoom_about(factor, world_point{wheel->mouse_x, wheel->mouse_y});
                    }
                }
            }

            [[nodiscard]] bool is_opaque() const override {
                return true;
            }

        private:
            static bool key_held(sdlpp::scancode a, sdlpp::scancode b) {
                return hotkey{a}.held() || hotkey{b}.held();
            }

            [[nodiscard]] rect viewport() const {
                if (const auto size = get_renderer().get_output_size <dim>()) {
                    return rect{0, 0, size->width, size->height};
                }
                return rect{0, 0, window_width, window_height};
            }

            [[nodiscard]] world_point viewport_center() const {
                const rect v = viewport();
                return world_point{static_cast <float>(v.x) + static_cast <float>(v.w) * 0.5f,
                                   static_cast <float>(v.y) + static_cast <float>(v.h) * 0.5f};
            }

            // Zoom while keeping the world point currently under `screen` (a viewport
            // pixel) stationary -- so the wheel zooms toward the cursor and +/- zoom
            // toward the viewport centre. With a look-at camera the world under a screen
            // pixel is `target + (screen - centre)/zoom`, so holding it fixed shifts the
            // target by `(screen - centre)*(1/z0 - 1/z1)`.
            void zoom_about(float factor, world_point screen) {
                const float z0 = m_camera.zoom;
                const float z1 = std::clamp(z0 * factor, min_zoom, max_zoom);
                if (z1 == z0) {
                    return;
                }
                const rect v = viewport();
                const float k = 1.0f / z0 - 1.0f / z1;
                m_camera.zoom = z1;
                m_camera.target.x += (screen.x - static_cast <float>(v.w) * 0.5f) * k;
                m_camera.target.y += (screen.y - static_cast <float>(v.h) * 0.5f) * k;
            }

            // Keep the camera on the map. The view spans [target - half, target + half]
            // in world pixels; clamp the target so that stays inside [0, map], or centre
            // it on an axis where the view is larger than the map (nowhere to pan). This
            // prevents panning off the map, where the finite parallax bands would drift.
            void clamp_camera() {
                const rect v = viewport();
                const float map_w = static_cast <float>(m_world.width() * m_world.tile_width());
                const float map_h = static_cast <float>(m_world.height() * m_world.tile_height());
                const float half_w = static_cast <float>(v.w) / m_camera.zoom * 0.5f;
                const float half_h = static_cast <float>(v.h) / m_camera.zoom * 0.5f;
                m_camera.target.x = 2.0f * half_w >= map_w ? map_w * 0.5f
                                                           : std::clamp(m_camera.target.x, half_w, map_w - half_w);
                m_camera.target.y = 2.0f * half_h >= map_h ? map_h * 0.5f
                                                           : std::clamp(m_camera.target.y, half_h, map_h - half_h);
            }

            void reset_camera() {
                m_camera.zoom = 1.0f;
                m_camera.target = world_point{static_cast <float>(m_world.width() * m_world.tile_width()) * 0.5f,
                                              static_cast <float>(m_world.height() * m_world.tile_height()) * 0.5f};
                clamp_camera();
            }

            world m_world;
            std::unique_ptr <world_renderer> m_renderer;
            camera m_camera;
            bool m_logged_first_frame{false};
            bool m_quitting{false};
    };

    class map_viewer_app final : public application {
        protected:
            sdlpp::window_config get_window_config() override {
                return {
                    "Neutrino Map Viewer",
                    window_width,
                    window_height,
                    sdlpp::window_flags::resizable,
                    60
                };
            }

            std::unique_ptr <base_scene> create_initial_scene() override {
                return std::make_unique <map_viewer_scene>();
            }
    };
}

SDLPP_MAIN(map_viewer_app)
