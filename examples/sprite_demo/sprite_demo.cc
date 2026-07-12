//
// Created by igor on 05/07/2026.
//

#include <neutrino/application.hh>
#include <neutrino/input/hotkey.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/scene/scene_transitions.hh>
#include <neutrino/video/draw.hh>
#include <neutrino/video/sprite/sprite_cache.hh>
#include <neutrino/video/sprite/sprite_def.hh>

#include <failsafe/enforce.hh>
#include <sdlpp/app/entry_point.hh>
#include <sdlpp/image/image.hh>
#include <sdlpp/video/pixels.hh>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    constexpr int window_width = 640;
    constexpr int window_height = 360;
    constexpr int ground_y = 268;
    constexpr float player_scale = 3.0f;
    constexpr float prop_scale = 3.0f;
    constexpr float player_half_width = 16.0f * player_scale;

    struct named_frame {
        std::string_view name;
        neutrino::rect rect;
        neutrino::point origin;
    };

    const std::vector <named_frame> demo_frames{
        {"player.idle", {0, 0, 32, 32}, {16, 32}},
        {"player.walk.0", {0, 0, 32, 32}, {16, 32}},
        {"player.walk.1", {32, 0, 32, 32}, {16, 32}},
        {"player.walk.2", {64, 0, 32, 32}, {16, 32}},
        {"player.jump", {0, 32, 32, 32}, {16, 32}},
        {"spark.0", {192, 0, 16, 16}, {8, 16}},
        {"spark.1", {192, 16, 16, 16}, {8, 16}},
        {"spark.2", {192, 32, 16, 16}, {8, 16}},
        {"coin.0", {80, 112, 16, 16}, {8, 16}},
        {"coin.1", {96, 112, 16, 16}, {8, 16}}
    };

    [[nodiscard]] std::filesystem::path demo_asset_path() {
        return std::filesystem::path{NEUTRINO_SPRITE_DEMO_ASSET_DIR} / "arcade_platformerV2.png";
    }

    [[nodiscard]] sdlpp::surface load_demo_sheet() {
        auto loaded = sdlpp::image::load(demo_asset_path());
        ENFORCE(loaded.has_value());

        auto converted = loaded->convert(sdlpp::pixel_format_enum::RGBA8888);
        ENFORCE(converted.has_value());

        auto matte = converted->get_pixel(0, 0);
        ENFORCE(matte.has_value());

        for (int y = 0; y < converted->height(); ++y) {
            for (int x = 0; x < converted->width(); ++x) {
                auto pixel = converted->get_pixel(x, y);
                ENFORCE(pixel.has_value());

                if (pixel->r == matte->r && pixel->g == matte->g && pixel->b == matte->b) {
                    pixel->a = 0;
                    auto written = converted->put_pixel(x, y, *pixel);
                    ENFORCE(written.has_value());
                }
            }
        }

        return std::move(*converted);
    }

    // Frame duration helper for readability.
    [[nodiscard]] neutrino::sprite_animation_duration ms(float v) {
        return neutrino::sprite_animation_duration{v};
    }

    [[nodiscard]] neutrino::sprite_frame_def frame(std::string_view visual, float duration) {
        return neutrino::sprite_frame_def{std::string(visual), ms(duration), neutrino::sprite_flip::none};
    }

    // The whole sprite asset as pure data: the matted sheet as the atlas image, the named
    // frames as explicit visuals, and the animations as named clips. build_sprite_set (via
    // the cache) turns this into registered resources -- no manual register/unregister.
    [[nodiscard]] neutrino::sprite_def make_demo_def() {
        auto sheet = load_demo_sheet();

        neutrino::sprite_def def;
        def.image.width = static_cast <unsigned>(sheet.get()->w);
        def.image.height = static_cast <unsigned>(sheet.get()->h);
        def.image.source = neutrino::image_from_surface{
            std::make_shared <const sdlpp::surface>(std::move(sheet)), std::nullopt};

        def.visuals.reserve(demo_frames.size());
        for (const auto& f : demo_frames) {
            def.visuals.push_back(neutrino::sprite_visual_def{
                std::string(f.name), f.rect, f.origin, std::nullopt, std::nullopt});
        }

        def.clips = {
            neutrino::sprite_clip_def{"idle", {frame("player.idle", 1000.0f)}, true},
            neutrino::sprite_clip_def{"walk",
                {frame("player.walk.0", 90.0f), frame("player.walk.1", 90.0f),
                 frame("player.walk.2", 90.0f), frame("player.walk.1", 90.0f)}, true},
            neutrino::sprite_clip_def{"jump",
                {frame("player.jump", 140.0f), frame("player.idle", 140.0f)}, /*loop=*/false},
            neutrino::sprite_clip_def{"torch",
                {frame("spark.0", 120.0f), frame("spark.1", 120.0f),
                 frame("spark.2", 120.0f), frame("spark.1", 120.0f)}, true},
            neutrino::sprite_clip_def{"coin",
                {frame("coin.0", 160.0f), frame("coin.1", 160.0f)}, true},
        };
        return def;
    }

    enum class player_mode {
        idle,
        walk,
        jump
    };

    class sprite_demo_scene final : public neutrino::base_scene {
        public:
            void on_enter() override {
                // Data -> one acquire -> spawn per actor. No manual register/unregister:
                // the instances (states + leases) and the handle release by RAII on exit.
                m_set = m_cache.acquire(make_demo_def());
                m_player = m_set.spawn("idle");
                m_torch = m_set.spawn("torch");
                m_coin = m_set.spawn("coin");
            }

            void update_physics(neutrino::frame_duration dt) override {
                const float seconds = dt.count() / 1000.0f;
                const bool left = neutrino::hotkey{sdlpp::scancode::left}.held()
                    || neutrino::hotkey{sdlpp::scancode::a}.held();
                const bool right = neutrino::hotkey{sdlpp::scancode::right}.held()
                    || neutrino::hotkey{sdlpp::scancode::d}.held();
                const bool jump_pressed = neutrino::hotkey{sdlpp::scancode::space}.pressed()
                    || neutrino::hotkey{sdlpp::scancode::up}.pressed();

                if (left != right) {
                    m_player_x += (right ? 1.0f : -1.0f) * 90.0f * seconds;
                    m_player_x = std::clamp(m_player_x, player_half_width, static_cast <float>(window_width) - player_half_width);
                    m_facing_left = left;
                }

                if (jump_pressed && m_on_ground) {
                    m_on_ground = false;
                    m_player_vy = -210.0f;
                    m_mode = player_mode::jump;
                    m_player.restart("jump");
                }

                if (!m_on_ground) {
                    m_player_vy += 520.0f * seconds;
                    m_player_y += m_player_vy * seconds;
                    if (m_player_y >= ground_y) {
                        m_player_y = ground_y;
                        m_player_vy = 0.0f;
                        m_on_ground = true;
                    }
                }

                if (m_on_ground) {
                    if (left != right) {
                        m_mode = player_mode::walk;
                        m_player.switch_to("walk");
                    } else if (m_mode != player_mode::idle) {
                        m_mode = player_mode::idle;
                        m_player.switch_to("idle");
                    }
                }
            }

            void render(neutrino::frame_duration) override {
                draw_background();

                neutrino::draw_sprite(neutrino::point{430, ground_y - 4}, m_torch.state(), {.scale = prop_scale});
                neutrino::draw_sprite(neutrino::point{470, ground_y - 4}, m_coin.state(), {.scale = prop_scale});

                const auto player_flip = m_facing_left ? neutrino::sprite_flip::horizontal : neutrino::sprite_flip::none;
                neutrino::draw_sprite(
                    neutrino::point{static_cast <int>(m_player_x), static_cast <int>(m_player_y)},
                    m_player.state(),
                    {.scale = player_scale, .flip = player_flip});
            }

            void handle_action(const sdlpp::event&) override {
                if (neutrino::hotkey{sdlpp::scancode::escape}.pressed()) {
                    neutrino::pop_scene();
                }
            }

            [[nodiscard]] bool is_opaque() const override {
                return true;
            }

        private:
            static void draw_background() {
                neutrino::draw_rect_fill(neutrino::rect{0, 0, window_width, window_height}, sdlpp::color{118, 196, 255, 255});
                neutrino::draw_rect_fill(neutrino::rect{0, ground_y + 8, window_width, window_height - ground_y - 8}, sdlpp::color{60, 150, 86, 255});
                neutrino::draw_rect_fill(neutrino::rect{0, ground_y - 4, window_width, 12}, sdlpp::color{72, 190, 100, 255});
                neutrino::draw_rect_fill(neutrino::rect{390, ground_y - 20, 72, 24}, sdlpp::color{116, 82, 46, 255});
                neutrino::draw_rect_fill(neutrino::rect{418, ground_y - 48, 16, 32}, sdlpp::color{86, 55, 32, 255});
            }

            // Declared cache -> handle -> instances, so destruction runs in reverse:
            // instances (unregister states, drop leases), then the handle, then the cache.
            neutrino::sprite_cache m_cache;
            neutrino::sprite_set_handle m_set;
            neutrino::sprite_instance m_player;
            neutrino::sprite_instance m_torch;
            neutrino::sprite_instance m_coin;
            float m_player_x{120.0f};
            float m_player_y{static_cast <float>(ground_y)};
            float m_player_vy{0.0f};
            bool m_on_ground{true};
            bool m_facing_left{false};
            player_mode m_mode{player_mode::idle};
    };

    class sprite_demo_app final : public neutrino::application {
        public:
            sprite_demo_app()
                : neutrino::application(make_config()) {
            }

        protected:
            std::unique_ptr <neutrino::base_scene> create_initial_scene() override {
                return std::make_unique <sprite_demo_scene>();
            }

        private:
            static neutrino::application_config make_config() {
                neutrino::application_config cfg;
                cfg.title = "Neutrino Sprite Demo";
                cfg.width = window_width;
                cfg.height = window_height;
                cfg.flags = sdlpp::window_flags::resizable;
                // Fixed 640x360 design resolution, integer-scaled to the window: the
                // demo always draws in these coordinates and a larger window scales
                // it up crisply (pixel-art friendly) instead of showing more.
                cfg.logical_size = neutrino::dim{window_width, window_height};
                cfg.scale = neutrino::scale_mode::integer_scale;
                return cfg;
            }
    };
}

SDLPP_MAIN(sprite_demo_app)
