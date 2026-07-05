#include <doctest/doctest.h>
#include <neutrino/video/sprite/sprite_animation.hh>

#include <chrono>
#include <vector>

TEST_SUITE("neutrino::video::sprite_animation") {
    namespace {
        [[nodiscard]] neutrino::sprite_animation_frame frame(
            neutrino::sprite_flip flip,
            float milliseconds,
            bool visible = true) {
            return neutrino::sprite_animation_frame{
                .appearance = {
                    .flip = flip,
                    .visible = visible
                },
                .duration = neutrino::sprite_animation_duration{milliseconds}
            };
        }
    }

    TEST_CASE("sprite_animation stores frames and total duration") {
        neutrino::sprite_animation animation(false);

        animation.add_frame(frame(neutrino::sprite_flip::none, 100.0f));
        animation.add_frame(frame(neutrino::sprite_flip::horizontal, 250.0f));

        CHECK_FALSE(animation.loop());
        CHECK_FALSE(animation.empty());
        CHECK(animation.frame_count() == 2);
        CHECK(animation.total_duration().count() == doctest::Approx(350.0f));
        CHECK(animation.frame(0).appearance.flip == neutrino::sprite_flip::none);
        CHECK(animation.frame(1).appearance.flip == neutrino::sprite_flip::horizontal);
    }

    TEST_CASE("non-looping sprite_animation clamps before and after range") {
        neutrino::sprite_animation animation({
            frame(neutrino::sprite_flip::none, 100.0f),
            frame(neutrino::sprite_flip::horizontal, 50.0f),
            frame(neutrino::sprite_flip::vertical, 25.0f)
        }, false);

        CHECK(animation.appearance_at(neutrino::sprite_animation_duration{-10.0f}).flip == neutrino::sprite_flip::none);
        CHECK(animation.appearance_at(neutrino::sprite_animation_duration{0.0f}).flip == neutrino::sprite_flip::none);
        CHECK(animation.appearance_at(neutrino::sprite_animation_duration{99.0f}).flip == neutrino::sprite_flip::none);
        CHECK(animation.appearance_at(neutrino::sprite_animation_duration{100.0f}).flip == neutrino::sprite_flip::horizontal);
        CHECK(animation.appearance_at(neutrino::sprite_animation_duration{150.0f}).flip == neutrino::sprite_flip::vertical);
        CHECK(animation.appearance_at(neutrino::sprite_animation_duration{1000.0f}).flip == neutrino::sprite_flip::vertical);
    }

    TEST_CASE("looping sprite_animation wraps elapsed time") {
        neutrino::sprite_animation animation({
            frame(neutrino::sprite_flip::none, 100.0f),
            frame(neutrino::sprite_flip::horizontal, 50.0f)
        });

        REQUIRE(animation.loop());
        CHECK(animation.appearance_at(neutrino::sprite_animation_duration{0.0f}).flip == neutrino::sprite_flip::none);
        CHECK(animation.appearance_at(neutrino::sprite_animation_duration{149.0f}).flip == neutrino::sprite_flip::horizontal);
        CHECK(animation.appearance_at(neutrino::sprite_animation_duration{150.0f}).flip == neutrino::sprite_flip::none);
        CHECK(animation.appearance_at(neutrino::sprite_animation_duration{250.0f}).flip == neutrino::sprite_flip::horizontal);
    }

    TEST_CASE("sprite_animation preserves frame visibility in appearances") {
        neutrino::sprite_animation animation({
            frame(neutrino::sprite_flip::none, 100.0f, false)
        });

        const auto appearance = animation.appearance_at(neutrino::sprite_animation_duration{0.0f});
        CHECK_FALSE(appearance.visible);
        CHECK(appearance.flip == neutrino::sprite_flip::none);
    }
}
