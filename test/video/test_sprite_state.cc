#include <doctest/doctest.h>
#include <neutrino/video/sprite/sprite_state.hh>

#include "test_application.hh"

#include <chrono>
#include <thread>
#include <unordered_set>

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
    }

    TEST_CASE("sprite_state_id supports invalid handles and unordered containers") {
        const neutrino::sprite_state_id invalid;

        CHECK_FALSE(invalid.valid());

        std::unordered_set <neutrino::sprite_state_id> states;
        states.insert(invalid);
        states.insert(invalid);

        CHECK(states.size() == 1);
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
}
