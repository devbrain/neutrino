//
// Created by igor on 28/06/2026.
//
// Footing / edge sensors (§19 #4). ground_support probes straight down at three points of the
// actor's footprint -- left edge, centre, right edge -- and reports the solid ground under each
// (the left/right pair is exactly Sonic's twin floor sensors A/B). It is a pure QUERY the game
// reads to drive teeter/balance, edge-stop, coyote-time and ledge-grab -- a state the game
// animates, not a solver behaviour. Probes are solid-only (a sensor underfoot is not support)
// and self-excluding; the actor is never moved.
//
#include <doctest/doctest.h>

#include <optional>
#include <variant>

#include <neutrino/physics/collide/world.hh>

using namespace neutrino::physics;


namespace {
    world foot_world() {
        world_config cfg;
        cfg.bounds = aabb{vec{0, 0}, vec{64, 64}};
        cfg.up = {0, 1};
        cfg.skin = 0.01f;
        return world(cfg);
    }

    aabb box_of(const world& w, collider_id id) {
        return std::visit([](const auto& s) { return enclose(s); }, w.get_shape(id));
    }

    collider_id add_solid(world& w, entity_id_t eid, const aabb& box, material_props m = {}) {
        return w.add(eid, static_body{shape_t{box}, m, {}});
    }

    collider_id add_slope(world& w, entity_id_t eid, const segment& s) {
        return w.add(eid, static_body{shape_t{s}, {}, {}});
    }

    // a footprint of the given x-extent whose bottom rests skin-short above `top` (the post-solver state)
    collider_id add_actor(world& w, entity_id_t eid, float x0, float x1, float top) {
        const float b = top + 0.01f;
        return w.add(eid, kinematic_body{moving_shape_t{aabb{vec{x0, b}, vec{x1, b + 1.0f}}}, {}, {}, vec{0, 0}});
    }

    // a footprint floating at an explicit bottom y (no skin offset)
    collider_id add_actor_at(world& w, entity_id_t eid, float x0, float x1, float bottom) {
        return w.add(eid, kinematic_body{moving_shape_t{aabb{vec{x0, bottom}, vec{x1, bottom + 1.0f}}}, {}, {}, vec{0, 0}});
    }
}

TEST_SUITE("world: ground_support / footing (§19 #4)") {
    TEST_CASE("firmly on a wide floor: all three feet supported") {
        world w = foot_world();
        add_solid(w, 1, aabb{vec{0, 0}, vec{32, 2}});
        const collider_id a = add_actor(w, 2, 4.0f, 5.0f, 2.0f);

        const footing f = w.ground_support(a, 1.0f);
        CHECK(f.left.has_value());
        CHECK(f.centre.has_value());
        CHECK(f.right.has_value());
        CHECK(f.grounded());
        CHECK(f.fully_supported());
        CHECK_FALSE(f.at_ledge());
        CHECK(f.centre->normal.y() == doctest::Approx(1.0f).epsilon(0.01));
        CHECK(box_of(w, a).min.y() == doctest::Approx(2.0f + 0.01f)); // pure query: actor not moved
    }

    TEST_CASE("at a right-hand ledge: left+centre supported, right foot hangs") {
        world w = foot_world();
        add_solid(w, 1, aabb{vec{0, 0}, vec{5, 2}}); // floor ends at x=5
        const collider_id a = add_actor(w, 2, 4.2f, 5.2f, 2.0f); // right foot at x=5.2, past the edge

        const footing f = w.ground_support(a, 1.0f);
        CHECK(f.left.has_value());
        CHECK(f.centre.has_value());
        CHECK_FALSE(f.right.has_value());
        CHECK(f.at_ledge());
        CHECK(f.ledge_right());
        CHECK_FALSE(f.ledge_left());
    }

    TEST_CASE("at a left-hand ledge: right+centre supported, left foot hangs") {
        world w = foot_world();
        add_solid(w, 1, aabb{vec{5, 0}, vec{32, 2}}); // floor starts at x=5
        const collider_id a = add_actor(w, 2, 4.8f, 5.8f, 2.0f); // left foot at x=4.8, before the edge

        const footing f = w.ground_support(a, 1.0f);
        CHECK_FALSE(f.left.has_value());
        CHECK(f.centre.has_value());
        CHECK(f.right.has_value());
        CHECK(f.at_ledge());
        CHECK(f.ledge_left());
        CHECK_FALSE(f.ledge_right());
    }

    TEST_CASE("balanced on a narrow pillar: only the centre is supported (both sides hang)") {
        world w = foot_world();
        add_solid(w, 1, aabb{vec{4.3f, 0}, vec{4.7f, 2}}); // 0.4-wide pillar under the centre
        const collider_id a = add_actor(w, 2, 4.0f, 5.0f, 2.0f); // feet at 4.0 / 4.5 / 5.0

        const footing f = w.ground_support(a, 1.0f);
        CHECK_FALSE(f.left.has_value());
        CHECK(f.centre.has_value());
        CHECK_FALSE(f.right.has_value());
        CHECK(f.grounded());
        CHECK(f.at_ledge());
        CHECK_FALSE(f.fully_supported());
        CHECK(f.ledge_left());
        CHECK(f.ledge_right());
    }

    TEST_CASE("airborne: nothing solid within reach -> no support, not grounded") {
        world w = foot_world();
        add_solid(w, 1, aabb{vec{0, 0}, vec{32, 2}});
        const collider_id a = add_actor_at(w, 2, 4.0f, 5.0f, 10.0f); // 8 above the floor

        const footing f = w.ground_support(a, 2.0f); // reach 2 < 8
        CHECK_FALSE(f.grounded());
        CHECK_FALSE(f.at_ledge());
        CHECK_FALSE(f.ledge_left());
        CHECK_FALSE(f.ledge_right());
    }

    TEST_CASE("a sensor underfoot is not support") {
        world w = foot_world();
        material_props sensor;
        sensor.response = response_mode::SENSOR;
        add_solid(w, 1, aabb{vec{0, 0}, vec{32, 2}}, sensor);
        const collider_id a = add_actor(w, 2, 4.0f, 5.0f, 2.0f);

        const footing f = w.ground_support(a, 1.0f);
        CHECK_FALSE(f.grounded()); // a sensor is detected by triggers, not stood upon
    }

    TEST_CASE("a steep slope underfoot is not support (matches grounded/snap/step semantics)") {
        world w = foot_world();
        add_slope(w, 1, segment{vec{2, 0}, vec{6, 8}}); // ~63 deg, spans under the whole footprint
        const collider_id a = add_actor_at(w, 2, 4.0f, 5.0f, 7.0f); // feet above the steep face

        const footing f = w.ground_support(a, 4.0f); // probes reach the slope, but it is too steep
        CHECK_FALSE(f.grounded()); // a steep face is not standable ground
        CHECK_FALSE(f.at_ledge());
    }

    TEST_CASE("a gentle (walkable) slope underfoot IS support") {
        world w = foot_world();
        add_slope(w, 1, segment{vec{0, 0}, vec{8, 4}}); // slope 0.5 (~27 deg), walkable
        const collider_id a = add_actor_at(w, 2, 3.0f, 4.0f, 3.0f); // feet above the gentle slope

        const footing f = w.ground_support(a, 2.0f);
        CHECK(f.grounded()); // a gentle slope is standable, unlike the steep one above
    }

    TEST_CASE("a ONE_WAY platform supports from above") {
        world w = foot_world();
        material_props oneway;
        oneway.response = response_mode::ONE_WAY;
        oneway.block_normal = {0, 1};
        add_solid(w, 1, aabb{vec{0, 0}, vec{32, 2}}, oneway);
        const collider_id a = add_actor(w, 2, 4.0f, 5.0f, 2.0f);

        const footing f = w.ground_support(a, 1.0f);
        CHECK(f.fully_supported());
    }
}
