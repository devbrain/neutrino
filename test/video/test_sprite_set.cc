#include <doctest/doctest.h>

#include <neutrino/video/sprite/sprite_set.hh>
#include <neutrino/video/sprite/sprite_def.hh>

#include "test_application.hh"
#include "video/test_images.hh"

#include <sdlpp/video/surface.hh>

#include <optional>
#include <utility>
#include <vector>

using namespace neutrino;
using namespace neutrino::test;

namespace {
    // A world_image backed by a BMP of a blank w*h surface (decodable via load_image).
    // A 64x64 image, a 4x4 grid of 16px cells, and two clips over grid frames.
    sprite_def grid_def() {
        sprite_def d;
        d.image = bmp_image(64, 64);
        d.grid = sprite_grid{16, 16, 4, 16, 0, 0, sprite_origin_rule::bottom_center};
        d.clips = {
            sprite_clip_def{"idle",
                            {sprite_frame_def{"0", sprite_animation_duration{100.0f}, sprite_flip::none}},
                            true},
            sprite_clip_def{"walk",
                            {sprite_frame_def{"1", sprite_animation_duration{90.0f}, sprite_flip::none},
                             sprite_frame_def{"2", sprite_animation_duration{90.0f}, sprite_flip::none}},
                            true},
        };
        return d;
    }
}

TEST_SUITE("neutrino::video sprite_set") {
    TEST_CASE("baked_visual_origin subtracts the trim offset from the untrimmed pivot") {
        sprite_visual_def v;
        v.origin = point{6, 12};                      // pivot in the untrimmed frame
        CHECK(baked_visual_origin(v).x == 6);          // untrimmed: origin == pivot
        CHECK(baked_visual_origin(v).y == 12);

        v.source_size = dim{12, 12};
        v.trim_offset = point{2, 4};
        CHECK(baked_visual_origin(v).x == 6 - 2);      // pivot - trim_offset
        CHECK(baked_visual_origin(v).y == 12 - 4);
    }

    TEST_CASE("build_sprite_set resolves visuals and clips by name") {
        neutrino::test::test_application app("sprite_set build");

        sprite_set set = build_sprite_set(grid_def());

        CHECK(set.visual("0").has_value());            // grid frame
        CHECK(set.visual("15").has_value());
        CHECK_FALSE(set.visual("nope").has_value());   // unknown -> nullopt
        CHECK(set.visual("0")->valid());

        CHECK(set.clip("idle").has_value());
        CHECK(set.clip("walk").has_value());
        CHECK_FALSE(set.clip("run").has_value());

        // Sprites bake no shared states -- playheads are per-instance.
        CHECK(set.states.empty());
        CHECK(set.animations.size() == 2);             // one per clip
        CHECK(set.sheets.size() == 1);
        CHECK(set.atlases.size() == 1);
    }

    TEST_CASE("an explicit visual overrides a grid frame of the same name") {
        neutrino::test::test_application app("sprite_set override");

        sprite_def d = grid_def();
        d.visuals.push_back(sprite_visual_def{"0", rect{0, 0, 8, 8}, point{4, 8}, std::nullopt, std::nullopt});
        d.clips.clear(); // avoid clip resolution noise

        sprite_set set = build_sprite_set(d); // must not throw
        CHECK(set.visual("0").has_value());
    }

    TEST_CASE("duplicate explicit visual or clip names fail the build") {
        neutrino::test::test_application app("sprite_set duplicates");

        SUBCASE("duplicate explicit visual name") {
            sprite_def d;
            d.image = bmp_image(32, 32);
            d.visuals = {
                sprite_visual_def{"body", rect{0, 0, 16, 16}, point{0, 0}, std::nullopt, std::nullopt},
                sprite_visual_def{"body", rect{16, 0, 16, 16}, point{0, 0}, std::nullopt, std::nullopt},
            };
            CHECK_THROWS(build_sprite_set(d));
        }

        SUBCASE("duplicate clip name") {
            sprite_def d = grid_def();
            d.clips.push_back(sprite_clip_def{"idle",
                              {sprite_frame_def{"3", sprite_animation_duration{50.0f}, sprite_flip::none}},
                              true});
            CHECK_THROWS(build_sprite_set(d));
        }

        SUBCASE("a clip referencing an unknown visual") {
            sprite_def d = grid_def();
            d.clips = {sprite_clip_def{"bad",
                       {sprite_frame_def{"does_not_exist", sprite_animation_duration{50.0f}, sprite_flip::none}},
                       true}};
            CHECK_THROWS(build_sprite_set(d));
        }
    }

    TEST_CASE("a sprite_set tears down its resources and rebuilds cleanly (RAII)") {
        neutrino::test::test_application app("sprite_set teardown");

        {
            sprite_set set = build_sprite_set(grid_def());
            CHECK(set.clip("walk").has_value());
        } // destructor: render_bundle::release() unregisters animations -> sheet -> atlas

        // Rebuilding after full teardown works (no stale registrations).
        sprite_set again = build_sprite_set(grid_def());
        CHECK(again.clip("idle").has_value());
    }
}
