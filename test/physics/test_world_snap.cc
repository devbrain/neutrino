//
// Created by igor on 28/06/2026.
//
// Ground snapping (§19 #2). snap_to_ground keeps a grounded actor glued to a floor that
// receded beneath it this frame -- so it hugs a downhill slope / staircase instead of
// launching off each lip. It sweeps the actor's own shape straight down (-up) by at most
// max_drop and, if it lands on a WALKABLE surface within reach, translates the actor down
// onto it (leaving the standard skin gap). No walkable ground within reach -> nullopt (a
// real cliff / >45 deg face; the actor should fall). Policy (grounded-last-frame, not
// rising) is the caller's; the helper is the query+snap.
//
#include <doctest/doctest.h>

#include <optional>
#include <variant>

#include <neutrino/physics/collide/world.hh>

using namespace neutrino::physics;
namespace {
    // pure-dynamic world (no grid): floors/slopes are static_body residents in the BVH.
    world snap_world() {
        world_config cfg;
        cfg.bounds = aabb{vec{0, 0}, vec{64, 64}};
        cfg.up = {0, 1};
        cfg.skin = 0.01f;
        return world(cfg);
    }

    aabb box_of(const world& w, collider_id id) {
        return std::visit([](const auto& s) { return enclose(s); }, w.get_shape(id));
    }

    collider_id add_floor(world& w, entity_id_t eid, const aabb& box) {
        return w.add(eid, static_body{shape_t{box}, {}, {}});
    }

    collider_id add_slope(world& w, entity_id_t eid, const segment& s) {
        return w.add(eid, static_body{shape_t{s}, {}, {}});
    }

    // a 1x1 actor whose bottom sits at `bottom`
    collider_id add_actor(world& w, entity_id_t eid, float x, float bottom) {
        return w.add(eid, kinematic_body{
                         moving_shape_t{aabb{vec{x, bottom}, vec{x + 1.0f, bottom + 1.0f}}}, {}, {}, vec{0, 0}});
    }

    constexpr float SKIN = 0.01f;
}

TEST_SUITE("world: snap_to_ground (§19 #2)") {
    TEST_CASE("snaps a grounded actor down onto a flat floor within reach") {
        world w = snap_world();
        add_floor(w, 1, aabb{vec{0, 0}, vec{32, 2}}); // top at y=2
        const collider_id a = add_actor(w, 2, 5.0f, 3.0f); // bottom y=3, a 1-unit gap below

        const auto hit = w.snap_to_ground(a, 2.0f);
        REQUIRE(hit.has_value());
        CHECK(box_of(w, a).min.y() == doctest::Approx(2.0f + SKIN).epsilon(0.01)); // landed skin-short
        CHECK(hit->normal.y() == doctest::Approx(1.0f).epsilon(0.01));             // floor faces up
        CHECK(w.get_eid(hit->who) == 1u);
    }

    TEST_CASE("a cliff deeper than max_drop is not snapped (actor falls)") {
        world w = snap_world();
        add_floor(w, 1, aabb{vec{0, 0}, vec{32, 2}});
        const collider_id a = add_actor(w, 2, 5.0f, 10.0f); // bottom y=10, an 8-unit gap
        const float y0 = box_of(w, a).min.y();

        const auto hit = w.snap_to_ground(a, 2.0f); // floor is 8 below, reach is 2
        CHECK_FALSE(hit.has_value());
        CHECK(box_of(w, a).min.y() == doctest::Approx(y0)); // unmoved
    }

    TEST_CASE("an actor already on the floor (within the skin gap) returns the contact, no move") {
        world w = snap_world();
        add_floor(w, 1, aabb{vec{0, 0}, vec{32, 2}});
        const collider_id a = add_actor(w, 2, 5.0f, 2.0f + SKIN / 2.0f); // already inside the skin gap
        const float y0 = box_of(w, a).min.y();

        const auto hit = w.snap_to_ground(a, 2.0f);
        REQUIRE(hit.has_value());
        CHECK(box_of(w, a).min.y() == doctest::Approx(y0)); // dist clamps to 0 -> no move
    }

    TEST_CASE("no ground below -> nullopt, actor unmoved (and does not snap onto itself)") {
        world w = snap_world();
        const collider_id a = add_actor(w, 2, 5.0f, 5.0f); // floating; nothing beneath
        const float y0 = box_of(w, a).min.y();

        const auto hit = w.snap_to_ground(a, 4.0f);
        CHECK_FALSE(hit.has_value());
        CHECK(box_of(w, a).min.y() == doctest::Approx(y0));
    }

    TEST_CASE("snaps onto a gentle (walkable) slope") {
        world w = snap_world();
        add_slope(w, 1, segment{vec{0, 0}, vec{8, 4}}); // slope 0.5 (~27 deg): normal.y ~ 0.894
        const collider_id a = add_actor(w, 2, 3.0f, 5.0f);

        const auto hit = w.snap_to_ground(a, 4.0f);
        REQUIRE(hit.has_value());
        CHECK(hit->normal.y() > 0.707f); // walkable
    }

    TEST_CASE("a steep (>45 deg) face below is not walkable -> not snapped") {
        world w = snap_world();
        add_slope(w, 1, segment{vec{2, 0}, vec{4, 6}}); // slope 3 (~71 deg): normal.y ~ 0.316
        const collider_id a = add_actor(w, 2, 2.5f, 7.0f);
        const float y0 = box_of(w, a).min.y();

        const auto hit = w.snap_to_ground(a, 4.0f);
        CHECK_FALSE(hit.has_value()); // steeper than the walkable limit -> fall
        CHECK(box_of(w, a).min.y() == doctest::Approx(y0));
    }

    TEST_CASE("snaps onto a ONE_WAY platform from above") {
        world w = snap_world();
        material_props oneway;
        oneway.response = response_mode::ONE_WAY;
        oneway.block_normal = {0, 1};
        w.add(1, static_body{shape_t{aabb{vec{0, 0}, vec{32, 2}}}, oneway, {}});
        const collider_id a = add_actor(w, 2, 5.0f, 3.0f);

        const auto hit = w.snap_to_ground(a, 2.0f);
        REQUIRE(hit.has_value()); // landing on top crosses the blocked face -> solid
        CHECK(box_of(w, a).min.y() == doctest::Approx(2.0f + SKIN).epsilon(0.01));
    }

    TEST_CASE("walking off a step-down: the gap re-grounds onto the lower floor") {
        world w = snap_world();
        add_floor(w, 1, aabb{vec{0, 0}, vec{10, 4}});  // high floor, top y=4 (x 0..10)
        add_floor(w, 2, aabb{vec{10, 0}, vec{32, 2}}); // step down, top y=2 (x 10..32)
        // the actor stepped just past the lip; its bottom is still at the high top (y=4),
        // now floating over the lower floor with a 2-unit gap.
        const collider_id a = add_actor(w, 3, 11.0f, 4.0f);

        const auto hit = w.snap_to_ground(a, 3.0f); // step height 2 < reach 3
        REQUIRE(hit.has_value());
        CHECK(box_of(w, a).min.y() == doctest::Approx(2.0f + SKIN).epsilon(0.01));
        CHECK(w.get_eid(hit->who) == 2u); // re-grounded on the lower floor
    }
}
