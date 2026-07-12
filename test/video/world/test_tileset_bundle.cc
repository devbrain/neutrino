#include <doctest/doctest.h>

#include <neutrino/video/world/tileset_bundle.hh>

#include "test_application.hh"

#include <sdlpp/video/surface.hh>

#include <chrono>
#include <memory>
#include <vector>

using namespace neutrino;

namespace {
    // A world_image whose embedded data is a BMP of a blank w*h surface, so
    // build_bundle can decode it through load_image with no on-disk asset.
    world_image bmp_image(unsigned w, unsigned h) {
        auto surface = sdlpp::surface::create_rgb(
            static_cast <int>(w), static_cast <int>(h), sdlpp::pixel_format_enum::RGBA8888);
        REQUIRE(surface.has_value());
        auto bytes = sdlpp::save_bmp(*surface);
        REQUIRE(bytes.has_value());

        world_image img;
        img.width = w;
        img.height = h;
        img.source = image_from_memory{std::move(*bytes)};
        return img;
    }

    // A world_image backed by an already-decoded surface (the Flavor-1 source): no
    // encode/decode round-trip, build_bundle borrows the pixels directly.
    world_image surface_image(unsigned w, unsigned h) {
        auto surface = sdlpp::surface::create_rgb(
            static_cast <int>(w), static_cast <int>(h), sdlpp::pixel_format_enum::RGBA8888);
        REQUIRE(surface.has_value());

        world_image img;
        img.width = w;
        img.height = h;
        img.source = image_from_surface{
            std::make_shared <const sdlpp::surface>(std::move(*surface)), std::nullopt};
        return img;
    }

    // Uniform tileset: one shared image sliced into a tile_count grid.
    world_tileset uniform_tileset(unsigned image_wh, unsigned tile_wh, unsigned columns,
                                  unsigned tile_count, int off_x = 0, int off_y = 0) {
        world_tileset ts;
        ts.first_gid = 1;
        ts.tile_width = tile_wh;
        ts.tile_height = tile_wh;
        ts.columns = columns;
        ts.tile_count = tile_count;
        ts.offset_x = off_x;
        ts.offset_y = off_y;
        ts.image = bmp_image(image_wh, image_wh);
        return ts;
    }
}

TEST_SUITE("neutrino::video tileset bundle") {
    TEST_CASE("uniform tileset builds one registered visual per tile") {
        neutrino::test::test_application app("tileset bundle uniform");

        const world_tileset ts = uniform_tileset(64, 16, 4, 16, 3, -5);
        tileset_bundle bundle = build_bundle(ts);

        CHECK(bundle.visuals.size() == 16);
        for (world_local_tile_id id = 0; id < 16; ++id) {
            CHECK(bundle.visual(id).valid());
        }
        CHECK_FALSE(bundle.sheets.empty());
        CHECK(bundle.atlases.size() == bundle.sheets.size()); // one sheet per uploaded page

        // No animated tiles: every state slot is the invalid sentinel.
        for (world_local_tile_id id = 0; id < 16; ++id) {
            CHECK_FALSE(bundle.state(id).valid());
        }
        CHECK(bundle.animations.empty());

        destroy_bundle(bundle);
        CHECK(bundle.visuals.empty());
        CHECK(bundle.sheets.empty());
        CHECK(bundle.atlases.empty());
    }

    TEST_CASE("a uniform tileset with omitted tile_count derives its slots from the image") {
        neutrino::test::test_application app("tileset bundle derived count");

        // A hand-authored or direct-built uniform tileset that never set tile_count:
        // the 64x64 image at 16px tiles is a 4x4 grid, so 16 visuals must still build.
        SUBCASE("columns known, tile_count omitted") {
            const world_tileset ts = uniform_tileset(64, 16, 4, /*tile_count=*/0);
            CHECK(ts.effective_tile_count() == 16);
            tileset_bundle bundle = build_bundle(ts);
            CHECK(bundle.visuals.size() == 16);
            for (world_local_tile_id id = 0; id < 16; ++id) {
                CHECK(bundle.visual(id).valid());
            }
            destroy_bundle(bundle);
        }

        // Neither columns nor tile_count set: both derive from the image dimensions.
        SUBCASE("columns and tile_count both omitted") {
            const world_tileset ts = uniform_tileset(64, 16, /*columns=*/0, /*tile_count=*/0);
            CHECK(ts.effective_tile_count() == 16);
            tileset_bundle bundle = build_bundle(ts);
            CHECK(bundle.visuals.size() == 16);
            CHECK(bundle.visual(15).valid());
            destroy_bundle(bundle);
        }
    }

    TEST_CASE("a surface-backed image builds a bundle without a decode round-trip (Flavor 1)") {
        neutrino::test::test_application app("tileset bundle from surface");

        // A uniform tileset whose shared image is an already-decoded surface (procedural
        // or an in-memory importer). It must slice and register exactly like a file image.
        world_tileset ts;
        ts.first_gid = 1;
        ts.tile_width = 16;
        ts.tile_height = 16;
        ts.columns = 4;
        ts.tile_count = 16;
        ts.image = surface_image(64, 64);

        tileset_bundle bundle = build_bundle(ts);

        CHECK(bundle.visuals.size() == 16);
        for (world_local_tile_id id = 0; id < 16; ++id) {
            CHECK(bundle.visual(id).valid());
        }
        destroy_bundle(bundle);
    }

    TEST_CASE("collection tileset resolves present tiles and leaves gaps invalid") {
        neutrino::test::test_application app("tileset bundle collection");

        world_tileset ts;
        ts.first_gid = 100;
        ts.tile_count = 3; // ids 0 and 2 present, id 1 is a gap

        world_tile t0;
        t0.id = 0;
        t0.image = bmp_image(24, 32);
        world_tile t2;
        t2.id = 2;
        t2.image = bmp_image(10, 10);
        ts.tiles = {t0, t2};

        tileset_bundle bundle = build_bundle(ts);

        CHECK(bundle.visuals.size() == 3);
        CHECK(bundle.visual(0).valid());
        CHECK_FALSE(bundle.visual(1).valid());   // gap: no image, no shared tileset image
        CHECK(bundle.visual(2).valid());
        CHECK(bundle.visual(0) != bundle.visual(2));

        // Two small tiles fit one page, so one atlas and one sheet back them both.
        CHECK(bundle.atlases.size() == 1);
        CHECK(bundle.sheets.size() == 1);

        destroy_bundle(bundle);
    }

    TEST_CASE("a collection tileset with sparse ids past tile_count resolves the high ids") {
        neutrino::test::test_application app("tileset bundle sparse ids");

        // Image-collection tilesets can number tiles sparsely, past tile_count (Tiled
        // writes them that way). The bundle must size its id tables to the highest id.
        world_tileset ts;
        ts.first_gid = 1;
        ts.tile_count = 2; // but ids run to 9

        world_tile t0;
        t0.id = 0;
        t0.image = bmp_image(16, 16);
        world_tile t9;
        t9.id = 9;
        t9.image = bmp_image(16, 16);
        ts.tiles = {t0, t9};

        tileset_bundle bundle = build_bundle(ts);

        REQUIRE(bundle.visuals.size() >= 10);   // spans the highest id
        CHECK(bundle.visual(0).valid());
        CHECK(bundle.visual(9).valid());        // the sparse high id resolves (was invalid before)
        CHECK_FALSE(bundle.visual(5).valid());  // a genuine gap stays invalid

        destroy_bundle(bundle);
    }

    TEST_CASE("collection tileset spills across pages under an explicit cap") {
        neutrino::test::test_application app("tileset bundle spill");

        // Two 40x40 tiles are 42x42 each with their gutter belts, so two cannot
        // share a 64x64 page: an explicit 64x64 cap forces one page (and one sheet)
        // per tile, exercising the multi-page ref-map deterministically.
        world_tileset ts;
        ts.first_gid = 1;
        ts.tile_count = 2;
        world_tile t0;
        t0.id = 0;
        t0.image = bmp_image(40, 40);
        world_tile t1;
        t1.id = 1;
        t1.image = bmp_image(40, 40);
        ts.tiles = {t0, t1};

        tileset_bundle bundle = build_bundle(ts, dim{64, 64});

        CHECK(bundle.atlases.size() == 2); // one uploaded page per tile
        CHECK(bundle.sheets.size() == 2);  // one sheet per page (a sheet is single-atlas)
        REQUIRE(bundle.visual(0).valid());
        REQUIRE(bundle.visual(1).valid());
        // The point of the ref-map: the two tiles resolve through different sheets,
        // not one shared sheet + identity index.
        CHECK(bundle.visual(0).sheet != bundle.visual(1).sheet);

        destroy_bundle(bundle);
    }

    TEST_CASE("animated tile gets a shared state resolving to its first frame visual") {
        neutrino::test::test_application app("tileset bundle animation");

        world_tileset ts = uniform_tileset(32, 16, 2, 4);
        world_tile animated;
        animated.id = 1;
        animated.animation = {
            {0u, std::chrono::milliseconds{100}},
            {2u, std::chrono::milliseconds{150}}
        };
        ts.tiles = {animated};

        tileset_bundle bundle = build_bundle(ts);

        // Every grid tile still resolves to a visual, including the animated one.
        for (world_local_tile_id id = 0; id < 4; ++id) {
            CHECK(bundle.visual(id).valid());
        }

        // Only tile 1 is animated: it has a state; the others do not.
        REQUIRE(bundle.state(1).valid());
        CHECK_FALSE(bundle.state(0).valid());
        CHECK_FALSE(bundle.state(2).valid());
        CHECK(bundle.animations.size() == 1);

        // A fresh state sits at elapsed zero -> the first frame, whose visual is the
        // one built for the frame's referenced tile (tile 0).
        const sprite_appearance appearance = sprite_state_appearance(bundle.state(1));
        CHECK(appearance.visual == bundle.visual(0));

        destroy_bundle(bundle);
    }

    TEST_CASE("teardown is order-clean and the tileset rebuilds") {
        neutrino::test::test_application app("tileset bundle teardown");

        const world_tileset ts = uniform_tileset(32, 16, 2, 4);

        tileset_bundle first = build_bundle(ts);
        REQUIRE_FALSE(first.sheets.empty());
        destroy_bundle(first); // states -> animations -> sheets -> atlases, no uses() violation

        // The resources are fully released, so an identical tileset builds again.
        tileset_bundle second = build_bundle(ts);
        CHECK(second.visuals.size() == 4);
        for (world_local_tile_id id = 0; id < 4; ++id) {
            CHECK(second.visual(id).valid());
        }
        destroy_bundle(second);
    }

    TEST_CASE("moving a bundle leaves the source empty so its destructor cannot double-free") {
        neutrino::test::test_application app("tileset bundle move empties source");

        const world_tileset ts = uniform_tileset(32, 16, 2, 4);

        SUBCASE("move construction empties the source") {
            tileset_bundle src = build_bundle(ts);
            REQUIRE_FALSE(src.sheets.empty());
            tileset_bundle dst(std::move(src));

            CHECK(src.atlases.empty());
            CHECK(src.sheets.empty());
            CHECK(src.visuals.empty());
            CHECK(src.states.empty());
            CHECK(src.animations.empty());
            CHECK_FALSE(dst.sheets.empty()); // ownership transferred intact
        }

        SUBCASE("move assignment empties the source and releases the target's old resources") {
            tileset_bundle src = build_bundle(ts);
            tileset_bundle dst = build_bundle(ts); // dst starts with its own live resources
            REQUIRE_FALSE(src.sheets.empty());
            REQUIRE_FALSE(dst.sheets.empty());

            dst = std::move(src); // must release dst's old resources, then adopt src's

            CHECK(src.atlases.empty());
            CHECK(src.sheets.empty());
            CHECK(src.visuals.empty());
            CHECK(src.states.empty());
            CHECK(src.animations.empty());
            CHECK_FALSE(dst.sheets.empty());
        }
        // Both bundles now destruct: the empty sources no-op, so no id is unregistered
        // twice (a double-free would trip the sprite manager under the hood).
    }

    TEST_CASE("a bundle left to fall out of scope releases its resources (RAII)") {
        neutrino::test::test_application app("tileset bundle raii");

        const world_tileset ts = uniform_tileset(32, 16, 2, 4);
        {
            tileset_bundle scoped = build_bundle(ts);
            REQUIRE_FALSE(scoped.sheets.empty());
            // No destroy_bundle call: the destructor must unregister everything.
        }

        // If the destructor did not release the sheets/atlases, rebuilding the same
        // tileset would eventually trip the sprite manager's uses() guard or leak; a
        // clean rebuild confirms the destructor tore the previous bundle down.
        tileset_bundle again = build_bundle(ts);
        CHECK(again.visuals.size() == 4);
        for (world_local_tile_id id = 0; id < 4; ++id) {
            CHECK(again.visual(id).valid());
        }
    }
}
