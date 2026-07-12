#include <doctest/doctest.h>

#include <neutrino/video/world/sprite_batch.hh>
#include <neutrino/video/world/camera.hh>
#include <neutrino/world/world.hh>

#include "test_application.hh"
#include "video/world/test_cameras.hh"

using namespace neutrino;
using namespace neutrino::test;

namespace {
}

TEST_SUITE("neutrino::video sprite_batch") {
    TEST_CASE("plan sorts by depth ascending and is stable on ties") {
        const camera cam = cam_at({0.0f, 0.0f}, 1.0f);
        const rect viewport{0, 0, 100, 100};
        const world_layer_header plane = plane_at();
        sprite_batch batch(cam, viewport, plane);

        // Distinct world x so each entry is identifiable by its screen x.
        batch.add(world_point{10.0f, 0.0f}, 100.0f, sprite_visual_ref{});
        batch.add(world_point{20.0f, 0.0f}, 50.0f, sprite_visual_ref{});
        batch.add(world_point{30.0f, 0.0f}, 150.0f, sprite_visual_ref{});
        // Two more at depth 50, added later: ties must keep insertion order.
        batch.add(world_point{40.0f, 0.0f}, 50.0f, sprite_visual_ref{});
        batch.add(world_point{50.0f, 0.0f}, 50.0f, sprite_visual_ref{});

        REQUIRE(batch.size() == 5);
        const auto plan = batch.plan();
        REQUIRE(plan.size() == 5);

        auto sx = [&](float wx) {
            return to_screen(cam, plane, viewport.dimensions(), world_point{wx, 0.0f}).x;
        };
        // Depth order: 50(20), 50(40), 50(50) [stable], then 100(10), 150(30).
        CHECK(plan[0].position.x == sx(20.0f));
        CHECK(plan[1].position.x == sx(40.0f));
        CHECK(plan[2].position.x == sx(50.0f));
        CHECK(plan[3].position.x == sx(10.0f));
        CHECK(plan[4].position.x == sx(30.0f));
    }

    TEST_CASE("plan applies the camera transform: viewport-offset position and zoom-folded scale") {
        const camera cam = cam_at({5.0f, 5.0f}, 3.0f);
        const rect viewport{7, 9, 64, 64};
        const world_layer_header plane = plane_at();
        sprite_batch batch(cam, viewport, plane);

        batch.add(world_point{12.0f, 34.0f}, 0.0f, sprite_visual_ref{}, sprite_draw_params{2.0f});
        const auto plan = batch.plan();
        REQUIRE(plan.size() == 1);

        const point sp = to_screen(cam, plane, viewport.dimensions(), world_point{12.0f, 34.0f});
        CHECK(plan[0].position.x == viewport.x + sp.x); // viewport top-left added
        CHECK(plan[0].position.y == viewport.y + sp.y);
        CHECK(plan[0].params.scale == doctest::Approx(2.0f * cam.zoom)); // caller scale * zoom
    }

    TEST_CASE("flush clears the queue and does not throw on invalid content") {
        neutrino::test::test_application app("sprite_batch flush");
        const camera cam = cam_at({0.0f, 0.0f}, 1.0f);
        const world_layer_header plane = plane_at();
        sprite_batch batch(cam, rect{0, 0, 64, 64}, plane);

        batch.add(world_point{0.0f, 0.0f}, 0.0f, sprite_visual_ref{}); // invalid visual -> no-op
        batch.add(world_point{1.0f, 1.0f}, 0.0f, sprite_state_id{});   // invalid state  -> no-op
        REQUIRE(batch.size() == 2);
        batch.flush();                                                  // must not throw
        CHECK(batch.empty());
    }
}
