#include <doctest/doctest.h>
#include <neutrino/video/sprites.hh>

#include "test_application.hh"

#include <initializer_list>
#include <utility>

TEST_SUITE("neutrino::video::sprites") {
    namespace {
        [[nodiscard]] neutrino::sprite_sheet_id make_registered_sheet() {
            neutrino::sprite_sheet sheet(neutrino::gpu_texture_atlas_id{});
            sheet.add_visual("idle.0", neutrino::sprite_visual{.texture_rect = {0, 0, 8, 8}});
            sheet.add_visual("run.0", neutrino::sprite_visual{.texture_rect = {8, 0, 8, 8}});
            sheet.add_visual("run.1", neutrino::sprite_visual{.texture_rect = {16, 0, 8, 8}});
            return neutrino::register_sprite_sheet(std::move(sheet));
        }
    }

    TEST_CASE("sprite facade builds appearances from visual references, indexes, and names") {
        neutrino::test::test_application test_app("Sprite facade appearance test");
        const auto sheet = make_registered_sheet();
        const auto idle = neutrino::visual_ref(sheet, "idle.0");

        const auto direct = neutrino::make_sprite_appearance(
            idle,
            neutrino::sprite_flip::horizontal,
            false);
        CHECK(direct.visual == idle);
        CHECK(direct.flip == neutrino::sprite_flip::horizontal);
        CHECK_FALSE(direct.visible);

        const auto by_index = neutrino::make_sprite_appearance(sheet, 1, neutrino::sprite_flip::vertical);
        CHECK(by_index.visual == neutrino::visual_ref(sheet, 1));
        CHECK(by_index.flip == neutrino::sprite_flip::vertical);
        CHECK(by_index.visible);

        const auto by_name = neutrino::make_sprite_appearance(sheet, "run.1");
        CHECK(by_name.visual == neutrino::visual_ref(sheet, "run.1"));
        CHECK(by_name.visible);

        CHECK_THROWS(neutrino::visual_ref(sheet, "missing"));
        neutrino::unregister_sprite_sheet(sheet);
    }

    TEST_CASE("sprite facade builds animation frames and constant-duration animations") {
        neutrino::test::test_application test_app("Sprite facade animation test");
        const auto sheet = make_registered_sheet();

        const auto idle = neutrino::make_sprite_appearance(sheet, "idle.0");
        const auto run = neutrino::make_sprite_appearance(
            sheet,
            "run.0",
            neutrino::sprite_flip::horizontal);

        const auto single = neutrino::make_sprite_animation_frame(
            idle,
            neutrino::sprite_animation_duration{75.0f});
        CHECK(single.appearance.visual == idle.visual);
        CHECK(single.duration.count() == doctest::Approx(75.0f));

        const auto animation = neutrino::make_sprite_animation(
            {idle, run},
            neutrino::sprite_animation_duration{50.0f},
            false);
        CHECK_FALSE(animation.loop());
        REQUIRE(animation.frame_count() == 2);
        CHECK(animation.total_duration().count() == doctest::Approx(100.0f));
        CHECK(animation.frame(0).appearance.visual == idle.visual);
        CHECK(animation.frame(1).appearance.flip == neutrino::sprite_flip::horizontal);

        const auto variable = neutrino::make_sprite_animation({
            neutrino::make_sprite_animation_frame(idle, neutrino::sprite_animation_duration{10.0f}),
            neutrino::make_sprite_animation_frame(run, neutrino::sprite_animation_duration{20.0f})
        });
        REQUIRE(variable.frame_count() == 2);
        CHECK(variable.loop());
        CHECK(variable.total_duration().count() == doctest::Approx(30.0f));

        CHECK_THROWS(neutrino::make_sprite_animation(
            std::initializer_list <neutrino::sprite_appearance>{},
            neutrino::sprite_animation_duration{10.0f}));
        CHECK_THROWS(neutrino::make_sprite_animation(
            {idle},
            neutrino::sprite_animation_duration{0.0f}));

        neutrino::unregister_sprite_sheet(sheet);
    }

    TEST_CASE("sprite facade builds animations from sheet indexes, names, and ranges") {
        neutrino::test::test_application test_app("Sprite facade sheet animation test");
        const auto sheet = make_registered_sheet();

        const auto by_index = neutrino::make_sprite_animation(
            sheet,
            {0u, 1u, 2u},
            neutrino::sprite_animation_duration{30.0f},
            true,
            neutrino::sprite_flip::vertical);
        REQUIRE(by_index.frame_count() == 3);
        CHECK(by_index.frame(0).appearance.visual == neutrino::visual_ref(sheet, 0));
        CHECK(by_index.frame(2).appearance.visual == neutrino::visual_ref(sheet, 2));
        CHECK(by_index.frame(1).appearance.flip == neutrino::sprite_flip::vertical);

        const auto by_name = neutrino::make_sprite_animation(
            sheet,
            {"run.0", "run.1"},
            neutrino::sprite_animation_duration{40.0f},
            false);
        CHECK_FALSE(by_name.loop());
        REQUIRE(by_name.frame_count() == 2);
        CHECK(by_name.frame(0).appearance.visual == neutrino::visual_ref(sheet, "run.0"));
        CHECK(by_name.frame(1).appearance.visual == neutrino::visual_ref(sheet, "run.1"));

        const auto by_range = neutrino::make_sprite_animation_range(
            sheet,
            1,
            2,
            neutrino::sprite_animation_duration{60.0f});
        REQUIRE(by_range.frame_count() == 2);
        CHECK(by_range.frame(0).appearance.visual == neutrino::visual_ref(sheet, 1));
        CHECK(by_range.frame(1).appearance.visual == neutrino::visual_ref(sheet, 2));

        CHECK_THROWS(neutrino::make_sprite_animation_range(
            sheet,
            0,
            0,
            neutrino::sprite_animation_duration{60.0f}));
        CHECK_THROWS(neutrino::make_sprite_animation(
            sheet,
            std::initializer_list <std::size_t>{},
            neutrino::sprite_animation_duration{60.0f}));

        neutrino::unregister_sprite_sheet(sheet);
    }
}
