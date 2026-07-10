#include <doctest/doctest.h>

#include <neutrino/video/world/world_renderer.hh>
#include <neutrino/video/world/resource_cache.hh>

#include "test_application.hh"
#include "services/service_locator.hh"
#include "video/sprite/sprites_manager.hh"

#include <sdlpp/video/surface.hh>

#include <chrono>
#include <utility>
#include <vector>

using namespace neutrino;

namespace {
    // Embedded BMP of a w*h surface, decodable by build_bundle through load_image
    // with no on-disk asset.
    world_image bmp_image(unsigned w, unsigned h) {
        auto surface = sdlpp::surface::create_rgb(
            static_cast <int>(w), static_cast <int>(h), sdlpp::pixel_format_enum::RGBA8888);
        REQUIRE(surface.has_value());
        auto bytes = sdlpp::save_bmp(*surface);
        REQUIRE(bytes.has_value());

        world_image img;
        img.width = w;
        img.height = h;
        img.data = std::move(*bytes);
        return img;
    }

    // 16x16 tiles, a 2x2 grid in a 32x32 image (gids first_gid..first_gid+3).
    world_tileset square_ts(world_tile_id first_gid = 1) {
        world_tileset ts;
        ts.first_gid = first_gid;
        ts.tile_width = 16;
        ts.tile_height = 16;
        ts.columns = 2;
        ts.tile_count = 4;
        ts.image = bmp_image(32, 32);
        return ts;
    }

    // A single 16x32 tile (twice as tall as the map cell) -> one cell of overhang.
    world_tileset tall_ts() {
        world_tileset ts;
        ts.first_gid = 1;
        ts.tile_width = 16;
        ts.tile_height = 32;
        ts.columns = 1;
        ts.tile_count = 1;
        ts.image = bmp_image(16, 32);
        return ts;
    }

    world_tile_layer filled_layer(unsigned w, unsigned h, world_tile_id gid) {
        world_tile_layer layer;
        layer.width = w;
        layer.height = h;
        layer.visible = true;
        layer.cells.assign(static_cast <std::size_t>(w) * h, world_tile_cell{});
        for (world_tile_cell& c : layer.cells) {
            c.gid = gid;
        }
        return layer;
    }

    // square_ts with local tile id 1 (gid 2) animated: frame 0 -> tile 0 for 100ms,
    // then frame 1 -> tile 2 for 150ms.
    world_tileset animated_ts() {
        world_tileset ts = square_ts();
        world_tile animated;
        animated.id = 1;
        animated.animation = {
            {0u, std::chrono::milliseconds{100}},
            {2u, std::chrono::milliseconds{150}}
        };
        ts.tiles = {animated};
        return ts;
    }

    sprites_manager& manager() {
        sprites_manager* m = service_locator::instance().get_sprites_manager();
        REQUIRE(m != nullptr);
        return *m;
    }

    // A world with a 16px cell grid carrying one tileset and one tile layer.
    world make_world(world_tileset ts, world_tile_layer layer, unsigned cell = 16) {
        world w;
        w.set_tile_size(cell, cell);
        w.set_size(layer.width, layer.height);
        w.add_tileset(std::move(ts));
        w.add_layer(std::move(layer));
        return w;
    }

    // A look-at camera whose view top-left sits at world (0,0) for the given viewport
    // -- i.e. it shows the world rect [0, viewport) at zoom 1, the natural frame for
    // these fixtures (target = the viewport centre in world pixels).
    camera origin_camera(rect viewport) {
        camera cam;
        cam.target = world_point{static_cast <float>(viewport.w) * 0.5f,
                                 static_cast <float>(viewport.h) * 0.5f};
        return cam;
    }

    // One tile object at (0,16) with the given gid/visibility/transform.
    world_object_layer tile_object_layer(world_tile_id gid, bool visible = true,
                                         double rotation = 0.0,
                                         sprite_flip flip = sprite_flip::none) {
        world_object_layer layer;
        layer.visible = true;
        world_rectangle_object obj;
        obj.gid = gid;
        obj.origin = {0.0f, 16.0f};
        obj.rotation = rotation;
        obj.flip = flip;
        obj.visible = visible;
        layer.objects.push_back(obj);
        return layer;
    }

    world world_with(world_tileset ts, world_layer layer) {
        world w;
        w.set_tile_size(16, 16);
        w.set_size(8, 8);
        w.add_tileset(std::move(ts));
        w.add_layer(std::move(layer));
        return w;
    }

    world_tile_chunk chunk_at(int x, int y, int w, int h, world_tile_id gid) {
        world_tile_chunk c;
        c.x = x;
        c.y = y;
        c.width = w;
        c.height = h;
        c.cells.assign(static_cast <std::size_t>(w) * h, world_tile_cell{});
        for (world_tile_cell& cell : c.cells) {
            cell.gid = gid;
        }
        return c;
    }

    // An infinite (chunked) world with a 16px cell grid and one chunked tile layer.
    world infinite_world(std::vector <world_tile_chunk> chunks) {
        world w;
        w.set_tile_size(16, 16);
        w.set_infinite(true);
        w.add_tileset(square_ts());
        world_tile_layer layer;
        layer.visible = true; // width/height stay 0 -- infinite layers are chunk-addressed
        layer.chunks = std::move(chunks);
        w.add_layer(std::move(layer));
        return w;
    }
}

TEST_SUITE("neutrino::video world draw") {
    TEST_CASE("a finite orthogonal layer draws every non-empty cell") {
        neutrino::test::test_application app("world draw full");
        resource_cache cache;

        world w = make_world(square_ts(), filled_layer(4, 4, 1));
        world_renderer r(w, cache);

        const draw_stats stats = r.draw(origin_camera(rect{0, 0, 64, 64}), rect{0, 0, 64, 64});

        CHECK(stats.drawn == 16); // 4x4 all non-empty, viewport shows all
        CHECK(stats.skipped == 0);
        CHECK(stats.failed == 0);
    }

    TEST_CASE("empty cells are absent, not skipped") {
        neutrino::test::test_application app("world draw empties");
        resource_cache cache;

        world_tile_layer layer = filled_layer(4, 4, 1);
        // Zero out a diagonal: 4 empty cells, 12 non-empty.
        for (unsigned i = 0; i < 4; ++i) {
            layer.cells[i * 4 + i].gid = 0;
        }
        world w = make_world(square_ts(), std::move(layer));
        world_renderer r(w, cache);

        const draw_stats stats = r.draw(origin_camera(rect{0, 0, 64, 64}), rect{0, 0, 64, 64});

        CHECK(stats.drawn == 12);
        CHECK(stats.skipped == 0); // gid 0 is not counted
        CHECK(stats.failed == 0);
    }

    TEST_CASE("a non-empty gid that does not resolve is skipped, not fatal") {
        neutrino::test::test_application app("world draw skip");
        resource_cache cache;

        SUBCASE("gid below every first_gid has no owning tileset") {
            world w = make_world(square_ts(10), filled_layer(1, 1, 5)); // 5 < first_gid 10
            world_renderer r(w, cache);
            const draw_stats stats = r.draw(origin_camera(rect{0, 0, 64, 64}), rect{0, 0, 64, 64});
            CHECK(stats.drawn == 0);
            CHECK(stats.skipped == 1);
            CHECK(stats.failed == 0);
        }
        SUBCASE("gid past the tileset's tile_count has no valid visual") {
            world w = make_world(square_ts(1), filled_layer(1, 1, 99)); // local 98, absent
            world_renderer r(w, cache);
            const draw_stats stats = r.draw(origin_camera(rect{0, 0, 64, 64}), rect{0, 0, 64, 64});
            CHECK(stats.drawn == 0);
            CHECK(stats.skipped == 1);
            CHECK(stats.failed == 0);
        }
    }

    TEST_CASE("flipped and hex-rotated cells draw without error") {
        neutrino::test::test_application app("world draw transforms");
        resource_cache cache;

        world_tile_layer layer = filled_layer(2, 1, 1);
        layer.cells[0].flip = sprite_flip::horizontal;
        layer.cells[1].flip = sprite_flip::diagonal;
        layer.cells[1].rotation_degrees = 120.0f; // hex-120
        world w = make_world(square_ts(), std::move(layer));
        world_renderer r(w, cache);

        const draw_stats stats = r.draw(origin_camera(rect{0, 0, 64, 64}), rect{0, 0, 64, 64});

        CHECK(stats.drawn == 2);
        CHECK(stats.failed == 0);
    }

    TEST_CASE("the viewport origin repositions the map without changing what draws") {
        neutrino::test::test_application app("world draw viewport origin");
        resource_cache cache;

        world w = make_world(square_ts(), filled_layer(4, 4, 1));
        world_renderer r(w, cache);

        const draw_stats at_origin = r.draw(origin_camera(rect{0, 0, 64, 64}), rect{0, 0, 64, 64});
        const draw_stats inset = r.draw(origin_camera(rect{100, 50, 64, 64}), rect{100, 50, 64, 64});

        CHECK(inset.drawn == at_origin.drawn);
        CHECK(inset.failed == 0);
    }

    TEST_CASE("every render order visits the full visible range") {
        neutrino::test::test_application app("world draw render order");
        resource_cache cache;

        for (world_render_order order : {world_render_order::right_down,
                                        world_render_order::right_up,
                                        world_render_order::left_down,
                                        world_render_order::left_up}) {
            world w = make_world(square_ts(), filled_layer(4, 4, 1));
            w.set_render_order(order);
            world_renderer r(w, cache);

            const draw_stats stats = r.draw(origin_camera(rect{0, 0, 64, 64}), rect{0, 0, 64, 64});

            CHECK(stats.drawn == 16); // full coverage regardless of iteration direction
            CHECK(stats.failed == 0);
        }
    }

    TEST_CASE("an invisible layer draws nothing") {
        neutrino::test::test_application app("world draw invisible");
        resource_cache cache;

        world_tile_layer layer = filled_layer(4, 4, 1);
        layer.visible = false;
        world w = make_world(square_ts(), std::move(layer));
        world_renderer r(w, cache);

        const draw_stats stats = r.draw(origin_camera(rect{0, 0, 64, 64}), rect{0, 0, 64, 64});

        CHECK(stats.drawn == 0);
        CHECK(stats.skipped == 0);
    }

    TEST_CASE("overhang inflation draws an oversized tile whose cell is just off-screen") {
        neutrino::test::test_application app("world draw overhang");
        resource_cache cache;

        // Only row 3 carries tiles; the viewport grid-covers rows 0..2 (48px / 16).
        // A tall tile at row 3 overhangs upward and must still be drawn; a same-size
        // tile at row 3 does not overhang and is correctly culled.
        auto row3_layer = [] {
            world_tile_layer layer;
            layer.width = 3;
            layer.height = 5;
            layer.visible = true;
            layer.cells.assign(15, world_tile_cell{});
            for (unsigned x = 0; x < 3; ++x) {
                layer.cells[3 * 3 + x].gid = 1; // row y=3
            }
            return layer;
        };

        SUBCASE("tall tileset: the overhang row is inflated in and drawn") {
            world w = make_world(tall_ts(), row3_layer());
            world_renderer r(w, cache);
            const draw_stats stats = r.draw(origin_camera(rect{0, 0, 48, 48}), rect{0, 0, 48, 48});
            CHECK(stats.drawn == 3); // row 3 pulled in by 1 cell of overhang
            CHECK(stats.failed == 0);
        }
        SUBCASE("square tileset: no overhang, the off-screen row is culled") {
            world w = make_world(square_ts(), row3_layer());
            world_renderer r(w, cache);
            const draw_stats stats = r.draw(origin_camera(rect{0, 0, 48, 48}), rect{0, 0, 48, 48});
            CHECK(stats.drawn == 0); // grid range [0,3) excludes row 3
        }
    }

    TEST_CASE("an animated tile resolves to the current frame from the shared clock") {
        neutrino::test::test_application app("world draw animated clock");
        resource_cache cache;

        world w = make_world(animated_ts(), filled_layer(2, 2, 2)); // gid 2 = animated local id 1
        world_renderer r(w, cache);

        const bundle_handle& handle = r.handle_for_tileset(0);
        const sprite_state_id st = handle.state(1);
        REQUIRE(st.valid());

        // t=0 -> first frame references tile 0.
        CHECK(sprite_state_appearance(st).visual == handle.visual(0));

        // Advance past the 100ms first frame -> second frame references tile 2.
        manager().update(sprite_animation_duration{std::chrono::milliseconds{120}});
        CHECK(sprite_state_appearance(st).visual == handle.visual(2));

        // Every animated cell still draws (through the shared state, in lockstep).
        const draw_stats stats = r.draw(origin_camera(rect{0, 0, 64, 64}), rect{0, 0, 64, 64});
        CHECK(stats.drawn == 4);
        CHECK(stats.failed == 0);
    }

    TEST_CASE("a flipped animated cell draws without error") {
        neutrino::test::test_application app("world draw animated flip");
        resource_cache cache;

        world_tile_layer layer = filled_layer(1, 1, 2);
        layer.cells[0].flip = sprite_flip::horizontal;
        world w = make_world(animated_ts(), std::move(layer));
        world_renderer r(w, cache);

        manager().update(sprite_animation_duration{std::chrono::milliseconds{120}});
        const draw_stats stats = r.draw(origin_camera(rect{0, 0, 64, 64}), rect{0, 0, 64, 64});

        CHECK(stats.drawn == 1);
        CHECK(stats.failed == 0);
    }

    TEST_CASE("animation survives a resident-tileset switch without resetting") {
        neutrino::test::test_application app("world draw animated switch");
        resource_cache cache;

        world w1 = make_world(animated_ts(), filled_layer(2, 2, 2));
        world_renderer r(w1, cache);

        const sprite_state_id before = r.handle_for_tileset(0).state(1);
        REQUIRE(before.valid());
        manager().update(sprite_animation_duration{std::chrono::milliseconds{120}}); // into frame 1
        REQUIRE(sprite_state_appearance(before).visual == r.handle_for_tileset(0).visual(2));

        // Switch to a different world sharing the identical tileset: the bundle stays
        // resident (acquire-before-release), so the shared state and its clock persist.
        world w2 = make_world(animated_ts(), filled_layer(3, 3, 2));
        r.switch_to(w2);

        const sprite_state_id after = r.handle_for_tileset(0).state(1);
        CHECK(after == before);                                                  // not rebuilt
        CHECK(sprite_state_appearance(after).visual == r.handle_for_tileset(0).visual(2)); // still frame 1
    }

    TEST_CASE("a tile object draws through the same gid path as a cell") {
        neutrino::test::test_application app("world draw tile object");
        resource_cache cache;

        world w = world_with(square_ts(), tile_object_layer(1));
        world_renderer r(w, cache);

        const draw_stats stats = r.draw(origin_camera(rect{0, 0, 64, 64}), rect{0, 0, 64, 64});

        CHECK(stats.drawn == 1);
        CHECK(stats.skipped == 0);
        CHECK(stats.failed == 0);
    }

    TEST_CASE("non-tile and hidden objects are neither drawn nor counted") {
        neutrino::test::test_application app("world draw object skip");
        resource_cache cache;

        SUBCASE("a shape object (gid 0) is skipped silently") {
            world w = world_with(square_ts(), tile_object_layer(0));
            world_renderer r(w, cache);
            const draw_stats stats = r.draw(origin_camera(rect{0, 0, 64, 64}), rect{0, 0, 64, 64});
            CHECK(stats.drawn == 0);
            CHECK(stats.skipped == 0); // not sprite-renderable -> absent, not a skip
        }
        SUBCASE("a hidden tile object is skipped silently") {
            world w = world_with(square_ts(), tile_object_layer(1, /*visible=*/false));
            world_renderer r(w, cache);
            const draw_stats stats = r.draw(origin_camera(rect{0, 0, 64, 64}), rect{0, 0, 64, 64});
            CHECK(stats.drawn == 0);
            CHECK(stats.skipped == 0);
        }
    }

    TEST_CASE("a rotated, flipped tile object draws without error") {
        neutrino::test::test_application app("world draw object transform");
        resource_cache cache;

        world w = world_with(square_ts(), tile_object_layer(1, true, 120.0, sprite_flip::diagonal));
        world_renderer r(w, cache);

        const draw_stats stats = r.draw(origin_camera(rect{0, 0, 64, 64}), rect{0, 0, 64, 64});

        CHECK(stats.drawn == 1);
        CHECK(stats.failed == 0);
    }

    TEST_CASE("an animated tile object plays off the shared clock") {
        neutrino::test::test_application app("world draw object animated");
        resource_cache cache;

        world w = world_with(animated_ts(), tile_object_layer(2)); // gid 2 = animated local id 1
        world_renderer r(w, cache);

        manager().update(sprite_animation_duration{std::chrono::milliseconds{120}});
        const draw_stats stats = r.draw(origin_camera(rect{0, 0, 64, 64}), rect{0, 0, 64, 64});

        CHECK(stats.drawn == 1);
        CHECK(stats.failed == 0);
    }

    TEST_CASE("an image layer draws its picture once") {
        neutrino::test::test_application app("world draw image layer");
        resource_cache cache;

        world w;
        w.set_tile_size(16, 16);
        world_image_layer img;
        img.visible = true;
        img.image = bmp_image(32, 32);
        w.add_layer(std::move(img));

        world_renderer r(w, cache);
        const draw_stats stats = r.draw(origin_camera(rect{0, 0, 64, 64}), rect{0, 0, 64, 64});

        CHECK(stats.drawn == 1);
        CHECK(stats.failed == 0);
    }

    TEST_CASE("an infinite map draws only the chunks the camera intersects") {
        neutrino::test::test_application app("world draw infinite");
        resource_cache cache;

        // Chunk A at the origin (visible), chunk B far away (must never be visited).
        world w = infinite_world({
            chunk_at(0, 0, 16, 16, 1),
            chunk_at(100, 100, 16, 16, 1),
        });
        world_renderer r(w, cache);

        const draw_stats stats = r.draw(origin_camera(rect{0, 0, 64, 64}), rect{0, 0, 64, 64});

        // Only the visible 4x4 corner of chunk A draws (64/16); chunk B contributes 0.
        CHECK(stats.drawn == 16);
        CHECK(stats.failed == 0);
    }

    TEST_CASE("an infinite map renders chunks and cells at negative coordinates") {
        neutrino::test::test_application app("world draw infinite negative");
        resource_cache cache;

        world w = infinite_world({chunk_at(-16, -16, 16, 16, 1)});
        world_renderer r(w, cache);

        camera cam;
        cam.target = {-32.0f, -32.0f}; // centre the view on the bottom-right corner

        const draw_stats stats = r.draw(cam, rect{0, 0, 64, 64});

        CHECK(stats.drawn == 16); // cells [-4,0) x [-4,0)
        CHECK(stats.failed == 0);
    }

    TEST_CASE("a hexagonal map draws every cell at its staggered position") {
        neutrino::test::test_application app("world draw hex");
        resource_cache cache;

        world w;
        w.set_tile_size(64, 64);
        w.set_orientation(world_orientation::hexagonal);
        w.set_hex_side_length(32);
        w.set_stagger_axis(world_stagger_axis::y);
        w.set_stagger_index(world_stagger_index::odd);
        w.set_size(4, 4);
        w.add_tileset(square_ts());          // any tileset; tiles draw at the hex anchors
        w.add_layer(filled_layer(4, 4, 1));

        world_renderer r(w, cache);
        // A view large enough to cover the whole staggered 4x4 field.
        const draw_stats stats = r.draw(origin_camera(rect{0, 0, 512, 512}), rect{0, 0, 512, 512});

        CHECK(stats.drawn == 16); // all cells drawn -- no orthogonal-grid assumption
        CHECK(stats.skipped == 0);
        CHECK(stats.failed == 0);
    }

    TEST_CASE("a staggered map draws every cell") {
        neutrino::test::test_application app("world draw staggered");
        resource_cache cache;

        world w;
        w.set_tile_size(64, 64);
        w.set_orientation(world_orientation::staggered); // hex_side_length 0
        w.set_stagger_axis(world_stagger_axis::y);
        w.set_stagger_index(world_stagger_index::odd);
        w.set_size(4, 4);
        w.add_tileset(square_ts());
        w.add_layer(filled_layer(4, 4, 1));

        world_renderer r(w, cache);
        const draw_stats stats = r.draw(origin_camera(rect{0, 0, 512, 512}), rect{0, 0, 512, 512});

        CHECK(stats.drawn == 16);
        CHECK(stats.failed == 0);
    }

    TEST_CASE("an isometric map draws every cell") {
        neutrino::test::test_application app("world draw isometric");
        resource_cache cache;

        world w;
        w.set_tile_size(64, 64);
        w.set_orientation(world_orientation::isometric);
        w.set_size(4, 4);
        w.add_tileset(square_ts());
        w.add_layer(filled_layer(4, 4, 1));

        world_renderer r(w, cache);
        const draw_stats stats = r.draw(origin_camera(rect{0, 0, 640, 640}), rect{0, 0, 640, 640});

        CHECK(stats.drawn == 16); // all diamond cells drawn, none skipped/failed
        CHECK(stats.failed == 0);
        CHECK(stats.skipped == 0);
    }

    TEST_CASE("an infinite map with the camera off every chunk draws nothing") {
        neutrino::test::test_application app("world draw infinite empty view");
        resource_cache cache;

        world w = infinite_world({chunk_at(0, 0, 16, 16, 1)});
        world_renderer r(w, cache);

        camera cam;
        cam.target = {1000.0f, 1000.0f}; // far from the only chunk

        const draw_stats stats = r.draw(cam, rect{0, 0, 64, 64});

        CHECK(stats.drawn == 0);
    }
}
