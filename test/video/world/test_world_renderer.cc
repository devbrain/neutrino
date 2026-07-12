#include <doctest/doctest.h>

#include <neutrino/video/world/world_renderer.hh>
#include <neutrino/video/world/resource_cache.hh>

#include "test_application.hh"
#include "services/service_locator.hh"

#include <sdlpp/video/surface.hh>

#include <cstdint>
#include <utility>

using namespace neutrino;

namespace {
    // Embedded BMP of a size*size surface; different sizes are guaranteed distinct
    // content (different byte length -> different content key).
    world_image bmp_image(unsigned size) {
        auto surface = sdlpp::surface::create_rgb(
            static_cast<int>(size), static_cast<int>(size), sdlpp::pixel_format_enum::RGBA8888);
        REQUIRE(surface.has_value());
        auto bytes = sdlpp::save_bmp(*surface);
        REQUIRE(bytes.has_value());

        world_image img;
        img.width = size;
        img.height = size;
        img.source = image_from_memory{std::move(*bytes)};
        return img;
    }

    // A tileset whose embedded bytes are not a decodable image, so build_bundle
    // (via load_image) throws when the cache tries to acquire it.
    world_tileset broken_ts(world_tile_id first_gid) {
        world_image img;
        img.width = 16;
        img.height = 16;
        img.source = image_from_memory{{std::uint8_t{1}, 2, 3, 4}}; // garbage, not an image
        world_tileset ts;
        ts.first_gid = first_gid;
        ts.tile_width = 16;
        ts.tile_height = 16;
        ts.columns = 1;
        ts.tile_count = 1;
        ts.image = std::move(img);
        return ts;
    }

    world_tileset uniform_ts(world_image img, world_tile_id first_gid = 1) {
        world_tileset ts;
        ts.first_gid = first_gid;
        ts.tile_width = 16;
        ts.tile_height = 16;
        ts.columns = 2;
        ts.tile_count = 4;
        ts.image = std::move(img);
        return ts;
    }

    // A world carrying the given tilesets (no layers needed for lifecycle tests).
    world make_world(std::initializer_list<world_tileset> tilesets) {
        world w;
        for (const world_tileset& ts : tilesets) {
            w.add_tileset(ts);
        }
        return w;
    }
}

TEST_SUITE("neutrino::video world_renderer") {
    TEST_CASE("construction acquires a bundle per tileset; destruction releases all") {
        neutrino::test::test_application app("world_renderer lifecycle");
        resource_cache cache;

        const world w = make_world({uniform_ts(bmp_image(32), 1),
                                    uniform_ts(bmp_image(40), 5)});
        {
            world_renderer r(w, cache);
            CHECK(r.tileset_count() == 2);
            CHECK(cache.resident_count() == 2);
            CHECK(cache.cold_count() == 0);            // both in use
            CHECK(r.handle_for_tileset(0).valid());
            CHECK(r.handle_for_tileset(1).valid());
        }
        // Destroyed: both released, now idle in the cold pool (still resident).
        CHECK(cache.resident_count() == 2);
        CHECK(cache.cold_count() == 2);
    }

    TEST_CASE("a tileset shared by two renderers stays resident when the first is destroyed") {
        neutrino::test::test_application app("world_renderer shared");
        resource_cache cache;

        const world_image shared = bmp_image(32);
        const world wa = make_world({uniform_ts(shared, 1),        // X (shared)
                                     uniform_ts(bmp_image(40), 5)}); // Y
        const world wb = make_world({uniform_ts(shared, 1),        // X (same content)
                                     uniform_ts(bmp_image(48), 5)}); // Z

        world_renderer rb(wb, cache);
        const tileset_bundle* x_bundle = rb.handle_for_tileset(0).bundle;
        {
            world_renderer ra(wa, cache);
            CHECK(ra.handle_for_tileset(0).bundle == x_bundle); // X reused, not rebuilt
            CHECK(cache.cold_count() == 0);                     // X refcount 2, Y and Z live
        }
        // ra gone: X held by rb (still in use), Y dropped to cold. Z still live.
        CHECK(cache.cold_count() == 1);                          // only Y idle
        CHECK(rb.handle_for_tileset(0).bundle == x_bundle);      // X survived, same bundle
        CHECK(rb.handle_for_tileset(0).visual(0).valid());
    }

    TEST_CASE("moving a renderer does not double-release") {
        neutrino::test::test_application app("world_renderer move");
        resource_cache cache;

        const world w = make_world({uniform_ts(bmp_image(32), 1),
                                    uniform_ts(bmp_image(40), 5)});

        SUBCASE("move construction") {
            {
                world_renderer a(w, cache);
                world_renderer b(std::move(a));
                CHECK(b.tileset_count() == 2);
                CHECK(cache.cold_count() == 0); // a is inert; b holds the two refs
            }
            CHECK(cache.cold_count() == 2);     // exactly one release per tileset
        }

        SUBCASE("move assignment releases the target's own handles first") {
            const world other = make_world({uniform_ts(bmp_image(64), 1)});
            {
                world_renderer a(w, cache);       // holds the 2 tilesets of w
                world_renderer b(other, cache);   // holds the 1 tileset of `other`
                CHECK(cache.cold_count() == 0);
                b = std::move(a);                 // b releases `other`; adopts w's two
                CHECK(cache.cold_count() == 1);   // `other` went idle
                CHECK(b.tileset_count() == 2);
            }
            CHECK(cache.cold_count() == 3);       // other + w's two, released once each
        }
    }

    TEST_CASE("switch_to acquires the next level before releasing the current one") {
        neutrino::test::test_application app("world_renderer switch");
        resource_cache cache;

        const world_image shared = bmp_image(32);
        const world old_level  = make_world({uniform_ts(shared, 1),          // S (shared)
                                             uniform_ts(bmp_image(40), 5)});   // old-only
        const world next_level = make_world({uniform_ts(shared, 1),          // S (same content)
                                             uniform_ts(bmp_image(48), 5)});   // new-only

        world_renderer r(old_level, cache);
        const tileset_bundle* s_bundle = r.handle_for_tileset(0).bundle;
        const sprite_sheet_id s_sheet = r.handle_for_tileset(0).bundle->sheets.at(0);

        r.switch_to(next_level);

        // The shared tileset was never rebuilt across the switch (same bundle + sheet
        // id), the old-only tileset went idle, and the new-only tileset built once.
        CHECK(r.handle_for_tileset(0).bundle == s_bundle);
        CHECK(r.handle_for_tileset(0).bundle->sheets.at(0) == s_sheet);
        CHECK(cache.cold_count() == 1);        // only the departed old-only tileset
        CHECK(r.tileset_count() == 2);
    }

    TEST_CASE("a construction that fails partway releases the handles it already acquired") {
        neutrino::test::test_application app("world_renderer partial failure");
        resource_cache cache;

        // First tileset builds fine; the second cannot decode, so acquisition throws
        // mid-construction. The good tileset's ref must not leak.
        const world w = make_world({uniform_ts(bmp_image(32), 1),
                                    broken_ts(5)});

        CHECK_THROWS(world_renderer(w, cache));

        // The good tileset was released back to the cache (refcount 0 -> cold), not
        // left stuck live: without the fix it would sit at refcount 1 forever
        // (cold_count == 0). The broken one never entered the cache.
        CHECK(cache.resident_count() == 1);
        CHECK(cache.cold_count() == 1);
    }

    TEST_CASE("the default constructor binds to the application-wide cache") {
        neutrino::test::test_application app("world_renderer located cache");
        resource_cache* located = service_locator::instance().get_resource_cache();
        REQUIRE(located != nullptr);                 // published at on_ready
        CHECK(located->resident_count() == 0);

        const world w = make_world({uniform_ts(bmp_image(32), 1)});
        {
            world_renderer r(w);                     // no explicit cache
            CHECK(located->resident_count() == 1);   // acquired through the located cache
            CHECK(located->cold_count() == 0);
        }
        CHECK(located->cold_count() == 1);           // released back to the located cache
    }
}
