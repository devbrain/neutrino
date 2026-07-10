#include <doctest/doctest.h>

#include <neutrino/video/world/resource_cache.hh>

#include "test_application.hh"

#include <sdlpp/video/surface.hh>

#include <filesystem>
#include <fstream>
#include <utility>
#include <vector>

using namespace neutrino;

namespace {
    // A world_image whose embedded data is a BMP of a blank size*size surface. Two
    // images of different size are guaranteed distinct content (different byte
    // length -> different content key), which is all these tests rely on.
    world_image bmp_image(unsigned size) {
        auto surface = sdlpp::surface::create_rgb(
            static_cast<int>(size), static_cast<int>(size), sdlpp::pixel_format_enum::RGBA8888);
        REQUIRE(surface.has_value());
        auto bytes = sdlpp::save_bmp(*surface);
        REQUIRE(bytes.has_value());

        world_image img;
        img.width = size;
        img.height = size;
        img.data = std::move(*bytes);
        return img;
    }

    // A uniform 2-column, 4-tile tileset over the given image.
    world_tileset uniform_ts(world_image img) {
        world_tileset ts;
        ts.first_gid = 1;
        ts.tile_width = 16;
        ts.tile_height = 16;
        ts.columns = 2;
        ts.tile_count = 4;
        ts.image = std::move(img);
        return ts;
    }

    void write_bmp_file(const std::filesystem::path& path, unsigned size) {
        auto surface = sdlpp::surface::create_rgb(
            static_cast<int>(size), static_cast<int>(size), sdlpp::pixel_format_enum::RGBA8888);
        REQUIRE(surface.has_value());
        auto bytes = sdlpp::save_bmp(*surface);
        REQUIRE(bytes.has_value());
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        REQUIRE(out.good());
        out.write(reinterpret_cast<const char*>(bytes->data()),
                  static_cast<std::streamsize>(bytes->size()));
    }
}

TEST_SUITE("neutrino::video resource_cache") {
    TEST_CASE("acquiring the same tileset twice builds once and shares the bundle") {
        neutrino::test::test_application app("resource_cache share");
        resource_cache cache;

        const world_tileset ts = uniform_ts(bmp_image(32));
        const bundle_handle a = cache.acquire(ts);
        const bundle_handle b = cache.acquire(ts);

        REQUIRE(a.valid());
        CHECK(a.bundle == b.bundle);          // same entry: no second build
        CHECK(cache.resident_count() == 1);
        CHECK(cache.cold_count() == 0);       // refcount 2, nothing idle
        CHECK(a.visual(0).valid());

        cache.release(a);
        cache.release(b);
    }

    TEST_CASE("release drops to cold only at refcount zero, and cold stays resolvable") {
        neutrino::test::test_application app("resource_cache cold");
        resource_cache cache;

        const world_tileset ts = uniform_ts(bmp_image(32));
        const bundle_handle a = cache.acquire(ts);
        const bundle_handle b = cache.acquire(ts);

        cache.release(a);
        CHECK(cache.resident_count() == 1);
        CHECK(cache.cold_count() == 0);       // still one live reference

        cache.release(b);
        CHECK(cache.resident_count() == 1);
        CHECK(cache.cold_count() == 1);       // idle now, but still resident
        CHECK(b.bundle->visual(0).valid());   // a cold bundle still resolves
    }

    TEST_CASE("overflowing the cold budget evicts and tears down the LRU bundle") {
        neutrino::test::test_application app("resource_cache evict");
        resource_cache cache(1); // cold pool holds a single bundle

        const world_tileset tsa = uniform_ts(bmp_image(32));
        const world_tileset tsb = uniform_ts(bmp_image(40));

        bundle_handle a = cache.acquire(tsa);
        const sprite_sheet_id a_sheet = a.bundle->sheets.at(0); // capture before eviction
        cache.release(a);                                       // A -> cold (pool size 1)

        const bundle_handle b = cache.acquire(tsb);
        cache.release(b);                                       // B -> cold, evicts A
        CHECK(cache.cold_count() == 1);
        CHECK(cache.resident_count() == 1);                     // only B remains

        // Re-acquiring A is a miss -> a fresh build with a newer sheet id.
        const bundle_handle a2 = cache.acquire(tsa);
        CHECK(a2.bundle->sheets.at(0) != a_sheet);
        CHECK(cache.resident_count() == 2);
        cache.release(a2);
    }

    TEST_CASE("two tilesets with identical content share one bundle") {
        neutrino::test::test_application app("resource_cache dedup");
        resource_cache cache;

        const world_image shared = bmp_image(32);
        const world_tileset tsa = uniform_ts(shared); // same bytes...
        const world_tileset tsb = uniform_ts(shared); // ...copied into a second tileset

        const bundle_handle a = cache.acquire(tsa);
        const bundle_handle b = cache.acquire(tsb);
        CHECK(a.bundle == b.bundle);          // same content key -> shared bundle
        CHECK(cache.resident_count() == 1);

        cache.release(a);
        cache.release(b);
    }

    TEST_CASE("changing a file-backed image yields a new bundle") {
        neutrino::test::test_application app("resource_cache mtime");
        resource_cache cache;

        const auto path = std::filesystem::temp_directory_path() / "neutrino_rc_mtime.bmp";
        std::filesystem::remove(path);
        write_bmp_file(path, 32);

        world_tileset ts;
        ts.first_gid = 1;
        ts.tile_width = 16;
        ts.tile_height = 16;
        ts.columns = 2;
        ts.tile_count = 4;
        ts.image = world_image{};
        ts.image->source = path;
        ts.image->width = 32;  // declared tile-grid extent; the file may be larger
        ts.image->height = 32;

        const bundle_handle first = cache.acquire(ts);
        const sprite_sheet_id first_sheet = first.bundle->sheets.at(0);
        cache.release(first);

        // Rewrite the file with different content -> different stat + content key.
        write_bmp_file(path, 48);
        const bundle_handle second = cache.acquire(ts);
        CHECK(second.bundle->sheets.at(0) != first_sheet); // a distinct, freshly built bundle
        CHECK(cache.resident_count() == 2);
        cache.release(second);

        std::filesystem::remove(path);
    }

    TEST_CASE("a stale handle whose entry was evicted and rebuilt is ignored by release") {
        neutrino::test::test_application app("resource_cache stale handle");
        resource_cache cache(1); // cold pool holds a single bundle

        const world_tileset tsa = uniform_ts(bmp_image(32));
        const world_tileset tsb = uniform_ts(bmp_image(40));

        const bundle_handle a = cache.acquire(tsa); // token stamped here
        cache.release(a);                           // A -> cold (pool size 1)

        const bundle_handle b = cache.acquire(tsb);
        cache.release(b);                           // B -> cold, evicts and tears down A

        // Re-acquire A: content is identical, so the content key recycles, but this is
        // a fresh entry (a miss rebuilt it) with a new token and refcount 1.
        const bundle_handle a2 = cache.acquire(tsa);
        CHECK(a2.token != a.token);                 // same key, distinct entry identity
        CHECK(cache.resident_count() == 2);         // B cold + A2 live
        CHECK(cache.cold_count() == 1);             // only B is idle

        // The stale original handle carries the old token; releasing it must NOT
        // decrement the rebuilt entry (which would drop a live A2 to cold).
        cache.release(a);
        CHECK(cache.resident_count() == 2);         // unchanged: A2 still live
        CHECK(cache.cold_count() == 1);             // still only B idle
        CHECK(a2.bundle->visual(0).valid());        // A2 intact and resolvable

        cache.release(a2);                          // the genuine release still works
    }

    TEST_CASE("same image bytes with different declared dimensions do not share a bundle") {
        neutrino::test::test_application app("resource_cache declared dims");
        resource_cache cache;

        // Identical bytes, but two different declared grid extents. The declared dims
        // drive tile_rect's column derivation (columns == 0), so the bundles' src
        // rects differ and the entries must not collide on content-byte identity alone.
        const world_image base = bmp_image(32);

        const auto make = [] (const world_image& img, unsigned declared) {
            world_tileset ts;
            ts.first_gid = 1;
            ts.tile_width = 8;
            ts.tile_height = 8;
            ts.columns = 0;          // derive column count from the declared image width
            ts.tile_count = 4;
            ts.image = img;          // same bytes...
            ts.image->width = declared;  // ...different declared extent
            ts.image->height = declared;
            return ts;
        };

        const world_tileset ts32 = make(base, 32); // 4 columns
        const world_tileset ts16 = make(base, 16); // 2 columns -> different layout

        const bundle_handle a = cache.acquire(ts32);
        const bundle_handle b = cache.acquire(ts16);
        CHECK(a.bundle != b.bundle);          // distinct keys -> distinct bundles
        CHECK(cache.resident_count() == 2);

        cache.release(a);
        cache.release(b);
    }

    TEST_CASE("image-collection tiles slicing different sub-rects do not share a bundle") {
        neutrino::test::test_application app("resource_cache source rect");
        resource_cache cache;

        // Same shared atlas image, but the tile slices a different sub-rectangle -- so
        // world_tileset::drawable renders a different region. The bundles must not
        // collide on image-byte identity alone (the key must fold source_rect).
        const world_image atlas = bmp_image(64);

        const auto make = [&] (std::optional <rect> slice) {
            world_tileset ts;
            ts.first_gid = 1;
            ts.tile_count = 1;
            world_tile t;
            t.id = 0;
            t.image = atlas;         // same shared image bytes...
            t.source_rect = slice;   // ...different (or absent) slice
            ts.tiles.push_back(std::move(t));
            return ts;
        };

        const bundle_handle a = cache.acquire(make(rect{0, 0, 16, 16}));
        const bundle_handle b = cache.acquire(make(rect{16, 0, 16, 16})); // different x offset
        const bundle_handle c = cache.acquire(make(std::nullopt));        // whole image

        CHECK(a.bundle != b.bundle);          // distinct source rects -> distinct keys
        CHECK(c.bundle != a.bundle);          // presence vs absence keyed too
        CHECK(cache.resident_count() == 3);

        // The identical slice re-acquires the same bundle (sanity: the key is stable).
        const bundle_handle a2 = cache.acquire(make(rect{0, 0, 16, 16}));
        CHECK(a2.bundle == a.bundle);

        cache.release(a);
        cache.release(a2);
        cache.release(b);
        cache.release(c);
    }

    TEST_CASE("switch delta: shared tileset survives, only the difference moves") {
        neutrino::test::test_application app("resource_cache switch delta");
        resource_cache cache;

        const world_tileset tsa = uniform_ts(bmp_image(32));
        const world_tileset tsb = uniform_ts(bmp_image(40));
        const world_tileset tsc = uniform_ts(bmp_image(48));

        // Old level holds {A, B}.
        const bundle_handle a = cache.acquire(tsa);
        const bundle_handle b = cache.acquire(tsb);
        const tileset_bundle* b_bundle = b.bundle;
        const sprite_sheet_id b_sheet = b.bundle->sheets.at(0);

        // New level acquires {B, C} before the old level releases.
        const bundle_handle b2 = cache.acquire(tsb);
        const bundle_handle c = cache.acquire(tsc);
        CHECK(b2.bundle == b_bundle);                 // B reused, not rebuilt

        // Old level releases {A, B}.
        cache.release(a);
        cache.release(b);

        // B was never rebuilt (same bundle + sheet id throughout); C built once; A idle.
        CHECK(b2.bundle == b_bundle);
        CHECK(b2.bundle->sheets.at(0) == b_sheet);
        CHECK(c.valid());
        CHECK(c.bundle != b_bundle);
        CHECK(cache.cold_count() == 1);               // only A went idle
        CHECK(cache.resident_count() == 3);           // A cold, B and C live

        cache.release(b2);
        cache.release(c);
    }
}
