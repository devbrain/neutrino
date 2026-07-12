#include <doctest/doctest.h>

#include "video/test_images.hh"

#include <neutrino/video/sprite/sprite_def.hh>
#include <neutrino/video/sprite/image_identity.hh>

#include <optional>
#include <utility>
#include <vector>

using namespace neutrino;
using namespace neutrino::test;

namespace {
    // A small def: a 2x2 grid over a 32x32 image plus one clip.
    sprite_def basic_def() {
        sprite_def d;
        d.image = mem_image(32, 32, {1, 2, 3, 4});
        d.grid = sprite_grid{16, 16, /*columns=*/2, /*count=*/4, /*margin=*/0, /*spacing=*/0,
                             sprite_origin_rule::top_left};
        d.clips = {
            sprite_clip_def{"idle",
                            {sprite_frame_def{"0", sprite_animation_duration{100.0f}, sprite_flip::none},
                             sprite_frame_def{"1", sprite_animation_duration{100.0f}, sprite_flip::none}},
                            true}
        };
        return d;
    }
}

TEST_SUITE("neutrino::video sprite_def") {
    TEST_CASE("origin_for picks the pivot for each rule") {
        CHECK(origin_for(sprite_origin_rule::top_left, dim{16, 8}).x == 0);
        CHECK(origin_for(sprite_origin_rule::top_left, dim{16, 8}).y == 0);
        CHECK(origin_for(sprite_origin_rule::center, dim{16, 8}).x == 8);
        CHECK(origin_for(sprite_origin_rule::center, dim{16, 8}).y == 4);
        CHECK(origin_for(sprite_origin_rule::bottom_center, dim{16, 8}).x == 8);
        CHECK(origin_for(sprite_origin_rule::bottom_center, dim{16, 8}).y == 8);
    }

    TEST_CASE("expand_grid slices row-major with derived counts and rule origins") {
        SUBCASE("explicit columns, derived count from image height") {
            const sprite_grid g{16, 16, 4, 0, 0, 0, sprite_origin_rule::center};
            const auto v = expand_grid(g, dim{64, 32}); // 4 cols x 2 rows
            REQUIRE(v.size() == 8);
            CHECK(v[0].name == "0");
            CHECK(v[0].src.x == 0);
            CHECK(v[0].src.y == 0);
            CHECK(v[0].src.w == 16);
            CHECK(v[5].name == "5");         // row 1, col 1
            CHECK(v[5].src.x == 16);
            CHECK(v[5].src.y == 16);
            CHECK(v[0].origin.x == 8);        // center of a 16x16 cell
            CHECK(v[0].origin.y == 8);
        }

        SUBCASE("columns and count both derived from the image") {
            const auto v = expand_grid(sprite_grid{16, 16}, dim{64, 32});
            CHECK(v.size() == 8);            // 4x2
        }

        SUBCASE("margin and spacing shift the cell rects") {
            const sprite_grid g{16, 16, 2, 2, /*margin=*/1, /*spacing=*/2, sprite_origin_rule::top_left};
            const auto v = expand_grid(g, dim{36, 20});
            REQUIRE(v.size() == 2);
            CHECK(v[0].src.x == 1);          // margin
            CHECK(v[1].src.x == 1 + 16 + 2); // margin + cell + spacing == 19
        }

        SUBCASE("a zero cell size yields nothing") {
            CHECK(expand_grid(sprite_grid{0, 16, 1, 1}, dim{64, 64}).empty());
        }
    }

    TEST_CASE("key_for is stable for identical defs and sensitive to every field") {
        image_identifier ident;
        const sprite_def d = basic_def();
        const content_key base = key_for(d, ident);
        CHECK(key_for(d, ident) == base); // identical -> equal

        SUBCASE("a changed clip frame duration") {
            sprite_def d2 = d;
            d2.clips[0].frames[0].duration = sprite_animation_duration{120.0f};
            CHECK(key_for(d2, ident) != base);
        }
        SUBCASE("a changed clip loop flag") {
            sprite_def d2 = d;
            d2.clips[0].loop = false;
            CHECK(key_for(d2, ident) != base);
        }
        SUBCASE("an added explicit visual") {
            sprite_def d2 = d;
            d2.visuals.push_back(sprite_visual_def{"hat", rect{0, 0, 8, 8}, point{4, 4}, std::nullopt, std::nullopt});
            CHECK(key_for(d2, ident) != base);
        }
        SUBCASE("a changed grid origin rule") {
            sprite_def d2 = d;
            d2.grid->origin = sprite_origin_rule::bottom_center;
            CHECK(key_for(d2, ident) != base);
        }
        SUBCASE("a different image") {
            sprite_def d2 = d;
            d2.image = mem_image(32, 32, {9, 9, 9, 9});
            CHECK(key_for(d2, ident) != base);
        }
    }

    TEST_CASE("trim metadata changes the key (so trimmed frames key distinctly)") {
        image_identifier ident;
        sprite_def d = basic_def();
        d.visuals.push_back(sprite_visual_def{"body", rect{0, 0, 8, 8}, point{4, 8}, std::nullopt, std::nullopt});
        const content_key untrimmed = key_for(d, ident);

        d.visuals.back().source_size = dim{12, 12};
        d.visuals.back().trim_offset = point{2, 4};
        CHECK(key_for(d, ident) != untrimmed);
    }
}
