#include <doctest/doctest.h>
#include <neutrino/video/sprite/cpu_texture_atlas.hh>
#include <neutrino/video/sprite/sprite_sheet.hh>
#include <neutrino/video/sprite/sprite_state.hh>
#include <neutrino/video/sprite/texture_atlas.hh>

#include "test_application.hh"

#include <chrono>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

TEST_SUITE("neutrino::video::sprite_state") {
    namespace {
        [[nodiscard]] neutrino::sprite_animation_frame frame(
            neutrino::sprite_flip flip,
            float milliseconds) {
            return neutrino::sprite_animation_frame{
                .appearance = {
                    .flip = flip,
                    .visible = true
                },
                .duration = neutrino::sprite_animation_duration{milliseconds}
            };
        }

        [[nodiscard]] neutrino::cpu_texture_atlas make_atlas() {
            auto surface = sdlpp::surface::create_rgb(4, 4, sdlpp::pixel_format_enum::RGBA8888);
            REQUIRE(surface.has_value());
            REQUIRE(surface->put_pixel(0, 0, sdlpp::colors::white).has_value());

            std::vector <neutrino::cpu_texture_atlas_frame> frames;
            frames.emplace_back(neutrino::rect{0, 0, 4, 4});
            return neutrino::cpu_texture_atlas(std::move(*surface), std::move(frames));
        }
    }

    TEST_CASE("sprite_state_id supports invalid handles and unordered containers") {
        const neutrino::sprite_state_id invalid;

        CHECK_FALSE(invalid.valid());

        std::unordered_set <neutrino::sprite_state_id> states;
        states.insert(invalid);
        states.insert(invalid);

        CHECK(states.size() == 1);
    }

    TEST_CASE("invalid sprite resource unregisters are no-ops") {
        CHECK_NOTHROW(neutrino::unregister_sprite_state(neutrino::sprite_state_id{}));
        CHECK_NOTHROW(neutrino::unregister_sprite_animation(neutrino::sprite_animation_id{}));
        CHECK_NOTHROW(neutrino::unregister_sprite_sheet(neutrino::sprite_sheet_id{}));
        CHECK_NOTHROW(neutrino::unregister_atlas(neutrino::gpu_texture_atlas_id{}));
    }

    TEST_CASE("sprite states resolve fixed appearances") {
        neutrino::test::test_application test_app("Sprite state fixed appearance test");

        const neutrino::sprite_appearance fixed{
            .flip = neutrino::sprite_flip::horizontal,
            .visible = false
        };
        const auto state = neutrino::create_sprite_state(fixed);

        auto resolved = neutrino::sprite_state_appearance(state);
        CHECK(resolved.flip == neutrino::sprite_flip::horizontal);
        CHECK_FALSE(resolved.visible);

        neutrino::set_sprite_state_appearance(state, neutrino::sprite_appearance{
            .flip = neutrino::sprite_flip::vertical,
            .visible = true
        });

        resolved = neutrino::sprite_state_appearance(state);
        CHECK(resolved.flip == neutrino::sprite_flip::vertical);
        CHECK(resolved.visible);
    }

    TEST_CASE("sprite states resolve animations advanced by the application loop") {
        neutrino::application_config cfg;
        cfg.title = "Sprite state animation test";
        cfg.width = 100;
        cfg.height = 100;
        cfg.flags = sdlpp::window_flags::hidden;
        cfg.target_fps = 0;
        neutrino::test::test_application test_app(cfg);

        const auto animation = neutrino::register_sprite_animation(neutrino::sprite_animation({
            frame(neutrino::sprite_flip::none, 1.0f),
            frame(neutrino::sprite_flip::horizontal, 50.0f)
        }, false));
        const auto state = neutrino::create_sprite_state(animation);

        CHECK(neutrino::sprite_state_appearance(state).flip == neutrino::sprite_flip::none);

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        test_app.iterate();

        CHECK(neutrino::sprite_state_appearance(state).flip == neutrino::sprite_flip::horizontal);

        neutrino::set_sprite_state_appearance(state, neutrino::sprite_appearance{
            .flip = neutrino::sprite_flip::vertical,
            .visible = true
        });
        CHECK(neutrino::sprite_state_appearance(state).flip == neutrino::sprite_flip::vertical);

        neutrino::set_sprite_state_animation(state, animation);
        CHECK(neutrino::sprite_state_appearance(state).flip == neutrino::sprite_flip::none);
    }

    TEST_CASE("sprite_state_finished reports completed non-looping animations") {
        neutrino::application_config cfg;
        cfg.title = "Sprite state finished test";
        cfg.width = 100;
        cfg.height = 100;
        cfg.flags = sdlpp::window_flags::hidden;
        cfg.target_fps = 0;
        neutrino::test::test_application test_app(cfg);

        const auto fixed = neutrino::create_sprite_state(neutrino::sprite_appearance{});
        CHECK_FALSE(neutrino::sprite_state_finished(fixed));

        const auto looping_animation = neutrino::register_sprite_animation(neutrino::sprite_animation({
            frame(neutrino::sprite_flip::none, 1.0f)
        }));
        const auto looping = neutrino::create_sprite_state(looping_animation);

        const auto one_shot_animation = neutrino::register_sprite_animation(neutrino::sprite_animation({
            frame(neutrino::sprite_flip::horizontal, 1.0f)
        }, false));
        const auto one_shot = neutrino::create_sprite_state(one_shot_animation);

        CHECK_FALSE(neutrino::sprite_state_finished(looping));
        CHECK_FALSE(neutrino::sprite_state_finished(one_shot));

        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        test_app.iterate();

        CHECK_FALSE(neutrino::sprite_state_finished(fixed));
        CHECK_FALSE(neutrino::sprite_state_finished(looping));
        CHECK(neutrino::sprite_state_finished(one_shot));

        neutrino::set_sprite_state_animation(one_shot, one_shot_animation);
        CHECK_FALSE(neutrino::sprite_state_finished(one_shot));

        neutrino::set_sprite_state_appearance(one_shot, neutrino::sprite_appearance{});
        CHECK_FALSE(neutrino::sprite_state_finished(one_shot));
    }

    TEST_CASE("sprite resource unregisters enforce dependency order") {
        neutrino::application_config cfg;
        cfg.title = "Sprite unregister dependency test";
        cfg.width = 100;
        cfg.height = 100;
        cfg.flags = sdlpp::window_flags::hidden;
        cfg.target_fps = 0;
        neutrino::test::test_application test_app(cfg);

        auto cpu_atlas = make_atlas();
        const auto atlas = neutrino::register_atlas(cpu_atlas, neutrino::atlas_texture_format::rgba);
        const auto sheet = neutrino::register_sprite_sheet(neutrino::sprite_sheet(atlas, cpu_atlas));
        const auto visual = neutrino::visual_ref(sheet, 0);

        const auto fixed_state = neutrino::create_sprite_state(neutrino::sprite_appearance{.visual = visual});
        CHECK_THROWS(neutrino::unregister_sprite_sheet(sheet));
        neutrino::unregister_sprite_state(fixed_state);

        const auto animation = neutrino::register_sprite_animation(neutrino::sprite_animation({
            neutrino::sprite_animation_frame{
                .appearance = neutrino::sprite_appearance{.visual = visual},
                .duration = neutrino::sprite_animation_duration{10.0f}
            }
        }));
        CHECK_THROWS(neutrino::unregister_sprite_sheet(sheet));

        const auto animated_state = neutrino::create_sprite_state(animation);
        CHECK_THROWS(neutrino::unregister_sprite_animation(animation));
        neutrino::unregister_sprite_state(animated_state);
        CHECK_NOTHROW(neutrino::unregister_sprite_animation(animation));

        CHECK_THROWS(neutrino::unregister_atlas(atlas));
        CHECK_NOTHROW(neutrino::unregister_sprite_sheet(sheet));
        CHECK_NOTHROW(neutrino::unregister_atlas(atlas));
    }
}
