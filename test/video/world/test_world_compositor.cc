#include <doctest/doctest.h>

#include <neutrino/video/world/world_compositor.hh>
#include <neutrino/video/world/world_renderer.hh>
#include <neutrino/video/world/resource_cache.hh>
#include <neutrino/video/world/render_layer.hh>
#include <neutrino/video/world/texture_layer.hh>
#include <neutrino/video/world/camera.hh>
#include <neutrino/video/globals.hh>
#include <neutrino/world/world.hh>

#include "test_application.hh"
#include "video/world/test_cameras.hh"

#include <sdlpp/video/texture.hh>

using namespace neutrino;
using namespace neutrino::test;

namespace {
    // A 1x1 tile layer with a single empty cell: it needs no tileset, draws nothing,
    // but still fires the after-layer callback -- enough to exercise slot placement.
    world_tile_layer empty_layer(world_layer_id id) {
        world_tile_layer l;
        l.id = id;
        l.width = 1;
        l.height = 1;
        l.cells.resize(1);
        return l;
    }

    // Records each draw's view; a render_layer used as a probe.
    struct spy final : render_layer {
        int        draws{0};
        layer_view last{};
        void draw(const layer_view& view, sprite_batch&) override {
            ++draws;
            last = view;
        }
    };

    world two_layer_world() {
        world w;
        w.set_tile_size(16, 16);
        w.set_size(1, 1);
        w.add_layer(empty_layer(10));
        w.add_layer(empty_layer(20));
        return w;
    }
}

TEST_SUITE("neutrino::video world_compositor") {
    TEST_CASE("a slot draws once, after its layer, with the frame alpha") {
        neutrino::test::test_application app("compositor slot");
        resource_cache cache;

        world w = two_layer_world();
        world_renderer r(w, cache);
        world_compositor comp(r);

        spy s;
        comp.insert_after(10, s);
        comp.draw(cam_at({0.0f, 0.0f}, 1.0f), rect{0, 0, 64, 64}, 0.5f);

        CHECK(s.draws == 1);
        CHECK(s.last.alpha == doctest::Approx(0.5f));
    }

    TEST_CASE("the slot's visible rect is the plane cull, matching eval_layer_origin") {
        neutrino::test::test_application app("compositor visible");
        resource_cache cache;

        world w = two_layer_world();
        world_renderer r(w, cache);
        world_compositor comp(r);

        spy s; // default plane: parallax 1, no offset
        comp.insert_after(20, s);

        const camera cam = cam_at({100.0f, 50.0f}, 2.0f);
        const rect viewport{0, 0, 64, 64};
        comp.draw(cam, viewport, 1.0f);

        REQUIRE(s.draws == 1);
        camera view = cam;
        view.parallax_rest = r.parallax_rest(); // (0,0) by default
        const world_point o = details::eval_layer_origin(view, {1.0f, 1.0f}, {0.0f, 0.0f},
                                                          viewport.dimensions());
        CHECK(s.last.visible.x == doctest::Approx(o.x));
        CHECK(s.last.visible.y == doctest::Approx(o.y));
        CHECK(s.last.visible.w == doctest::Approx(64.0f / 2.0f)); // viewport / zoom
        CHECK(s.last.visible.h == doctest::Approx(64.0f / 2.0f));
    }

    TEST_CASE("slots share a slot in insertion order; parallax planes cull differently; no-match never draws") {
        neutrino::test::test_application app("compositor planes");
        resource_cache cache;

        world w = two_layer_world();
        world_renderer r(w, cache);
        world_compositor comp(r);

        spy near_plane;                                  // parallax 1 (follows the camera)
        spy far_plane;
        far_plane.plane.parallax_x = 0.5f;
        far_plane.plane.parallax_y = 0.5f;               // a background plane
        spy orphan;                                      // slotted after a non-existent layer

        comp.insert_after(10, near_plane);
        comp.insert_after(10, far_plane);
        comp.insert_after(999, orphan);

        comp.draw(cam_at({100.0f, 0.0f}, 1.0f), rect{0, 0, 64, 64}, 1.0f);

        CHECK(near_plane.draws == 1);
        CHECK(far_plane.draws == 1);
        CHECK(orphan.draws == 0);                        // id matches no layer
        // The half-speed plane sees a different world origin than the full-speed one.
        CHECK(near_plane.last.visible.x != doctest::Approx(far_plane.last.visible.x));
    }

    TEST_CASE("a bottom slot draws beneath every map layer, and layer_view carries the viewport") {
        neutrino::test::test_application app("compositor bottom");
        resource_cache cache;

        world w = two_layer_world();
        world_renderer r(w, cache);
        world_compositor comp(r);

        // A shared counter records global draw order: the bottom layer must run before
        // any after-layer slot (i.e. beneath the map).
        struct order_spy final : render_layer {
            int*       counter;
            int        order{-1};
            layer_view last{};
            void draw(const layer_view& view, sprite_batch&) override {
                order = (*counter)++;
                last = view;
            }
        };

        int counter = 0;
        order_spy background;
        background.counter = &counter;
        order_spy actor;
        actor.counter = &counter;

        comp.insert_bottom(background);
        comp.insert_after(20, actor);                    // after the last map layer

        const rect viewport{3, 4, 64, 48};
        comp.draw(cam_at({0.0f, 0.0f}, 1.0f), viewport, 1.0f);

        CHECK(background.order == 0);                     // drawn first, beneath the tiles
        CHECK(actor.order == 1);
        CHECK(background.last.viewport.x == 3);           // layer_view carries the viewport
        CHECK(background.last.viewport.w == 64);
        CHECK(background.last.viewport.h == 48);
    }

    TEST_CASE("a texture_layer blits a game-owned texture without error") {
        neutrino::test::test_application app("compositor texture layer");
        resource_cache cache;

        world w = two_layer_world();
        world_renderer r(w, cache);
        world_compositor comp(r);

        auto tex = sdlpp::texture::create(get_renderer(), sdlpp::pixel_format_enum::RGBA8888,
                                          sdlpp::texture_access::streaming, 16, 16);
        REQUIRE(tex.has_value());

        texture_layer background;
        background.texture = &*tex;
        comp.insert_bottom(background);

        // Must not throw: the live texture is blitted to fill the viewport beneath the map.
        const draw_stats stats = comp.draw(cam_at({0.0f, 0.0f}, 1.0f), rect{0, 0, 64, 64}, 1.0f);
        CHECK(stats.failed == 0);

        // A null texture is a no-op (does not throw).
        texture_layer empty;
        comp.insert_bottom(empty);
        comp.draw(cam_at({0.0f, 0.0f}, 1.0f), rect{0, 0, 64, 64}, 1.0f);
    }
}
