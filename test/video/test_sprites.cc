#include <doctest/doctest.h>
#include <neutrino/video/sprites.hh>

#include "test_application.hh"

#include <initializer_list>
#include <utility>
#include <vector>

TEST_SUITE("neutrino::video::sprites") {
    namespace {
        struct registered_sheet {
            neutrino::gpu_texture_atlas_id atlas;
            neutrino::sprite_sheet_id sheet;
        };

        [[nodiscard]] neutrino::cpu_texture_atlas make_cpu_atlas() {
            auto surface = sdlpp::surface::create_rgb(24, 8, sdlpp::pixel_format_enum::RGBA8888);
            REQUIRE(surface.has_value());

            std::vector <neutrino::cpu_texture_atlas_frame> frames;
            frames.emplace_back(neutrino::rect{0, 0, 8, 8});
            frames.emplace_back(neutrino::rect{8, 0, 8, 8});
            frames.emplace_back(neutrino::rect{16, 0, 8, 8});
            return neutrino::cpu_texture_atlas(std::move(*surface), std::move(frames));
        }

        [[nodiscard]] registered_sheet make_registered_sheet() {
            auto cpu_atlas = make_cpu_atlas();
            const auto atlas = neutrino::register_atlas(cpu_atlas, neutrino::atlas_texture_format::rgba);
            neutrino::sprite_sheet sheet(atlas, cpu_atlas);
            sheet.bind("idle.0", sheet.visual_id(0));
            sheet.bind("run.0", sheet.visual_id(1));
            sheet.bind("run.1", sheet.visual_id(2));
            return registered_sheet{atlas, neutrino::register_sprite_sheet(std::move(sheet))};
        }

        void unregister(registered_sheet registered) {
            neutrino::unregister_sprite_sheet(registered.sheet);
            neutrino::unregister_atlas(registered.atlas);
        }
    }

    TEST_CASE("sprite facade builds appearances from visual references, indexes, and names") {
        neutrino::test::test_application test_app("Sprite facade appearance test");
        const auto registered = make_registered_sheet();
        const auto sheet = registered.sheet;
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
        unregister(registered);
    }

    TEST_CASE("sprite facade builds animation frames and constant-duration animations") {
        neutrino::test::test_application test_app("Sprite facade animation test");
        const auto registered = make_registered_sheet();
        const auto sheet = registered.sheet;

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

        unregister(registered);
    }

    TEST_CASE("sprite facade builds animations from sheet indexes, names, and ranges") {
        neutrino::test::test_application test_app("Sprite facade sheet animation test");
        const auto registered = make_registered_sheet();
        const auto sheet = registered.sheet;

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

        unregister(registered);
    }
}
