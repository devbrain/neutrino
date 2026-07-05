//
// Created by igor on 05/07/2026.
//

#include <neutrino/application.hh>
#include <neutrino/input/hotkey.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/scene/scene_transitions.hh>
#include <neutrino/video/draw.hh>
#include <neutrino/video/sprites.hh>

#include <failsafe/enforce.hh>
#include <sdlpp/app/entry_point.hh>
#include <sdlpp/image/image.hh>
#include <sdlpp/video/pixels.hh>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    constexpr int window_width = 640;
    constexpr int window_height = 360;
    constexpr int ground_y = 268;

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

    [[nodiscard]] neutrino::cpu_texture_atlas load_demo_atlas() {
        std::vector <neutrino::cpu_texture_atlas_frame> frames;
        frames.reserve(demo_frames.size());
        for (const auto& frame : demo_frames) {
            frames.emplace_back(frame.rect);
        }

        return neutrino::cpu_texture_atlas(load_demo_sheet(), std::move(frames));
    }

    struct sprite_resources {
        neutrino::gpu_texture_atlas_id atlas;
        neutrino::sprite_sheet_id sheet;
        neutrino::sprite_animation_id walk;
        neutrino::sprite_animation_id jump;
        neutrino::sprite_animation_id torch;
        neutrino::sprite_animation_id coin;
        neutrino::sprite_state_id player;
        neutrino::sprite_state_id torch_state;
        neutrino::sprite_state_id coin_state;
        neutrino::sprite_appearance idle_right;
        neutrino::sprite_appearance idle_left;
    };

    [[nodiscard]] sprite_resources load_sprites() {
        auto cpu_atlas = load_demo_atlas();
        const auto atlas = neutrino::register_atlas(cpu_atlas, neutrino::atlas_texture_format::rgba);

        neutrino::sprite_sheet sheet(atlas);
        for (const auto& frame : demo_frames) {
            sheet.add_visual(
                std::string(frame.name),
                neutrino::sprite_visual{
                    .texture_rect = frame.rect,
                    .origin = frame.origin
                });
        }

        const auto sheet_id = neutrino::register_sprite_sheet(std::move(sheet));
        const auto walk = neutrino::register_sprite_animation(neutrino::make_sprite_animation(
            sheet_id,
            {"player.walk.0", "player.walk.1", "player.walk.2", "player.walk.1"},
            neutrino::sprite_animation_duration{90.0f}));
        const auto jump = neutrino::register_sprite_animation(neutrino::make_sprite_animation(
            sheet_id,
            {"player.jump", "player.idle"},
            neutrino::sprite_animation_duration{140.0f},
            false));
        const auto torch = neutrino::register_sprite_animation(neutrino::make_sprite_animation(
            sheet_id,
            {"spark.0", "spark.1", "spark.2", "spark.1"},
            neutrino::sprite_animation_duration{120.0f}));
        const auto coin = neutrino::register_sprite_animation(neutrino::make_sprite_animation(
            sheet_id,
            {"coin.0", "coin.1"},
            neutrino::sprite_animation_duration{160.0f}));

        auto idle_right = neutrino::make_sprite_appearance(sheet_id, "player.idle");
        auto idle_left = neutrino::make_sprite_appearance(
            sheet_id,
            "player.idle",
            neutrino::sprite_flip::horizontal);

        return sprite_resources{
            .atlas = atlas,
            .sheet = sheet_id,
            .walk = walk,
            .jump = jump,
            .torch = torch,
            .coin = coin,
            .player = neutrino::create_sprite_state(idle_right),
            .torch_state = neutrino::create_sprite_state(torch),
            .coin_state = neutrino::create_sprite_state(coin),
            .idle_right = idle_right,
            .idle_left = idle_left
        };
    }

    void unload_sprites(sprite_resources& sprites) {
        neutrino::unregister_sprite_state(sprites.player);
        neutrino::unregister_sprite_state(sprites.torch_state);
        neutrino::unregister_sprite_state(sprites.coin_state);
        neutrino::unregister_sprite_animation(sprites.walk);
        neutrino::unregister_sprite_animation(sprites.jump);
        neutrino::unregister_sprite_animation(sprites.torch);
        neutrino::unregister_sprite_animation(sprites.coin);
        neutrino::unregister_sprite_sheet(sprites.sheet);
        neutrino::unregister_atlas(sprites.atlas);
    }

    enum class player_mode {
        idle,
        walk,
        jump
    };

    class sprite_demo_scene final : public neutrino::base_scene {
        public:
            void on_enter() override {
                m_sprites = load_sprites();
            }

            void on_exit() override {
                unload_sprites(m_sprites);
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
                    m_player_x = std::clamp(m_player_x, 32.0f, static_cast <float>(window_width - 32));
                    m_facing_left = left;
                }

                if (jump_pressed && m_on_ground) {
                    m_on_ground = false;
                    m_player_vy = -210.0f;
                    m_mode = player_mode::jump;
                    neutrino::restart_sprite_animation(m_sprites.player, m_sprites.jump);
                }

                if (!m_on_ground) {
                    m_player_vy += 520.0f * seconds;
                    m_player_y += m_player_vy * seconds;
                    if (m_player_y >= ground_y) {
                        m_player_y = ground_y;
                        m_player_vy = 0.0f;
                        m_on_ground = true;
                        m_mode = player_mode::idle;
                    }
                }

                if (m_on_ground) {
                    if (left != right) {
                        m_mode = player_mode::walk;
                        neutrino::switch_sprite_animation(m_sprites.player, m_sprites.walk);
                    } else if (m_mode != player_mode::idle) {
                        m_mode = player_mode::idle;
                        neutrino::set_sprite_state_appearance(
                            m_sprites.player,
                            m_facing_left ? m_sprites.idle_left : m_sprites.idle_right);
                    }
                }
            }

            void render(neutrino::frame_duration) override {
                draw_background();

                neutrino::draw_sprite(neutrino::point{430, ground_y - 4}, m_sprites.torch_state);
                neutrino::draw_sprite(neutrino::point{470, ground_y - 4}, m_sprites.coin_state);

                const auto player_flip = m_facing_left ? neutrino::sprite_flip::horizontal : neutrino::sprite_flip::none;
                if (m_mode == player_mode::walk || m_mode == player_mode::jump) {
                    neutrino::draw_sprite(
                        neutrino::point{static_cast <int>(m_player_x), static_cast <int>(m_player_y)},
                        neutrino::sprite_state_appearance(m_sprites.player).visual,
                        player_flip);
                } else {
                    neutrino::draw_sprite(
                        neutrino::point{static_cast <int>(m_player_x), static_cast <int>(m_player_y)},
                        m_sprites.player);
                }
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

            sprite_resources m_sprites;
            float m_player_x{120.0f};
            float m_player_y{static_cast <float>(ground_y)};
            float m_player_vy{0.0f};
            bool m_on_ground{true};
            bool m_facing_left{false};
            player_mode m_mode{player_mode::idle};
    };

    class sprite_demo_app final : public neutrino::application {
        protected:
            sdlpp::window_config get_window_config() override {
                return {
                    "Neutrino Sprite Demo",
                    window_width,
                    window_height,
                    sdlpp::window_flags::resizable,
                    60
                };
            }

            std::unique_ptr <neutrino::base_scene> create_initial_scene() override {
                return std::make_unique <sprite_demo_scene>();
            }
    };
}

SDLPP_MAIN(sprite_demo_app)
