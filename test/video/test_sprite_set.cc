#include <doctest/doctest.h>

#include <neutrino/video/sprite/sprite_set.hh>
#include <neutrino/video/sprite/sprite_cache.hh>
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

    // The require_* lookups exist so a CONFIGURATION invariant ("this sheet must have this
    // frame") fails at the lookup instead of degrading to zero geometry via value_or(rect{}) --
    // which reaches gameplay as a 0x0 collider or a 1x1 paddle rather than a diagnosable error.
    TEST_CASE("require_* returns the value when present and throws when absent") {
        neutrino::test::test_application app("sprite_set require");

        sprite_set set = build_sprite_set(grid_def()); // 4x4 grid of 16px cells, bottom_center pivot

        SUBCASE("present: same answer as the optional accessors, unwrapped") {
            CHECK(set.require_visual("0") == *set.visual("0"));
            CHECK(set.require_visual(std::size_t{3}) == *set.visual(std::size_t{3}));
            CHECK(set.require_frame_rect("0").w == 16);
            CHECK(set.require_frame_rect("0").h == 16);
            CHECK(set.require_frame_rect(std::size_t{5}).w == 16);
            CHECK(set.require_origin("0").x == 8);   // bottom_center of a 16x16 cell
            CHECK(set.require_origin("0").y == 16);
            CHECK(set.require_origin(std::size_t{0}).x == 8);
            CHECK(set.require_clip("idle") == *set.clip("idle"));
        }

        SUBCASE("absent by name") {
            CHECK_THROWS((void) set.require_visual("nope"));
            CHECK_THROWS((void) set.require_frame_rect("nope"));
            CHECK_THROWS((void) set.require_origin("nope"));
            CHECK_THROWS((void) set.require_clip("run"));
        }

        SUBCASE("index out of range") {
            CHECK_THROWS((void) set.require_visual(std::size_t{16}));      // 0..15 exist
            CHECK_THROWS((void) set.require_frame_rect(std::size_t{16}));
            CHECK_THROWS((void) set.require_origin(std::size_t{999}));
        }
    }

    // Trim metadata used to be consumed by the build (only the packed rect + baked pivot survived),
    // so the authored frame was unrecoverable at runtime and consumers treated packed pixel sizes
    // as gameplay dimensions. metrics() must carry both spaces through.
    TEST_CASE("metrics preserves the untrimmed frame through the build") {
        neutrino::test::test_application app("sprite_set metrics");

        sprite_def d;
        d.image = bmp_image(64, 64);
        d.visuals = {
            // A 20x20 authored frame trimmed to the 12x8 patch at (3,5) within it, pivot at its
            // bottom-centre (10,20) in UNTRIMMED space.
            sprite_visual_def{"trimmed", rect{32, 16, 12, 8}, point{10, 20}, dim{20, 20}, point{3, 5}},
            sprite_visual_def{"plain", rect{0, 0, 16, 16}, point{8, 16}, std::nullopt, std::nullopt},
        };
        sprite_set set = build_sprite_set(d);

        SUBCASE("a trimmed frame reports both the packed and the authored box") {
            const sprite_metrics m = set.require_metrics("trimmed");
            CHECK(m.trimmed());
            CHECK(m.texture_rect.w == 12);           // packed / visible pixels
            CHECK(m.texture_rect.h == 8);
            CHECK(m.source_size.width == 20);        // authored frame -- previously unrecoverable
            CHECK(m.source_size.height == 20);
            CHECK(m.trim_offset.x == 3);
            CHECK(m.trim_offset.y == 5);
            CHECK(m.logical_pivot.x == 10);          // pivot in untrimmed space, as authored
            CHECK(m.logical_pivot.y == 20);
            CHECK(m.pivot.x == 10 - 3);              // baked into packed space for drawing
            CHECK(m.pivot.y == 20 - 5);

            // Placing at an anchor: the visible pixels sit inside the logical box, offset by trim.
            const rect vis = m.visible_bounds_at(point{100, 100});
            const rect log = m.logical_bounds_at(point{100, 100});
            CHECK(log.x == 90);                      // 100 - 10
            CHECK(log.y == 80);                      // 100 - 20
            CHECK(log.w == 20);
            CHECK(log.h == 20);
            CHECK(vis.x == log.x + 3);               // trim offset within the logical box
            CHECK(vis.y == log.y + 5);
            CHECK(vis.w == 12);
            CHECK(vis.h == 8);

            const rect lb = m.local_bounds();        // logical box relative to the pivot
            CHECK(lb.x == -10);
            CHECK(lb.y == -20);
            CHECK(lb.w == 20);
            CHECK(lb.h == 20);
        }

        SUBCASE("incoherent trim metadata is rejected at build time") {
            // A trim_offset without a source_size leaves the authored size defaulted to the PACKED
            // size, so the visible patch would stick out of the frame it was supposedly trimmed
            // from: trimmed() would read false and logical_bounds_at() would no longer contain the
            // visible pixels. Reject it rather than emit silently wrong metrics.
            sprite_def bad;
            bad.image = bmp_image(64, 64);
            bad.visuals = {sprite_visual_def{"orphan_trim", rect{0, 0, 12, 8}, point{0, 0},
                                             std::nullopt, point{3, 5}}};
            CHECK_THROWS((void) build_sprite_set(bad));

            // Same invariant the other way: an authored frame too small to hold the packed patch.
            sprite_def small;
            small.image = bmp_image(64, 64);
            small.visuals = {sprite_visual_def{"too_small", rect{0, 0, 12, 8}, point{0, 0},
                                               dim{6, 6}, point{0, 0}}};
            CHECK_THROWS((void) build_sprite_set(small));

            // But a source_size WITHOUT a trim_offset is perfectly coherent -- art trimmed only on
            // the right/bottom sits at the authored frame's top-left.
            sprite_def ok;
            ok.image = bmp_image(64, 64);
            ok.visuals = {sprite_visual_def{"pad_right", rect{0, 0, 12, 8}, point{0, 0},
                                            dim{20, 20}, std::nullopt}};
            sprite_set s = build_sprite_set(ok);
            const sprite_metrics m = s.require_metrics("pad_right");
            CHECK(m.trimmed());
            CHECK(m.source_size.width == 20);
            CHECK(m.trim_offset.x == 0);
        }

        SUBCASE("an untrimmed frame reports identical packed and authored boxes") {
            const sprite_metrics m = set.require_metrics("plain");
            CHECK_FALSE(m.trimmed());
            CHECK(m.source_size.width == 16);
            CHECK(m.source_size.height == 16);
            CHECK(m.trim_offset.x == 0);
            CHECK(m.pivot.x == m.logical_pivot.x);   // no trim -> the two pivots coincide
            CHECK(m.pivot.y == m.logical_pivot.y);
            CHECK(m.visible_bounds_at(point{50, 50}).x == m.logical_bounds_at(point{50, 50}).x);
        }

        SUBCASE("metrics agrees with frame_rect and origin, and is absent for unknown names") {
            CHECK(set.metrics("trimmed")->texture_rect.w == set.frame_rect("trimmed")->w);
            CHECK(set.metrics("trimmed")->pivot.x == set.origin("trimmed")->x);
            CHECK_FALSE(set.metrics("nope").has_value());
            CHECK_THROWS((void) set.require_metrics("nope"));
            CHECK_THROWS((void) set.require_metrics(std::size_t{99}));
        }
    }

    // The name maps (visuals_by_name / clips_by_name) are plain members and survive release(),
    // which tears down the registered resources. Without a liveness check the NAMED lookups would
    // keep returning ids into unregistered resources while the INDEXED ones correctly reported
    // nothing -- and require_visual/require_clip would hand back a dangling id instead of throwing.
    TEST_CASE("a released sprite_set reports nothing through named lookups too") {
        neutrino::test::test_application app("sprite_set released lookups");

        sprite_set set = build_sprite_set(grid_def());
        REQUIRE(set.visual("0").has_value());     // live: name lookup resolves
        REQUIRE(set.clip("idle").has_value());
        REQUIRE(set.visual(std::size_t{0}).has_value());

        set.release();                            // tears down atlas + sheet + animations

        SUBCASE("named lookups agree with the indexed ones") {
            CHECK_FALSE(set.visual("0").has_value());
            CHECK_FALSE(set.visual(std::size_t{0}).has_value()); // indexed was already correct
            CHECK_FALSE(set.clip("idle").has_value());
            CHECK_FALSE(set.frame_rect("0").has_value());
            CHECK_FALSE(set.metrics("0").has_value());
        }

        SUBCASE("required named lookups throw instead of returning a dangling id") {
            CHECK_THROWS((void) set.require_visual("0"));
            CHECK_THROWS((void) set.require_clip("idle"));
            CHECK_THROWS((void) set.require_frame_rect("0"));
            CHECK_THROWS((void) set.require_metrics("0"));
        }
    }

    // An empty lease answers every optional lookup with nullopt; require_* must distinguish
    // "no such frame" from "no set acquired" rather than blaming the key.
    TEST_CASE("require_* on an empty sprite_set_handle lease throws") {
        sprite_set_handle empty;
        REQUIRE_FALSE(empty.valid());
        CHECK_THROWS((void) empty.require_visual("0"));
        CHECK_THROWS((void) empty.require_frame_rect("0"));
        CHECK_THROWS((void) empty.require_frame_rect(std::size_t{0}));
        CHECK_THROWS((void) empty.require_origin("0"));
        CHECK_THROWS((void) empty.require_metrics("0"));
        CHECK_THROWS((void) empty.require_clip("idle"));
        CHECK_FALSE(empty.metrics("0").has_value());
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
