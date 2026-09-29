#include <doctest/doctest.h>

#include <neutrino/video/sprite/sprite_cache.hh>
#include <neutrino/video/sprite/sprite_def.hh>

#include "test_application.hh"
#include "video/test_images.hh"

#include <sdlpp/video/surface.hh>

#include <utility>
#include <vector>

using namespace neutrino;
using namespace neutrino::test;

namespace {
    // A size x size image sliced into a 16px grid, with two clips over frames 0 and 1.
    sprite_def grid_def(unsigned size) {
        sprite_def d;
        d.image = bmp_image(size, size);
        d.grid = sprite_grid{16, 16, 0, 0, 0, 0, sprite_origin_rule::bottom_center};
        d.clips = {
            sprite_clip_def{"idle",
                            {sprite_frame_def{"0", sprite_animation_duration{100.0f}, sprite_flip::none}},
                            true},
            sprite_clip_def{"walk",
                            {sprite_frame_def{"1", sprite_animation_duration{90.0f}, sprite_flip::none}},
                            true},
        };
        return d;
    }
}

TEST_SUITE("neutrino::video sprite_cache") {
    TEST_CASE("identical defs share one set; distinct content builds a new one") {
        neutrino::test::test_application app("sprite_cache share");
        sprite_cache cache;

        const sprite_def d = grid_def(64);
        const sprite_set_handle a = cache.acquire(d);
        const sprite_set_handle b = cache.acquire(d); // same content -> shared
        REQUIRE(a.valid());
        CHECK(a.clip("idle").has_value());
        CHECK(cache.resident_count() == 1);

        const sprite_set_handle c = cache.acquire(grid_def(32)); // different image -> distinct
        CHECK(cache.resident_count() == 2);
    }

    TEST_CASE("a lease copy keeps the set resident; the last drop cools it") {
        neutrino::test::test_application app("sprite_cache lease");
        sprite_cache cache;

        {
            const sprite_set_handle outer = cache.acquire(grid_def(64));
            CHECK(cache.resident_count() == 1);
            CHECK(cache.cold_count() == 0);
            {
                const sprite_set_handle inner = outer; // copy -> retain
                CHECK(inner.clip("walk").has_value());
            }                                          // inner drops -> release (outer still holds)
            CHECK(cache.cold_count() == 0);            // not cold: outer keeps it
        }                                              // outer drops -> refcount 0 -> cold
        CHECK(cache.cold_count() == 1);
    }

    TEST_CASE("re-acquiring a cold set resurrects it") {
        neutrino::test::test_application app("sprite_cache resurrect");
        sprite_cache cache;

        const sprite_def d = grid_def(64);
        { const sprite_set_handle h = cache.acquire(d); } // -> cold
        CHECK(cache.cold_count() == 1);

        const sprite_set_handle again = cache.acquire(d); // resurrect from cold
        CHECK(cache.cold_count() == 0);
        CHECK(cache.resident_count() == 1);
        CHECK(again.clip("idle").has_value());
    }

    TEST_CASE("a move transfers the lease without touching the refcount") {
        neutrino::test::test_application app("sprite_cache move");
        sprite_cache cache;

        sprite_set_handle a = cache.acquire(grid_def(64));
        CHECK(cache.resident_count() == 1);
        const sprite_set_handle b = std::move(a);
        CHECK_FALSE(a.valid());
        CHECK(b.valid());
        CHECK(cache.cold_count() == 0); // still one live ref (b), not cold
    }

    TEST_CASE("overflowing the cold pool evicts the LRU set") {
        neutrino::test::test_application app("sprite_cache evict");
        sprite_cache cache(1); // cold budget of one

        { const sprite_set_handle a = cache.acquire(grid_def(64)); } // a -> cold
        CHECK(cache.cold_count() == 1);

        { const sprite_set_handle b = cache.acquire(grid_def(32)); } // b -> cold, overflow evicts a
        CHECK(cache.cold_count() == 1);
        CHECK(cache.resident_count() == 1);
    }
}

TEST_SUITE("neutrino::video sprite_instance") {
    TEST_CASE("spawn produces a playable instance; switch/restart route through the lease") {
        neutrino::test::test_application app("sprite_instance spawn");
        sprite_cache cache;
        const sprite_set_handle h = cache.acquire(grid_def(64));

        sprite_instance inst = h.spawn("idle");
        REQUIRE(inst.valid());
        CHECK(inst.state().valid());

        CHECK(inst.switch_to("walk"));        // known clip resolves via the lease
        CHECK_FALSE(inst.switch_to("nope"));  // unknown clip -> false, no change
        CHECK(inst.restart("idle"));

        CHECK_FALSE(h.spawn("nope").valid()); // unknown clip -> invalid instance
    }

    TEST_CASE("instances are independent playheads and move cleanly") {
        neutrino::test::test_application app("sprite_instance independent");
        sprite_cache cache;
        const sprite_set_handle h = cache.acquire(grid_def(64));

        sprite_instance a = h.spawn("idle");
        sprite_instance b = h.spawn("walk");
        REQUIRE(a.valid());
        REQUIRE(b.valid());

        sprite_instance moved = std::move(a);
        CHECK_FALSE(a.valid());               // moved-from -> empty (no double unregister)
        CHECK(moved.valid());
        CHECK(b.valid());                     // untouched
    }

    TEST_CASE("an instance's lease keeps its set resident after the owner handle drops") {
        neutrino::test::test_application app("sprite_instance lease");
        sprite_cache cache;

        {
            sprite_instance inst;
            {
                const sprite_set_handle h = cache.acquire(grid_def(64));
                inst = h.spawn("idle");
                CHECK(cache.resident_count() == 1);
            } // owner handle dropped -- but inst still leases the set

            CHECK(cache.cold_count() == 0);   // not cold: the instance's lease holds it
            CHECK(inst.valid());
            CHECK(inst.switch_to("walk"));    // its animation is still resident
        } // instance dropped: state unregistered first, then the lease releases

        CHECK(cache.cold_count() == 1);       // now the set is cold
    }

    TEST_CASE("application-level get_sprite_cache and acquire_sprite helper") {
        neutrino::test::test_application app("global sprite cache");

        auto& cache = neutrino::get_sprite_cache();
        CHECK(cache.resident_count() == 0);

        sprite_set_handle a = neutrino::acquire_sprite(grid_def(64));
        REQUIRE(a.valid());
        CHECK(cache.resident_count() == 1);

        sprite_set_handle b = neutrino::acquire_sprite(grid_def(64));
        REQUIRE(b.valid());
        CHECK(cache.resident_count() == 1); // Content shared in engine cache

        sprite_instance inst = a.spawn("idle");
        REQUIRE(inst.valid());
        CHECK(inst.switch_to("walk"));
    }

    TEST_CASE("sprite_set_handle safely outlives sprite_cache") {
        neutrino::test::test_application app("handle outlives cache");

        sprite_set_handle leaked_handle;

        {
            sprite_cache local_cache;
            leaked_handle = local_cache.acquire(grid_def(64));
            REQUIRE(leaked_handle.valid());
            CHECK(local_cache.resident_count() == 1);
        } // local_cache is destroyed here before leaked_handle

        // Handle detects that owner cache has expired
        CHECK_FALSE(leaked_handle.valid());
        CHECK_FALSE(leaked_handle.clip("idle").has_value());
        CHECK_FALSE(leaked_handle.visual("0").has_value());
        CHECK_FALSE(leaked_handle.spawn("idle").valid());

        // Destruction of leaked_handle at function exit must not crash or UAF
    }

    TEST_CASE("scene member destruction order independence: instance destroyed after handle") {
        neutrino::test::test_application app("order independence: inst after handle");

        struct scene_mock {
            // Declared in order where set is destroyed BEFORE player:
            sprite_instance   player;
            sprite_set_handle set;
        };

        {
            scene_mock scene;
            scene.set = neutrino::acquire_sprite(grid_def(64));
            scene.player = scene.set.spawn("idle");
            REQUIRE(scene.player.valid());
            REQUIRE(scene.set.valid());
        } // scene destroyed: set destroyed first, then player. No crash, clean teardown!

        CHECK(neutrino::get_sprite_cache().cold_count() == 1);
    }

    TEST_CASE("scene member destruction order independence: handle destroyed after instance") {
        neutrino::test::test_application app("order independence: handle after inst");

        struct scene_mock {
            // Declared in order where player is destroyed BEFORE set:
            sprite_set_handle set;
            sprite_instance   player;
        };

        {
            scene_mock scene;
            scene.set = neutrino::acquire_sprite(grid_def(64));
            scene.player = scene.set.spawn("idle");
            REQUIRE(scene.player.valid());
            REQUIRE(scene.set.valid());
        } // scene destroyed: player destroyed first, then set. No crash, clean teardown!

        CHECK(neutrino::get_sprite_cache().cold_count() == 1);
    }
}


