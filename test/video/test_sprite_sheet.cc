#include <doctest/doctest.h>
#include <neutrino/video/sprite/sprite_sheet.hh>

#include <unordered_set>
#include <utility>
#include <vector>

TEST_SUITE("neutrino::video::sprite_sheet") {
    TEST_CASE("sprite_sheet stores visuals over one atlas") {
        const neutrino::atlas_id atlas;
        neutrino::sprite_sheet sheet(atlas);

        const auto id = sheet.add_visual(neutrino::sprite_visual{
            .texture_rect = {1, 2, 5, 6},
            .origin = {3, 4}
        });

        REQUIRE(id.valid());
        CHECK(sheet.contains(id));
        CHECK(sheet.visual_count() == 1);
        CHECK(sheet.atlas() == atlas);

        const auto& visual = sheet.visual(id);
        CHECK(visual.texture_rect == neutrino::rect{1, 2, 5, 6});
        CHECK(visual.origin == neutrino::point{3, 4});
    }

    TEST_CASE("sprite_sheet keeps names as sheet-level lookup metadata") {
        neutrino::sprite_sheet sheet(neutrino::atlas_id{});

        const auto idle = sheet.add_visual(neutrino::sprite_visual{.texture_rect = {0, 0, 8, 8}});
        const auto run = sheet.add_visual("hero.run.0", neutrino::sprite_visual{.texture_rect = {8, 0, 8, 8}});

        CHECK_FALSE(sheet.find("hero.idle.0").has_value());

        sheet.bind("hero.idle.0", idle);
        auto found_idle = sheet.find("hero.idle.0");
        REQUIRE(found_idle.has_value());
        CHECK(*found_idle == idle);

        auto found_run = sheet.find("hero.run.0");
        REQUIRE(found_run.has_value());
        CHECK(*found_run == run);

        sheet.bind("hero.idle.0", run);
        found_idle = sheet.find("hero.idle.0");
        REQUIRE(found_idle.has_value());
        CHECK(*found_idle == run);
    }

    TEST_CASE("sprite_visual_id supports invalid handles and unordered containers") {
        neutrino::sprite_sheet sheet(neutrino::atlas_id{});

        const neutrino::sprite_visual_id invalid;
        CHECK_FALSE(invalid.valid());
        CHECK_FALSE(sheet.contains(invalid));

        const auto first = sheet.add_visual(neutrino::sprite_visual{.texture_rect = {0, 0, 8, 8}});
        const auto second = sheet.add_visual(neutrino::sprite_visual{.texture_rect = {8, 0, 8, 8}});
        CHECK(first != second);

        std::unordered_set <neutrino::sprite_visual_id> ids;
        ids.insert(invalid);
        ids.insert(invalid);
        ids.insert(first);
        ids.insert(first);
        ids.insert(second);

        CHECK(ids.size() == 3);
    }

    TEST_CASE("sprite visual ids are tied to the sheet that minted them") {
        neutrino::sprite_sheet first_sheet(neutrino::atlas_id{});
        neutrino::sprite_sheet second_sheet(neutrino::atlas_id{});

        const auto first_visual = first_sheet.add_visual(neutrino::sprite_visual{.texture_rect = {0, 0, 8, 8}});
        const auto second_visual = second_sheet.add_visual(neutrino::sprite_visual{.texture_rect = {0, 0, 8, 8}});

        CHECK(first_sheet.contains(first_visual));
        CHECK(second_sheet.contains(second_visual));
        CHECK_FALSE(second_sheet.contains(first_visual));
        CHECK_FALSE(first_sheet.contains(second_visual));
    }

    TEST_CASE("registered sprite handles support invalid values and unordered containers") {
        const neutrino::sprite_sheet_id invalid_sheet;
        const neutrino::sprite_visual_ref invalid_ref;

        CHECK_FALSE(invalid_sheet.valid());
        CHECK_FALSE(invalid_ref.valid());

        std::unordered_set <neutrino::sprite_sheet_id> sheets;
        sheets.insert(invalid_sheet);
        sheets.insert(invalid_sheet);

        std::unordered_set <neutrino::sprite_visual_ref> visuals;
        visuals.insert(invalid_ref);
        visuals.insert(invalid_ref);

        CHECK(sheets.size() == 1);
        CHECK(visuals.size() == 1);
    }

    TEST_CASE("sprite_sheet gets one whole-picture visual per atlas picture") {
        neutrino::image_bank bank;
        for (const neutrino::dim size : {neutrino::dim{4, 5}, neutrino::dim{6, 7}}) {
            auto surface = sdlpp::surface::create_rgb(size.width, size.height, sdlpp::pixel_format_enum::RGBA8888);
            REQUIRE(surface.has_value());
            bank.entries.push_back({std::move(*surface), 0});
        }

        const neutrino::image_atlas cpu_atlas = neutrino::build_atlas(bank);
        neutrino::sprite_sheet sheet(cpu_atlas);

        CHECK(sheet.atlas() == cpu_atlas.id());
        REQUIRE(sheet.visual_count() == 2);

        const auto first_id = sheet.visual_id(0);
        const auto second_id = sheet.visual_id(1);
        REQUIRE(first_id.valid());
        REQUIRE(second_id.valid());

        CHECK(sheet.visual(first_id).texture_rect == neutrino::rect{0, 0, 4, 5});
        CHECK(sheet.visual(first_id).picture == 0);
        CHECK(sheet.visual(first_id).origin == neutrino::point{0, 0});
        CHECK(sheet.visual(second_id).texture_rect == neutrino::rect{0, 0, 6, 7});
        CHECK(sheet.visual(second_id).picture == 1);
        CHECK(sheet.visual(second_id).origin == neutrino::point{0, 0});
        CHECK_FALSE(sheet.find("missing").has_value());
    }
}
