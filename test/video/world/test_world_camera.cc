#include <doctest/doctest.h>

#include <neutrino/video/world/camera.hh>

#include <limits>

using namespace neutrino;

namespace {
    world make_world(unsigned tile_w, unsigned tile_h) {
        world w;
        w.set_tile_size(tile_w, tile_h);
        return w;
    }

    world_tile_layer make_layer(unsigned w, unsigned h) {
        world_tile_layer layer;
        layer.width = w;
        layer.height = h;
        return layer;
    }

    // The look-at target that puts `world_topleft` at the viewport's top-left corner
    // (target = top-left + half the view), so range tests read in top-left terms.
    world_point target_at(world_point world_topleft, dim vp, float zoom = 1.0f) {
        return world_point{world_topleft.x + static_cast <float>(vp.width) / (2.0f * zoom),
                           world_topleft.y + static_cast <float>(vp.height) / (2.0f * zoom)};
    }
}

TEST_SUITE("neutrino::camera") {
    TEST_CASE("visible_cell_range matches the viewport rect at the origin") {
        const auto w = make_world(16, 16);
        const auto layer = make_layer(100, 100);
        camera cam;
        cam.target = target_at({0.0f, 0.0f}, dim{160, 144}); // view top-left at world origin

        const auto r = visible_cell_range(w, layer, cam, dim{160, 144});

        // 160/16 = 10 columns, 144/16 = 9 rows, anchored at the top-left cell.
        CHECK(r.x0 == 0);
        CHECK(r.y0 == 0);
        CHECK(r.x1 == 10);
        CHECK(r.y1 == 9);
        CHECK_FALSE(r.empty());
    }

    TEST_CASE("moving the target shifts the range by whole cells, size preserved") {
        const auto w = make_world(16, 16);
        const auto layer = make_layer(100, 100);
        camera cam;
        cam.target = target_at({32.0f, 16.0f}, dim{160, 144}); // 2 cells right, 1 cell down

        const auto r = visible_cell_range(w, layer, cam, dim{160, 144});

        CHECK(r.x0 == 2);
        CHECK(r.y0 == 1);
        CHECK(r.x1 == 12); // still 10 columns wide
        CHECK(r.y1 == 10); // still 9 rows tall
    }

    TEST_CASE("zoom grows/shrinks the visible cell count") {
        const auto w = make_world(16, 16);
        const auto layer = make_layer(100, 100);

        camera cam;

        SUBCASE("zoom in shows fewer cells") {
            cam.zoom = 2.0f;
            // Cell-aligned interior top-left (800 = 50*16) so counts are exact.
            cam.target = target_at({800.0f, 800.0f}, dim{160, 144}, cam.zoom);
            const auto r = visible_cell_range(w, layer, cam, dim{160, 144});
            CHECK(r.x1 - r.x0 == 5); // 80x72 world -> 5x5 cells
            CHECK(r.y1 - r.y0 == 5);
        }
        SUBCASE("zoom out shows more cells") {
            cam.zoom = 0.5f;
            cam.target = target_at({800.0f, 800.0f}, dim{160, 144}, cam.zoom);
            const auto r = visible_cell_range(w, layer, cam, dim{160, 144});
            CHECK(r.x1 - r.x0 == 20); // 320x288 world -> 20x18 cells
            CHECK(r.y1 - r.y0 == 18);
        }
    }

    TEST_CASE("range clamps at every layer edge, never negative or out of bounds") {
        const auto w = make_world(16, 16);
        const auto layer = make_layer(5, 5); // 80x80 px

        SUBCASE("top-left overhang clamps to 0") {
            camera cam;
            cam.target = target_at({-40.0f, -40.0f}, dim{160, 160});
            const auto r = visible_cell_range(w, layer, cam, dim{160, 160});
            CHECK(r.x0 == 0);
            CHECK(r.y0 == 0);
        }
        SUBCASE("bottom-right overhang clamps to the layer size") {
            camera cam;
            cam.target = target_at({0.0f, 0.0f}, dim{320, 320}); // view far larger than the layer
            const auto r = visible_cell_range(w, layer, cam, dim{320, 320});
            CHECK(r.x1 == 5);
            CHECK(r.y1 == 5);
        }
        SUBCASE("viewport larger than the layer on all sides -> full layer") {
            camera cam;
            cam.target = target_at({-40.0f, -40.0f}, dim{320, 320});
            const auto r = visible_cell_range(w, layer, cam, dim{320, 320});
            CHECK(r.x0 == 0);
            CHECK(r.y0 == 0);
            CHECK(r.x1 == 5);
            CHECK(r.y1 == 5);
            CHECK_FALSE(r.empty());
        }
    }

    TEST_CASE("parallax and layer offset shift the range") {
        const auto w = make_world(16, 16);
        camera cam;
        cam.target = target_at({200.0f, 0.0f}, dim{160, 144});

        SUBCASE("a background layer (parallax < 1) shifts less than the foreground") {
            auto foreground = make_layer(100, 100); // parallax 1 by default
            auto background = make_layer(100, 100);
            background.parallax_x = 0.5f;
            background.parallax_y = 0.5f;

            const auto fg = visible_cell_range(w, foreground, cam, dim{160, 144});
            const auto bg = visible_cell_range(w, background, cam, dim{160, 144});

            CHECK(fg.x0 == 12); // floor(200/16)
            CHECK(bg.x0 == 3);  // effective target 0.5*280 -> top-left 60 -> floor(60/16)
            CHECK(bg.x0 < fg.x0);
        }
        SUBCASE("a positive layer offset moves the origin earlier") {
            auto layer = make_layer(100, 100);
            layer.offset = {16.0f, 0.0f};
            const auto r = visible_cell_range(w, layer, cam, dim{160, 144});
            CHECK(r.x0 == 11); // floor((200 - 16)/16)
        }
    }

    TEST_CASE("parallax: effective target rest + factor*(target - rest)") {
        camera cam;
        cam.target = {200.0f, 0.0f}; // world point at the viewport centre
        cam.zoom = 1.0f;
        const dim vp{200, 0}; // half-view.x = 100
        world_tile_layer fg = make_layer(100, 100); // parallax 1
        world_tile_layer bg = make_layer(100, 100);
        bg.parallax_x = 0.5f;
        bg.parallax_y = 0.5f;
        const world_point p{50.0f, 0.0f};

        SUBCASE("foreground centres on the target; background at factor of it") {
            CHECK(to_screen(cam, fg, vp, p).x == -50); // origin = target - half = 100; 50-100
            CHECK(to_screen(cam, bg, vp, p).x == 50);  // eff = 0.5*200 = 100; origin = 0; 50-0
        }
        SUBCASE("the parallax rest shifts the background") {
            cam.parallax_rest = {40.0f, 0.0f};        // eff = 40 + 0.5*(200-40) = 120; origin = 20
            CHECK(to_screen(cam, bg, vp, p).x == 30); // 50 - 20
        }
        SUBCASE("the scene assembles when the target equals the parallax rest") {
            cam.parallax_rest = cam.target;           // eff = target for every factor
            CHECK(to_screen(cam, fg, vp, p).x == to_screen(cam, bg, vp, p).x);
        }
        SUBCASE("background scrolls slower than the foreground when the target moves") {
            const int fg0 = to_screen(cam, fg, vp, p).x;
            const int bg0 = to_screen(cam, bg, vp, p).x;
            cam.target.x += 100.0f;
            CHECK(to_screen(cam, fg, vp, p).x - fg0 == -100); // full scroll
            CHECK(to_screen(cam, bg, vp, p).x - bg0 == -50);  // half (factor 0.5)
        }
    }

    TEST_CASE("a camera fully off the layer yields an empty range") {
        const auto w = make_world(16, 16);
        const auto layer = make_layer(5, 5);

        SUBCASE("far past the layer") {
            camera cam;
            cam.target = {1000.0f, 1000.0f};
            CHECK(visible_cell_range(w, layer, cam, dim{160, 160}).empty());
        }
        SUBCASE("far before the layer") {
            camera cam;
            cam.target = {-1000.0f, -1000.0f};
            CHECK(visible_cell_range(w, layer, cam, dim{160, 160}).empty());
        }
    }

    TEST_CASE("non-positive or non-finite zoom throws") {
        const auto w = make_world(16, 16);
        const auto layer = make_layer(10, 10);
        camera cam;

        cam.zoom = 0.0f;
        CHECK_THROWS((void) visible_cell_range(w, layer, cam, dim{160, 160}));
        cam.zoom = -1.0f;
        CHECK_THROWS((void) visible_cell_range(w, layer, cam, dim{160, 160}));
        cam.zoom = std::numeric_limits <float>::infinity();
        CHECK_THROWS((void) visible_cell_range(w, layer, cam, dim{160, 160}));
    }

    TEST_CASE("to_screen projects world points under target, zoom and parallax") {
        camera cam;
        cam.target = {10.0f, 20.0f};
        cam.zoom = 2.0f;
        auto layer = make_layer(100, 100);

        SUBCASE("layer at parallax 1 matches the HUD overload") {
            // dim{0,0} puts the view centre at the target, so the origin is the target.
            const auto with_layer = to_screen(cam, layer, dim{0, 0}, world_point{50.0f, 60.0f});
            const auto hud = to_screen(cam, dim{0, 0}, world_point{50.0f, 60.0f});
            CHECK(with_layer.x == 80); // (50-10)*2
            CHECK(with_layer.y == 80); // (60-20)*2
            CHECK(hud.x == with_layer.x);
            CHECK(hud.y == with_layer.y);
        }
        SUBCASE("parallax scales the effective target") {
            layer.parallax_x = 0.5f;
            layer.parallax_y = 0.5f;
            // eff = 0.5*target (rest 0); origin = eff (half-view 0 at dim{0,0}).
            const auto p = to_screen(cam, layer, dim{0, 0}, world_point{50.0f, 60.0f});
            CHECK(p.x == 90);  // (50 - 5)*2
            CHECK(p.y == 100); // (60 - 10)*2
        }
    }

    TEST_CASE("to_screen rounds to the nearest pixel") {
        camera cam; // target 0, zoom 1
        const auto p = to_screen(cam, dim{0, 0}, world_point{10.4f, 10.6f});
        CHECK(p.x == 10); // rounds down
        CHECK(p.y == 11); // rounds up (floor would give 10)
    }

    TEST_CASE("cell_to_world lays out an orthogonal grid at the cell bottom-left") {
        world w = make_world(16, 16); // orientation defaults to orthogonal-equivalent
        CHECK(cell_to_world(w, 2, 3).x == 32); // 2*16
        CHECK(cell_to_world(w, 2, 3).y == 64); // (3+1)*16
    }

    TEST_CASE("cell_to_world staggers hexagonal (stagger-Y) cells") {
        world w = make_world(64, 64);
        w.set_orientation(world_orientation::hexagonal);
        w.set_hex_side_length(32);
        w.set_stagger_axis(world_stagger_axis::y);   // rows staggered (pointy-top)
        w.set_stagger_index(world_stagger_index::odd);
        // row stride = (64-32)/2 + 32 = 48; column shift = 32. Anchors are bottom-left
        // (Tiled tile top-left + tile height). Verified against HexagonalRenderer.
        CHECK(cell_to_world(w, 0, 0).x == 0);   CHECK(cell_to_world(w, 0, 0).y == 64);  // top-left (0,0)
        CHECK(cell_to_world(w, 1, 0).x == 64);  CHECK(cell_to_world(w, 1, 0).y == 64);  // same row, next column
        CHECK(cell_to_world(w, 0, 1).x == 32);  CHECK(cell_to_world(w, 0, 1).y == 112); // odd row: +col shift, +row stride
        CHECK(cell_to_world(w, 0, 2).x == 0);   CHECK(cell_to_world(w, 0, 2).y == 160); // even row: no shift
    }

    TEST_CASE("cell_to_world staggers a staggered (side-length 0) map like half-height hex") {
        world w = make_world(64, 64);
        w.set_orientation(world_orientation::staggered);
        w.set_stagger_axis(world_stagger_axis::y);
        w.set_stagger_index(world_stagger_index::odd);
        // side length 0 -> col_w = row_h = 32 (rows compressed to half height).
        CHECK(cell_to_world(w, 0, 0).x == 0);   CHECK(cell_to_world(w, 0, 0).y == 64);
        CHECK(cell_to_world(w, 1, 0).x == 64);  CHECK(cell_to_world(w, 1, 0).y == 64);
        CHECK(cell_to_world(w, 0, 1).x == 32);  CHECK(cell_to_world(w, 0, 1).y == 96);  // odd row staggered
        CHECK(cell_to_world(w, 0, 2).x == 0);   CHECK(cell_to_world(w, 0, 2).y == 128); // even row not
    }

    TEST_CASE("cell_to_world projects isometric cells to the diamond bbox bottom-left") {
        world w = make_world(64, 64);
        w.set_orientation(world_orientation::isometric);
        w.set_size(4, 4); // height 4 -> originX = 4*64/2 = 128
        // anchor = (tileToScreenCoords.x - tw/2, y + th); verified vs IsometricRenderer.
        CHECK(cell_to_world(w, 0, 0).x == 96);   CHECK(cell_to_world(w, 0, 0).y == 64);
        CHECK(cell_to_world(w, 1, 0).x == 128);  CHECK(cell_to_world(w, 1, 0).y == 96);
        CHECK(cell_to_world(w, 0, 1).x == 64);   CHECK(cell_to_world(w, 0, 1).y == 96);
        CHECK(cell_to_world(w, 1, 1).x == 96);   CHECK(cell_to_world(w, 1, 1).y == 128);
    }

    TEST_CASE("cell_range::empty reflects half-open bounds") {
        CHECK(cell_range{}.empty());
        CHECK(cell_range{0, 0, 0, 5}.empty()); // zero width
        CHECK(cell_range{0, 0, 5, 0}.empty()); // zero height
        CHECK_FALSE((cell_range{0, 0, 1, 1}.empty()));
    }
}
