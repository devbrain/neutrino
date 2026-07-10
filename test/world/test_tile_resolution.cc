#include <doctest/doctest.h>

#include <neutrino/world/world.hh>

#include <chrono>

using namespace neutrino;

namespace {
    world_image image_of(unsigned w, unsigned h) {
        world_image img;
        img.width = w;
        img.height = h;
        return img;
    }
}

TEST_SUITE("neutrino::world tile resolution") {
    TEST_CASE("uniform tileset resolves to a sub-rect of the shared image") {
        world_tileset ts;
        ts.first_gid = 1;
        ts.tile_width = 16;
        ts.tile_height = 16;
        ts.columns = 4;
        ts.tile_count = 16;
        ts.offset_x = 3;
        ts.offset_y = -5;
        ts.image = image_of(64, 64); // 4x4 grid of 16px tiles

        for (const world_local_tile_id id : {0u, 5u, 15u}) {
            const auto d = ts.drawable(id);
            const auto expected = ts.tile_rect(id);
            CHECK(d.image == &*ts.image);            // shared tileset image
            CHECK(d.src.x == expected.x);
            CHECK(d.src.y == expected.y);
            CHECK(d.src.w == expected.w);
            CHECK(d.src.h == expected.h);
            CHECK(d.origin.x == 3);                  // tileset draw offset
            CHECK(d.origin.y == -5);
            CHECK_FALSE(d.animated);
        }
    }

    TEST_CASE("collection tileset resolves to the tile's own whole image") {
        world_tileset ts;
        ts.first_gid = 100;
        ts.tile_count = 2;

        world_tile t0;
        t0.id = 0;
        t0.image = image_of(24, 32);
        world_tile t1;
        t1.id = 1;
        t1.image = image_of(10, 10);
        ts.tiles = {t0, t1};

        const auto d0 = ts.drawable(0);
        CHECK(d0.image == &*ts.tile(0)->image);      // the tile's own image
        CHECK(d0.src.x == 0);
        CHECK(d0.src.y == 0);
        CHECK(d0.src.w == 24);                       // whole image
        CHECK(d0.src.h == 32);

        const auto d1 = ts.drawable(1);
        CHECK(d1.image == &*ts.tile(1)->image);
        CHECK(d1.src.w == 10);
        CHECK(d1.src.h == 10);
    }

    TEST_CASE("collection tile with a source_rect resolves to that sub-rectangle") {
        // Image-collection atlas: several tiles slice regions out of one shared image.
        world_tileset ts;
        ts.first_gid = 1;
        ts.tile_count = 1;

        world_tile t;
        t.id = 0;
        t.image = image_of(256, 256);         // the shared image
        t.source_rect = rect{32, 48, 16, 24}; // this tile's region within it
        ts.tiles = {t};

        const auto d = ts.drawable(0);
        CHECK(d.image == &*ts.tile(0)->image);
        CHECK(d.src.x == 32);
        CHECK(d.src.y == 48);
        CHECK(d.src.w == 16);                 // the sub-rect, not the whole 256x256 image
        CHECK(d.src.h == 24);
    }

    TEST_CASE("animation_of returns frames only for animated tiles") {
        world_tileset ts;
        ts.tile_width = 16;
        ts.tile_height = 16;
        ts.columns = 2;
        ts.tile_count = 4;
        ts.image = image_of(32, 32);

        world_tile animated;
        animated.id = 1;
        animated.animation = {
            {0u, std::chrono::milliseconds{100}},
            {2u, std::chrono::milliseconds{150}}
        };
        ts.tiles = {animated};

        const auto* frames = ts.animation_of(1);
        REQUIRE(frames != nullptr);
        REQUIRE(frames->size() == 2);
        CHECK((*frames)[0].tile == 0);
        CHECK((*frames)[1].tile == 2);
        CHECK((*frames)[1].duration == std::chrono::milliseconds{150});

        CHECK(ts.animation_of(0) == nullptr);        // tile with no metadata
        CHECK(ts.animation_of(3) == nullptr);

        // drawable reflects the animated flag.
        CHECK(ts.drawable(1).animated);
        CHECK_FALSE(ts.drawable(0).animated);
    }

    TEST_CASE("tileset_for / to_local round-trip across multiple tilesets") {
        world w;
        world_tileset a;
        a.first_gid = 1;
        a.tile_count = 10;
        world_tileset b;
        b.first_gid = 11;
        b.tile_count = 5;
        w.add_tileset(a);
        w.add_tileset(b);

        const auto* ts5 = w.tileset_for(5);
        REQUIRE(ts5 != nullptr);
        CHECK(ts5->first_gid == 1);
        CHECK(ts5->to_local(5) == 4);
        CHECK(ts5->to_global(4) == 5);

        const auto* ts12 = w.tileset_for(12);
        REQUIRE(ts12 != nullptr);
        CHECK(ts12->first_gid == 11);
        CHECK(ts12->to_local(12) == 1);
        CHECK(ts12->to_global(1) == 12);

        CHECK(w.tileset_for(0) == nullptr);          // empty gid belongs to none
        // Tiled rule selects by highest first_gid <= gid; it does NOT upper-bound
        // by tile_count, so a gid past b's range still resolves to b.
        CHECK(w.tileset_for(100) == ts12);
    }
}
