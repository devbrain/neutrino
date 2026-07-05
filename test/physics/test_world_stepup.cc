//
// Created by igor on 28/06/2026.
//
// Step-up / ledge forgiveness (§19 #3). A mover walking into a small lip (a low step riser,
// a curb, a tile edge) should ride up over it instead of jamming against it. step_up lifts
// the actor up to max_step (capped by headroom), re-casts the horizontal step from the raised
// position, and -- if the path is now clear AND there is a tread to land on -- carries it
// forward over the lip and settles it onto the tread. A riser taller than max_step, no
// headroom to lift, or a ledge with no tread beyond the lip leaves the actor untouched.
// Policy (enable for characters, not crates) is the caller's; the helper is the query+lift.
//
#include <doctest/doctest.h>

#include <optional>
#include <variant>

#include <neutrino/physics/collide/world.hh>

using namespace neutrino::physics;

namespace {
    world step_world() {
        world_config cfg;
        cfg.bounds = aabb{vec{0, 0}, vec{64, 64}};
        cfg.up = {0, 1};
        cfg.skin = 0.01f;
        return world(cfg);
    }

    aabb box_of(const world& w, collider_id id) {
        return std::visit([](const auto& s) { return enclose(s); }, w.get_shape(id));
    }

    collider_id add_solid(world& w, entity_id_t eid, const aabb& box) {
        return w.add(eid, static_body{shape_t{box}, {}, {}});
    }

    collider_id add_slope(world& w, entity_id_t eid, const segment& s) {
        return w.add(eid, static_body{shape_t{s}, {}, {}});
    }

    constexpr float SKIN = 0.01f;

    // a 1x1 actor resting on a floor whose top is `top` -- placed at the skin-short gap the solver
    // leaves, the state step_up expects (a flush actor would read its own support as a toi-0 hit).
    collider_id add_actor_on(world& w, entity_id_t eid, float x, float top) {
        const float bottom = top + SKIN;
        return w.add(eid, kinematic_body{
                         moving_shape_t{aabb{vec{x, bottom}, vec{x + 1.0f, bottom + 1.0f}}}, {}, {}, vec{0, 0}});
    }
}

TEST_SUITE("world: step_up (§19 #3)") {
    TEST_CASE("climbs a short lip onto the higher floor and settles on the tread") {
        world w = step_world();
        add_solid(w, 1, aabb{vec{0, 0}, vec{32, 2}});  // lower floor, top y=2
        add_solid(w, 2, aabb{vec{6, 2}, vec{32, 4}});  // step block, top y=4, riser face at x=6 (height 2)
        const collider_id a = add_actor_on(w, 3, 4.0f, 2.0f); // on the lower floor, just left of the riser

        const auto tread = w.step_up(a, vec{2.0f, 0.0f}, 3.0f); // riser height 2 < max_step 3
        REQUIRE(tread.has_value());
        CHECK(box_of(w, a).min.y() == doctest::Approx(4.0f + SKIN).epsilon(0.01)); // on the step top
        CHECK(box_of(w, a).min.x() > 5.5f);                                        // carried forward over the lip
        CHECK(tread->normal.y() == doctest::Approx(1.0f).epsilon(0.01));
        CHECK(w.get_eid(tread->who) == 2u);                                        // settled on the step block
    }

    TEST_CASE("a riser taller than max_step stays a wall (no step, actor untouched)") {
        world w = step_world();
        add_solid(w, 1, aabb{vec{0, 0}, vec{32, 2}});
        add_solid(w, 2, aabb{vec{6, 2}, vec{32, 4}}); // riser height 2
        const collider_id a = add_actor_on(w, 3, 4.0f, 2.0f);
        const aabb b0 = box_of(w, a);

        const auto tread = w.step_up(a, vec{2.0f, 0.0f}, 1.0f); // max_step 1 < riser 2
        CHECK_FALSE(tread.has_value());
        CHECK(box_of(w, a).min.x() == doctest::Approx(b0.min.x())); // undone
        CHECK(box_of(w, a).min.y() == doctest::Approx(b0.min.y()));
    }

    TEST_CASE("no obstruction ahead -> nullopt, no move (leave it to the normal solver)") {
        world w = step_world();
        add_solid(w, 1, aabb{vec{0, 0}, vec{32, 2}}); // flat floor, clear path
        const collider_id a = add_actor_on(w, 3, 4.0f, 2.0f);
        const aabb b0 = box_of(w, a);

        const auto tread = w.step_up(a, vec{2.0f, 0.0f}, 3.0f);
        CHECK_FALSE(tread.has_value());
        CHECK(box_of(w, a).min.x() == doctest::Approx(b0.min.x()));
        CHECK(box_of(w, a).min.y() == doctest::Approx(b0.min.y()));
    }

    TEST_CASE("insufficient headroom (low ceiling) -> can't clear the riser -> nullopt, no move") {
        world w = step_world();
        add_solid(w, 1, aabb{vec{0, 0}, vec{32, 2}});  // lower floor
        add_solid(w, 2, aabb{vec{6, 2}, vec{32, 4}});  // riser height 2
        add_solid(w, 3, aabb{vec{0, 3.5f}, vec{6, 8}}); // ceiling 0.5 above the actor's head
        const collider_id a = add_actor_on(w, 4, 4.0f, 2.0f); // top ~y=3, only ~0.5 of headroom
        const aabb b0 = box_of(w, a);

        const auto tread = w.step_up(a, vec{2.0f, 0.0f}, 3.0f); // can lift < riser height -> stays blocked
        CHECK_FALSE(tread.has_value());
        CHECK(box_of(w, a).min.x() == doctest::Approx(b0.min.x()));
        CHECK(box_of(w, a).min.y() == doctest::Approx(b0.min.y()));
    }

    TEST_CASE("a non-walkable (steep) tread beyond the lip is not a clean step -> undo") {
        world w = step_world();
        add_solid(w, 1, aabb{vec{0, 0}, vec{5, 2}});             // floor, top y=2, x[0,5]
        add_slope(w, 2, segment{vec{5.2f, 2}, vec{5.7f, 3}});    // short STEEP slope (~63 deg) just past it
        const collider_id a = add_actor_on(w, 3, 4.0f, 2.0f);
        const aabb b0 = box_of(w, a);

        // forward clears after the lift and a tread DOES wait below -- but it is steeper than walkable,
        // so it must not be accepted as a step landing (regression: this used to "step up" onto a wall).
        const auto tread = w.step_up(a, vec{0.5f, 0.0f}, 1.5f);
        CHECK_FALSE(tread.has_value());
        CHECK(box_of(w, a).min.x() == doctest::Approx(b0.min.x())); // fully undone
        CHECK(box_of(w, a).min.y() == doctest::Approx(b0.min.y()));
    }

    TEST_CASE("the move starts from the actor's CURRENT position and advances by `step` (remaining delta)") {
        world w = step_world();
        add_solid(w, 1, aabb{vec{0, 0}, vec{32, 2}}); // lower floor, top y=2
        add_solid(w, 2, aabb{vec{5, 2}, vec{32, 4}}); // step block, riser face at x=5 (height 2)
        // the actor has already slid skin-short up to the riser (the post-move_and_slide state); the
        // caller passes the REMAINING delta (+1), not the original frame delta.
        const collider_id a = add_actor_on(w, 3, 5.0f - 1.0f - SKIN, 2.0f); // right edge at x=5-skin

        const auto tread = w.step_up(a, vec{1.0f, 0.0f}, 3.0f);
        REQUIRE(tread.has_value());
        CHECK(box_of(w, a).min.x() == doctest::Approx(5.0f - SKIN).epsilon(0.001)); // advanced by exactly +1
        CHECK(box_of(w, a).min.y() == doctest::Approx(4.0f + SKIN).epsilon(0.01));  // settled on the step top
        CHECK(w.get_eid(tread->who) == 2u);
    }

    TEST_CASE("a ledge beyond the lip (no tread to land on) is not stepped onto -> undo") {
        world w = step_world();
        add_solid(w, 1, aabb{vec{0, 0}, vec{8, 2}}); // lower floor ends at x=8
        add_solid(w, 2, aabb{vec{8, 2}, vec{9, 4}}); // a 1-wide bump (height 2); nothing beyond it
        const collider_id a = add_actor_on(w, 3, 6.0f, 2.0f);
        const aabb b0 = box_of(w, a);

        const auto tread = w.step_up(a, vec{4.0f, 0.0f}, 3.0f); // would clear the bump but land over a void
        CHECK_FALSE(tread.has_value());
        CHECK(box_of(w, a).min.x() == doctest::Approx(b0.min.x())); // fully undone
        CHECK(box_of(w, a).min.y() == doctest::Approx(b0.min.y()));
    }
}
