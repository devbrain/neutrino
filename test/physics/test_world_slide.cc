//
// Created by igor on 23/06/2026.
//
// Move-and-slide scenario tests, driven through world::run()'s movement pass (which builds the
// solid acceptor and calls move_and_slide per kinematic mover). Focus on the material/mode
// paths that the brute-force query tests don't cover:
//   * slope vs ice -- same scene, different friction => different retained tangential speed
//   * restitution  -- a bouncy surface reflects the normal component
//   * one-way      -- blocks from the solid side, passes from the other
//   * corners      -- floor + wall resolved together within max_slide_iter
//
#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include <neutrino/physics/collide/world.hh>

using namespace neutrino::physics;

namespace {
    const aabb BIG_REGION{{-100000, -100000}, {100000, 100000}};

    int count_collisions(const std::vector<world_event>& ev) {
        return static_cast<int>(std::count_if(ev.begin(), ev.end(),
            [](const world_event& e) { return e.kind == event_kind::COLLISION; }));
    }

    float speed(vec v) { return std::sqrt(v.x() * v.x() + v.y() * v.y()); }

    // A static surface with a chosen material.
    static_body surface(const shape_t& s, float friction = 0.0f, float restitution = 0.0f,
                        response_mode mode = response_mode::BLOCK, vec block_normal = {0, 1}) {
        static_body b;
        b.shape = s;
        b.material.friction = friction;
        b.material.restitution = restitution;
        b.material.response = mode;
        b.material.block_normal = block_normal;
        return b;
    }

    kinematic_body mover(const moving_shape_t& s, vec v) {
        kinematic_body b;
        b.shape = s;
        b.velocity = v;
        return b;
    }
} // namespace

TEST_SUITE("move_and_slide -- friction (slope vs ice)") {
    // A circle dropped onto a 45-degree slope segment. The into-slope component is removed; the
    // tangential (down-the-slope) component is what friction acts on. Same scene, only friction
    // changes -- that is the "slope and ice are one code path" property.
    auto drop_on_slope = [](float friction) {
        world w;
        // slope from (0,0) down to (10,-10): a -45 deg ramp.
        w.add(1, surface(segment{{0, 0}, {10, -10}}, friction));
        const collider_id k = w.add(2, mover(circle{{3, 0.5f}, 0.5f}, vec{0, -10}));
        (void) w.run(BIG_REGION, 1.0f);
        return w.get_velocity(k);
    };

    TEST_CASE("frictionless slope (ice) keeps the mover sliding") {
        const vec v = drop_on_slope(0.0f);
        CHECK(speed(v) > 3.0f);              // retains most of the tangential velocity
    }

    TEST_CASE("high friction slope nearly stops the mover") {
        const vec v = drop_on_slope(1.0f);
        CHECK(speed(v) < 0.5f);             // tangential fully damped
    }

    TEST_CASE("ice slides faster than a rough slope (same scene, only friction differs)") {
        CHECK(speed(drop_on_slope(0.0f)) > speed(drop_on_slope(0.8f)));
    }
}

TEST_SUITE("move_and_slide -- restitution (bounce)") {
    TEST_CASE("a bouncy floor reflects the downward velocity upward") {
        world w;
        w.add(1, surface(aabb{{-10, -1}, {10, 0}}, /*friction*/0.0f, /*restitution*/1.0f));
        const collider_id k = w.add(2, mover(aabb{{0, 2}, {1, 3}}, vec{0, -10}));
        (void) w.run(BIG_REGION, 1.0f);
        CHECK(w.get_velocity(k).y() > 5.0f);   // restitution 1 -> reflected up (was -10)
    }

    TEST_CASE("a non-bouncy floor absorbs the normal velocity") {
        world w;
        w.add(1, surface(aabb{{-10, -1}, {10, 0}}, 0.0f, 0.0f));
        const collider_id k = w.add(2, mover(aabb{{0, 2}, {1, 3}}, vec{0, -10}));
        (void) w.run(BIG_REGION, 1.0f);
        CHECK(std::fabs(w.get_velocity(k).y()) < 0.01f); // restitution 0 -> stops
    }
}

TEST_SUITE("move_and_slide -- one-way platforms") {
    TEST_CASE("falling onto a one-way platform from above is blocked") {
        world w;
        w.add(1, surface(aabb{{-10, -1}, {10, 0}}, 0.0f, 0.0f, response_mode::ONE_WAY, vec{0, 1}));
        const collider_id k = w.add(2, mover(aabb{{0, 2}, {1, 3}}, vec{0, -10})); // falling
        const auto& ev = w.run(BIG_REGION, 1.0f);
        CHECK(count_collisions(ev) == 1);
        CHECK(std::fabs(w.get_velocity(k).y()) < 0.01f);  // landed
    }

    TEST_CASE("rising through a one-way platform from below passes") {
        world w;
        w.add(1, surface(aabb{{-10, 0}, {10, 1}}, 0.0f, 0.0f, response_mode::ONE_WAY, vec{0, 1}));
        const collider_id k = w.add(2, mover(aabb{{0, -3}, {1, -2}}, vec{0, 10})); // rising
        const auto& ev = w.run(BIG_REGION, 1.0f);
        CHECK(count_collisions(ev) == 0);                 // passes through
        CHECK(std::fabs(w.get_velocity(k).y() - 10.0f) < 0.01f);
    }

    TEST_CASE("hitting the SIDE of a one-way block (while moving down) passes through") {
        // The one-way decision uses the CONTACT NORMAL, not velocity: a mover entering the left
        // face is not crossing the blocked (up) face, so it must pass even though it moves down.
        world w;
        w.add(1, surface(aabb{{5, 0}, {6, 4}}, 0.0f, 0.0f, response_mode::ONE_WAY, vec{0, 1}));
        const collider_id k = w.add(2, mover(aabb{{0, 1}, {1, 2}}, vec{10, -2})); // into left face, slightly down
        const auto& ev = w.run(BIG_REGION, 1.0f);
        CHECK(count_collisions(ev) == 0);                 // side of a one-way -> not blocked
        CHECK(std::fabs(w.get_velocity(k).x() - 10.0f) < 0.01f);
    }
}

TEST_SUITE("move_and_slide -- corners") {
    TEST_CASE("floor + wall corner: both axes resolved in one move") {
        world w;
        w.add(1, surface(aabb{{-10, -1}, {10, 0}}));   // floor, top y=0
        w.add(2, surface(aabb{{5, -10}, {6, 10}}));    // wall, left x=5
        // 1x1 box starting at (0,1) moving down-right into the corner
        const collider_id k = w.add(3, mover(aabb{{0, 1}, {1, 2}}, vec{10, -10}));

        const auto& ev = w.run(BIG_REGION, 1.0f);
        CHECK(count_collisions(ev) >= 2);                 // hit both surfaces
        const vec v = w.get_velocity(k);
        CHECK(std::fabs(v.x()) < 0.01f);                  // wall killed horizontal
        CHECK(std::fabs(v.y()) < 0.01f);                  // floor killed vertical
    }

    TEST_CASE("sliding into a single wall keeps the tangential axis") {
        world w;
        w.add(1, surface(aabb{{5, -10}, {6, 10}}));       // wall
        const collider_id k = w.add(2, mover(aabb{{0, 0}, {1, 1}}, vec{10, 4}));
        (void) w.run(BIG_REGION, 1.0f);
        const vec v = w.get_velocity(k);
        CHECK(std::fabs(v.x()) < 0.01f);                  // blocked
        CHECK(std::fabs(v.y() - 4.0f) < 0.01f);          // free axis preserved
    }
}
