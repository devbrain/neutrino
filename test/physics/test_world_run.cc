//
// Created by igor on 23/06/2026.
//
// Tests for world::run() -- the per-frame driver in <neutrino/physics/collide/world.hh>.
// run() detects and returns events (the game reacts after it returns); it never mutates the
// world on the game's behalf. Three passes feed one reused event buffer:
//   * movement -- kinematic move-and-slide vs solids        -> COLLISION
//   * bullets  -- swept cast + integrate (region-culled)    -> BULLET_HIT / BULLET_EXPIRED
//   * triggers -- sensor-overlap begin/end edge diff        -> TRIGGER_BEGIN / TRIGGER_END
// response_mode is the classifier: solids are resolved (COLLISION), sensors are reported as
// edges (TRIGGER_*); there is no generic per-frame "touch" event.
//
#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include <neutrino/physics/collide/world.hh>

using namespace neutrino::physics;

namespace {
    const aabb BIG_REGION{{-100000, -100000}, {100000, 100000}}; // "everything is active"

    int count_kind(const std::vector<world_event>& ev, event_kind k) {
        return static_cast<int>(std::count_if(ev.begin(), ev.end(),
                                              [k](const world_event& e) { return e.kind == k; }));
    }

    const world_event* find_kind(const std::vector<world_event>& ev, event_kind k) {
        const auto it = std::find_if(ev.begin(), ev.end(),
                                     [k](const world_event& e) { return e.kind == k; });
        return it == ev.end() ? nullptr : &*it;
    }

    static_body mk_static(const shape_t& s, response_mode mode = response_mode::BLOCK) {
        static_body b;
        b.shape = s;
        b.material.response = mode;
        return b;
    }

    kinematic_body mk_kine(const moving_shape_t& s, vec v) {
        kinematic_body b;
        b.shape = s;
        b.velocity = v;
        return b;
    }

    float aabb_min_x(const shape_t& s) { return std::get<aabb>(s).min.x(); }
    float aabb_min_y(const shape_t& s) { return std::get<aabb>(s).min.y(); }
} // namespace

TEST_SUITE("world::run -- movement pass (COLLISION)") {
    TEST_CASE("kinematic lands on a floor: COLLISION, grounded stop") {
        world w;
        const collider_id floor = w.add(1, mk_static(aabb{{-10, -1}, {10, 0}})); // top at y=0
        const collider_id k = w.add(2, mk_kine(aabb{{0, 2}, {1, 3}}, vec{0, -10}));

        const auto& ev = w.run(BIG_REGION, 1.0f);
        REQUIRE(count_kind(ev, event_kind::COLLISION) == 1);
        const world_event* c = find_kind(ev, event_kind::COLLISION);
        CHECK(c->mover.value == k.value);
        CHECK(c->target.value == floor.value);
        CHECK(std::fabs(w.get_velocity(k).y()) < 0.01f);          // vertical killed
        CHECK(aabb_min_y(w.get_shape(k)) >= -0.05f);              // resting near the floor top
        CHECK(aabb_min_y(w.get_shape(k)) < 0.2f);
    }

    TEST_CASE("free fall with no obstacle: full move, no event") {
        world w;
        const collider_id k = w.add(1, mk_kine(aabb{{0, 0}, {1, 1}}, vec{0, -7}));
        const auto& ev = w.run(BIG_REGION, 1.0f);
        CHECK(ev.empty());
        CHECK(std::fabs(aabb_min_y(w.get_shape(k)) - (-7.0f)) < 1e-3f);
    }

    TEST_CASE("zero-velocity kinematic does nothing") {
        world w;
        const collider_id k = w.add(1, mk_kine(aabb{{0, 0}, {1, 1}}, vec{0, 0}));
        CHECK(w.run(BIG_REGION, 1.0f).empty());
        CHECK(std::fabs(aabb_min_x(w.get_shape(k))) < 1e-4f);
    }

    TEST_CASE("off-region mover stays dormant") {
        world w;
        const aabb region{{-10, -10}, {10, 10}};
        const collider_id k = w.add(1, mk_kine(aabb{{5000, 0}, {5001, 1}}, vec{10, 0}));
        CHECK(w.run(region, 1.0f).empty());
        CHECK(std::fabs(aabb_min_x(w.get_shape(k)) - 5000.0f) < 1e-4f); // did not move
    }

    TEST_CASE("slide along a wall: horizontal blocked, vertical preserved") {
        world w;
        w.add(1, mk_static(aabb{{5, -10}, {6, 10}}));                       // vertical wall
        const collider_id k = w.add(2, mk_kine(aabb{{0, 0}, {1, 1}}, vec{10, 3}));
        const auto& ev = w.run(BIG_REGION, 1.0f);
        CHECK(count_kind(ev, event_kind::COLLISION) >= 1);
        CHECK(std::fabs(w.get_velocity(k).x()) < 0.01f);                    // into-wall killed
        CHECK(std::fabs(w.get_velocity(k).y() - 3.0f) < 0.01f);            // tangential kept (frictionless)
    }

    // Regression: a kinematic mover resting flush against a wall (landed there by a prior
    // move-and-slide, so it sits `skin` short) must be free to move AWAY from that wall the next
    // frame. The KE paddle surfaced this: driven into a pillar then pulled back, it intermittently
    // stayed pinned -- the away-move was being cancelled as if still into-surface.
    TEST_CASE("kinematic resting on a wall moves away freely (no toi-0 pin)") {
        world w;
        w.add(1, mk_static(aabb{{304, -10}, {320, 10}}));                   // right pillar, left face x=304

        SUBCASE("landed against the wall, then pulled back") {
            const collider_id k = w.add(2, mk_kine(aabb{{200, 0}, {238, 8}}, vec{100, 0})); // 38 wide, into wall
            (void) w.run(BIG_REGION, 1.0f);                                 // lands ~265.99 (304 - skin - 38)
            const float rest = aabb_min_x(w.get_shape(k));
            REQUIRE(rest > 265.0f);                                         // actually reached the wall
            REQUIRE(rest < 266.0f);

            w.set_velocity(k, vec{-100, 0});                               // pull left, away from the wall
            (void) w.run(BIG_REGION, 1.0f);
            CHECK(aabb_min_x(w.get_shape(k)) < rest - 90.0f);              // moved ~100 left, not pinned
        }

        SUBCASE("exactly flush (zero gap), then pulled back") {
            const collider_id k = w.add(2, mk_kine(aabb{{266, 0}, {304, 8}}, vec{-100, 0})); // right edge ON x=304
            (void) w.run(BIG_REGION, 1.0f);
            CHECK(aabb_min_x(w.get_shape(k)) < 266.0f - 90.0f);           // moved left, not pinned at the face
        }
    }
}

TEST_SUITE("world::run -- positional intent (set_target / move_to)") {
    // set_target is the position-driven mirror of set_velocity: the body carries a target and the
    // movement pass derives the step velocity, CLAMPED to geometry. It replaces the KE paddle's
    // (target-current)/dt hack -- the source of the intermittent wall-pin freeze.

    TEST_CASE("set_target with a clear path reaches the target") {
        world w;
        const collider_id k = w.add(1, mk_kine(aabb{{0, 0}, {1, 1}}, vec{0, 0})); // centre (0.5,0.5)
        w.set_target(k, vec{5.5f, 0.5f});                                         // want centre at +5 in x
        CHECK(w.has_target(k));
        (void) w.run(BIG_REGION, 1.0f);
        CHECK(aabb_min_x(w.get_shape(k)) == doctest::Approx(5.0f).epsilon(1e-3)); // centre at 5.5
        CHECK(std::fabs(aabb_min_y(w.get_shape(k))) < 1e-3f);                     // y untouched
    }

    TEST_CASE("set_target settles: reaching the target leaves ~zero velocity") {
        world w;
        const collider_id k = w.add(1, mk_kine(aabb{{0, 0}, {1, 1}}, vec{0, 0}));
        w.set_target(k, vec{5.5f, 0.5f});
        (void) w.run(BIG_REGION, 1.0f);                                          // arrives
        (void) w.run(BIG_REGION, 1.0f);                                          // already there
        CHECK(std::fabs(w.get_velocity(k).x()) < 1e-3f);                         // no residual drive
        CHECK(aabb_min_x(w.get_shape(k)) == doctest::Approx(5.0f).epsilon(1e-3)); // stayed put
    }

    // The freeze scenario: a target on the far side of a wall must clamp AT the wall, and
    // get_velocity must report the EFFECTIVE velocity -- the ground actually covered over dt.
    // Three distinct values are in play and only one is right:
    //   ~99.5 -> the fabricated (target-current)/dt request  (the freeze bug)
    //   ~0    -> move-and-slide's surface-PROJECTED velocity (says "didn't move" after moving 5)
    //   ~5    -> the applied displacement / dt               (correct: what actually happened)
    TEST_CASE("set_target past a wall clamps at the wall and reports the distance covered") {
        world w;
        w.add(1, mk_static(aabb{{6, -10}, {7, 10}}));                            // wall, left face x=6
        const collider_id k = w.add(2, mk_kine(aabb{{0, 0}, {1, 1}}, vec{0, 0})); // 1 wide, centre 0.5
        w.set_target(k, vec{100.0f, 0.5f});                                      // unreachable target
        (void) w.run(BIG_REGION, 1.0f);

        const float max_x = std::get<aabb>(w.get_shape(k)).max.x();
        CHECK(max_x <= 6.0f + 1e-3f);                                            // stopped at the wall
        CHECK(max_x > 5.5f);                                                     // actually advanced to it

        const float travelled = (max_x - 0.5f) - 0.5f;                           // centre moved this far
        const float vx = w.get_velocity(k).x();
        CHECK(vx == doctest::Approx(travelled).epsilon(1e-2));                   // applied / dt (dt == 1)
        CHECK(vx > 4.0f);                                                        // NOT the projected ~0
        CHECK(vx < 10.0f);                                                       // NOT the requested ~99.5
    }

    // A body pinned flush against the wall covers no ground, so there the effective velocity
    // really is ~0 -- the distinction the projected-velocity reading could not express.
    TEST_CASE("set_target into a wall it is already touching reports ~0") {
        world w;
        w.add(1, mk_static(aabb{{6, -10}, {7, 10}}));
        const collider_id k = w.add(2, mk_kine(aabb{{0, 0}, {1, 1}}, vec{0, 0}));
        w.set_target(k, vec{100.0f, 0.5f});
        (void) w.run(BIG_REGION, 1.0f); // frame 1: travels to the wall
        REQUIRE(w.get_velocity(k).x() > 4.0f);
        (void) w.run(BIG_REGION, 1.0f); // frame 2: already there, nowhere left to go
        CHECK(std::fabs(w.get_velocity(k).x()) < 0.1f);
    }

    // An off-region body is dormant -- it does not move, so it must not claim to. Without
    // settling the velocity on the early-out it would report the full requested rate.
    TEST_CASE("an off-region targeted body reports no motion") {
        world w;
        const aabb region{{-10, -10}, {10, 10}};
        const collider_id k = w.add(1, mk_kine(aabb{{5000, 0}, {5001, 1}}, vec{0, 0}));
        w.set_target(k, vec{9000.0f, 0.5f});         // a huge requested rate, culled before it applies
        (void) w.run(region, 1.0f);
        CHECK(std::fabs(aabb_min_x(w.get_shape(k)) - 5000.0f) < 1e-3f); // dormant: did not move
        CHECK(std::fabs(w.get_velocity(k).x()) < 1e-3f);               // and does not claim to
    }

    TEST_CASE("clear_target reverts to velocity control") {
        world w;
        const collider_id k = w.add(1, mk_kine(aabb{{0, 0}, {1, 1}}, vec{0, 0}));
        w.set_target(k, vec{5.5f, 0.5f});
        w.clear_target(k);
        CHECK_FALSE(w.has_target(k));
        w.set_velocity(k, vec{-3, 0});
        (void) w.run(BIG_REGION, 1.0f);
        CHECK(aabb_min_x(w.get_shape(k)) == doctest::Approx(-3.0f).epsilon(1e-3)); // moved by velocity, not target
    }

    // move_result::velocity is the APPLIED displacement over dt -- "how fast did it actually go".
    // Reporting the slide's outgoing velocity instead would say ~0 on the blocked axis for a move
    // that covered real distance before touching the wall.
    TEST_CASE("move_result::velocity reflects distance actually travelled, not the slide response") {
        world w;
        w.add(1, mk_static(aabb{{6, -10}, {7, 10}}));                            // wall, left face x=6
        const collider_id k = w.add(2, mk_kine(aabb{{0, 0}, {1, 1}}, vec{0, 0})); // centre (0.5,0.5)

        SUBCASE("blocked partway: velocity is the travelled distance / dt") {
            // Centre 0.5 -> the box's max.x reaches the wall at centre 5.5, i.e. ~5 units of travel.
            const move_result r = w.move_to(k, vec{100.0f, 0.5f}, units::duration{0.5f});
            REQUIRE(r.blocked());
            const float travelled = r.position.x() - 0.5f;
            REQUIRE(travelled > 4.0f);                                           // really did move
            CHECK(r.velocity.x() == doctest::Approx(travelled / 0.5f).epsilon(1e-2));
            CHECK(r.velocity.x() > 8.0f);                                        // NOT ~0
        }

        SUBCASE("clear path: velocity is the full requested rate") {
            world w2;
            const collider_id k2 = w2.add(1, mk_kine(aabb{{0, 0}, {1, 1}}, vec{0, 0}));
            const move_result r = w2.move_by(k2, units::displacement{vec{4.0f, 0.0f}},
                                             units::duration{2.0f});
            CHECK_FALSE(r.blocked());
            CHECK(r.velocity.x() == doctest::Approx(2.0f).epsilon(1e-3));        // 4 units / 2 s
            CHECK(std::fabs(r.remaining.x()) < 1e-3f);
        }
    }

    // An immediate move must actually be immediate: zeroing the velocity is not enough to leave the
    // body at rest while a standing set_target survives, because the next run() would recompute a
    // velocity toward that stale target and walk the body off where the move just put it.
    TEST_CASE("an immediate move ends target mode instead of resuming the old target") {
        world w;
        const collider_id k = w.add(1, mk_kine(aabb{{0, 0}, {1, 1}}, vec{0, 0})); // centre 0.5

        w.set_target(k, vec{50.0f, 0.5f});       // a standing intent, far to the right
        REQUIRE(w.has_target(k));

        const move_result r = w.move_to(k, vec{-4.5f, 0.5f}, units::duration{1.0f}); // jump LEFT
        REQUIRE_FALSE(r.blocked());
        REQUIRE(r.position.x() == doctest::Approx(-4.5f).epsilon(1e-3));

        SUBCASE("the target is gone") {
            CHECK_FALSE(w.has_target(k));
        }

        SUBCASE("run() does not drag the body back toward the old target") {
            (void) w.run(BIG_REGION, 1.0f);
            CHECK(aabb_min_x(w.get_shape(k)) == doctest::Approx(-5.0f).epsilon(1e-3)); // stayed put
            CHECK(std::fabs(w.get_velocity(k).x()) < 1e-3f);
        }

        SUBCASE("set_target re-arms it afterwards") {
            w.set_target(k, vec{2.5f, 0.5f});
            (void) w.run(BIG_REGION, 1.0f);
            CHECK(aabb_min_x(w.get_shape(k)) == doctest::Approx(2.0f).epsilon(1e-3)); // seeking again
        }
    }

    // dt <= 0 has no meaningful reading: the solver integrates velocity*dt, so no displacement
    // would be applied, yet the standing target would still be cleared -- silently doing nothing
    // while cancelling intent. Abort instead, and leave the body untouched.
    TEST_CASE("an immediate move rejects a non-positive duration") {
        world w;
        const collider_id k = w.add(1, mk_kine(aabb{{0, 0}, {1, 1}}, vec{0, 0}));
        w.set_target(k, vec{50.0f, 0.5f});

        CHECK_THROWS((void) w.move_by(k, units::displacement{vec{5, 0}}, units::duration{0.0f}));
        CHECK_THROWS((void) w.move_to(k, vec{5.0f, 0.5f}, units::duration{-1.0f}));

        // The rejected calls must not have half-applied: intent and position both intact.
        CHECK(w.has_target(k));
        CHECK(std::fabs(aabb_min_x(w.get_shape(k))) < 1e-4f);
    }

    TEST_CASE("move_to resolves immediately, clamps at a wall, leaves the body at rest") {
        world w;
        w.add(1, mk_static(aabb{{6, -10}, {7, 10}}));                            // wall, left face x=6
        const collider_id k = w.add(2, mk_kine(aabb{{0, 0}, {1, 1}}, vec{0, 0}));
        const move_result r = w.move_to(k, vec{100.0f, 0.5f}, units::duration{1.0f});
        CHECK(r.blocked());                                                      // hit the wall
        CHECK(r.remaining.x() > 90.0f);                                          // most of the request was blocked
        CHECK(std::get<aabb>(w.get_shape(k)).max.x() <= 6.0f + 1e-3f);          // stopped at the wall
        CHECK(std::fabs(w.get_velocity(k).x()) < 1e-4f);                         // left at rest (no re-apply)
        (void) w.run(BIG_REGION, 1.0f);                                          // must not drift afterwards
        CHECK(std::get<aabb>(w.get_shape(k)).max.x() <= 6.0f + 1e-3f);
    }
}

TEST_SUITE("world::run -- bullet pass") {
    TEST_CASE("bullet miss advances full delta; hit stops short + BULLET_HIT") {
        SUBCASE("miss") {
            world w;
            const collider_id b = w.add(1, [] { bullet x; x.shape = circle{{0, 0}, 0.25f}; x.velocity = vec{10, 0}; return x; }());
            const auto& ev = w.run(BIG_REGION, 0.5f);
            CHECK(ev.empty());
            CHECK(std::fabs(std::get<circle>(w.get_shape(b)).center.x() - 5.0f) < 1e-4f);
        }
        SUBCASE("hit") {
            world w;
            const collider_id wall = w.add(1, mk_static(aabb{{4, -5}, {5, 5}})); // left face x=4
            bullet bx; bx.shape = circle{{0, 0}, 0.25f}; bx.velocity = vec{20, 0};
            const collider_id b = w.add(2, bx);
            const auto& ev = w.run(BIG_REGION, 0.5f); // delta = 10
            REQUIRE(count_kind(ev, event_kind::BULLET_HIT) == 1);
            const world_event* e = find_kind(ev, event_kind::BULLET_HIT);
            CHECK(e->mover.value == b.value);
            CHECK(e->target.value == wall.value);
            // circle r=0.25 reaches the wall's left face when center.x = 3.75 -> toi = 3.75/10.
            CHECK(e->toi == doctest::Approx(0.375f).epsilon(1e-3));
            CHECK(e->normal.x() == doctest::Approx(-1.0f));               // wall's left face
            CHECK(std::get<circle>(w.get_shape(b)).center.x() == doctest::Approx(3.75f).epsilon(1e-3));
        }
    }

    // Regression: a circle bullet that rests exactly on a wall's Minkowski boundary and then
    // moves AWAY must separate, not re-graze at toi 0 and get pinned (delta*0). Before the
    // to_swept_hit_forward filter this stuck the ball to walls/paddle in KE, vibrating with a
    // repeated bounce sound.
    TEST_CASE("resting circle bullet moving away separates (no toi-0 pin)") {
        world w;
        w.add(1, mk_static(aabb{{4, -5}, {5, 5}}));                        // left face x=4
        // Tangent to the wall (center.x = 4 - r), velocity pointing AWAY from it.
        bullet bx; bx.shape = circle{{3.75f, 0}, 0.25f}; bx.velocity = vec{-20, 0};
        const collider_id b = w.add(2, bx);
        const auto& ev = w.run(BIG_REGION, 0.5f);                          // delta = -10
        CHECK(count_kind(ev, event_kind::BULLET_HIT) == 0);                // separating, not a collision
        CHECK(std::get<circle>(w.get_shape(b)).center.x() == doctest::Approx(-6.25f).epsilon(1e-3)); // full delta
    }

    TEST_CASE("bounced circle bullet leaves the wall the next frame") {
        world w;
        const collider_id wall = w.add(1, mk_static(aabb{{4, -5}, {5, 5}}));
        bullet bx; bx.shape = circle{{0, 0}, 0.25f}; bx.velocity = vec{20, 0};
        const collider_id b = w.add(2, bx);
        // Frame 1: approach + hit, stops at the contact (center.x = 3.75).
        REQUIRE(count_kind(w.run(BIG_REGION, 0.5f), event_kind::BULLET_HIT) == 1);
        REQUIRE(std::get<circle>(w.get_shape(b)).center.x() == doctest::Approx(3.75f).epsilon(1e-3));
        (void) wall;
        // The game reflects on BULLET_HIT: velocity now points away from the wall.
        w.set_velocity(b, vec{-20, 0});
        // Frame 2: must actually move away, not pin at toi 0 re-grazing the resting contact.
        const auto& ev2 = w.run(BIG_REGION, 0.5f);
        CHECK(count_kind(ev2, event_kind::BULLET_HIT) == 0);
        CHECK(std::get<circle>(w.get_shape(b)).center.x() < 3.0f);        // advanced away from 3.75
    }

    // Regression: a bullet that STARTS engulfed in a solid (e.g. a fast paddle swung onto it)
    // must be ejected along the MTV, not pinned at toi 0 deep inside. In KE this stuck the ball
    // in the middle of the paddle, vibrating with repeated hit sounds until the paddle moved.
    TEST_CASE("engulfed circle bullet is depenetrated, not pinned inside a solid") {
        world w;
        w.add(1, mk_static(aabb{{-5, -1}, {5, 1}}));                       // solid slab, top face y=1
        bullet bx; bx.shape = circle{{0, 0.3f}, 0.5f}; bx.velocity = vec{0, 8}; // spawned INSIDE it
        const collider_id b = w.add(2, bx);
        const auto& ev = w.run(BIG_REGION, 0.1f);
        CHECK(count_kind(ev, event_kind::BULLET_HIT) == 1);               // reported so the game can bounce
        const circle after = std::get<circle>(w.get_shape(b));
        // Ejected through the nearest face (top) instead of pinning: the circle clears the slab.
        CHECK(after.center.y() - after.radius >= 1.0f - 1e-3f);
    }

    // The resting-leave filter must also cover segment and triangle obstacles: their sweeps set
    // entry_time=0 in a separate start-overlap branch that bypassed to_swept_hit_forward, so a
    // bounced bullet pinned flush on a segment wall or triangle slope. Bounce off it, then check
    // the reflected bullet leaves instead of re-grazing at toi 0.
    TEST_CASE("bounced bullet leaves a segment/triangle surface (no flush pin)") {
        SUBCASE("segment") {
            world w;
            w.add(1, mk_static(segment{{-5, 0}, {5, 0}}));                 // horizontal wall at y=0
            bullet bx; bx.shape = aabb{{-0.5f, 2.0f}, {0.5f, 3.0f}};       // above, falling toward it
            bx.velocity = vec{0, -20};
            const collider_id b = w.add(2, bx);
            // Frame 1: fall onto the segment, stopping flush (bottom at y=0).
            REQUIRE(count_kind(w.run(BIG_REGION, 0.5f), event_kind::BULLET_HIT) == 1);
            REQUIRE(std::get<aabb>(w.get_shape(b)).min.y() == doctest::Approx(0.0f).epsilon(1e-3));
            w.set_velocity(b, vec{0, 20});                                 // the game reflects
            // Frame 2: must leave, not pin flush at toi 0.
            const auto& ev2 = w.run(BIG_REGION, 0.5f);
            CHECK(count_kind(ev2, event_kind::BULLET_HIT) == 0);
            CHECK(std::get<aabb>(w.get_shape(b)).min.y() > 1.0f);
        }
        SUBCASE("triangle") {
            world w;
            w.add(1, mk_static(triangle{{-5, 0}, {5, 0}, {0, -5}}));       // top edge along y=0
            bullet bx; bx.shape = circle{{0, 2.0f}, 0.5f};                 // above, falling toward it
            bx.velocity = vec{0, -20};
            const collider_id b = w.add(2, bx);
            REQUIRE(count_kind(w.run(BIG_REGION, 0.5f), event_kind::BULLET_HIT) == 1);
            w.set_velocity(b, vec{0, 20});                                 // the game reflects
            const auto& ev2 = w.run(BIG_REGION, 0.5f);
            CHECK(count_kind(ev2, event_kind::BULLET_HIT) == 0);
            CHECK(std::get<circle>(w.get_shape(b)).center.y() > 1.0f);
        }
    }

    // Two bullets can hit the SAME target in one run (KE: two balls of a multiball reaching one
    // brick). The event batch is resolved after run() returns, so a game that despawns the target
    // while handling the first event invalidates the second event's target handle. Pin that: the
    // batch is not self-consistent, and a consumer must re-check validity per event rather than
    // assume every reported target still exists.
    TEST_CASE("two bullets hitting one target: removing it invalidates the later event's handle") {
        world w;
        const collider_id wall = w.add(1, mk_static(aabb{{4, -5}, {5, 5}}));   // left face x=4
        bullet a; a.shape = circle{{0, 1.0f}, 0.25f}; a.velocity = vec{20, 0};
        bullet b; b.shape = circle{{0, -1.0f}, 0.25f}; b.velocity = vec{20, 0};
        w.add(2, a);
        w.add(3, b);

        const auto& ev = w.run(BIG_REGION, 0.5f);
        REQUIRE(count_kind(ev, event_kind::BULLET_HIT) == 2);                  // both reached it

        // Collect the two targets before mutating (the buffer is invalidated by the next run()).
        std::vector<collider_id> targets;
        for (const world_event& e : ev) {
            if (e.kind == event_kind::BULLET_HIT) {
                targets.push_back(e.target);
            }
        }
        REQUIRE(targets.size() == 2);
        CHECK(targets[0].value == wall.value);
        CHECK(targets[1].value == wall.value);                                 // same target twice

        // Handling event 0 despawns the target, exactly as a game destroying a brick would.
        REQUIRE(w.is_valid(targets[0]));
        w.remove(targets[0]);

        // Event 1 now names a dead collider: is_valid says so, and dereferencing it aborts --
        // which is why the consumer must guard instead of calling get_eid unconditionally.
        CHECK_FALSE(w.is_valid(targets[1]));
        CHECK_THROWS((void) w.get_eid(targets[1]));
    }

    // Same-step response (bullet_on_hit): a bullet's frame movement is a TIME BUDGET. `stop`
    // forfeits whatever is left after a contact -- so the projectile visibly rests on the surface
    // for a frame -- while `bounce`/`slide` re-aim and spend the remainder, leaving the surface
    // within the same step.
    TEST_CASE("bullet_on_hit spends the leftover step time") {
        const auto mk = [](vec v, bullet_on_hit policy) {
            bullet b;
            b.shape = circle{{0, 0}, 0.25f};
            b.velocity = v;
            b.on_hit = policy;
            return b;
        };

        SUBCASE("stop (the default) parks at the contact, as before") {
            world w;
            w.add(1, mk_static(aabb{{4, -5}, {5, 5}}));           // left face x=4
            const collider_id b = w.add(2, mk(vec{20, 0}, bullet_on_hit::stop));
            REQUIRE(count_kind(w.run(BIG_REGION, 0.5f), event_kind::BULLET_HIT) == 1);
            // Reached the face and stopped: the remaining half of the step is forfeited.
            CHECK(std::get<circle>(w.get_shape(b)).center.x() == doctest::Approx(3.75f).epsilon(1e-3));
            CHECK(w.get_velocity(b).x() == doctest::Approx(20.0f));  // untouched; the game responds
        }

        SUBCASE("bounce reflects AND departs within the same step") {
            world w;
            w.add(1, mk_static(aabb{{4, -5}, {5, 5}}));
            const collider_id b = w.add(2, mk(vec{20, 0}, bullet_on_hit::bounce));
            // delta = 10; contact at centre 3.75 consumes 3.75/10 of it, leaving 6.25 to spend
            // travelling back the way it came.
            REQUIRE(count_kind(w.run(BIG_REGION, 0.5f), event_kind::BULLET_HIT) == 1);
            CHECK(w.get_velocity(b).x() == doctest::Approx(-20.0f));         // reflected
            CHECK(std::get<circle>(w.get_shape(b)).center.x()
                  == doctest::Approx(3.75f - 6.25f).epsilon(1e-2));          // already left the wall
        }

        SUBCASE("bounce preserves speed and mirrors only the normal axis") {
            world w;
            w.add(1, mk_static(aabb{{4, -5}, {5, 5}}));            // vertical face: mirrors x only
            bullet bx; bx.shape = circle{{0, 0}, 0.25f}; bx.velocity = vec{20, 5};
            bx.on_hit = bullet_on_hit::bounce;
            const collider_id b = w.add(2, bx);
            REQUIRE(count_kind(w.run(BIG_REGION, 0.5f), event_kind::BULLET_HIT) == 1);
            CHECK(w.get_velocity(b).x() == doctest::Approx(-20.0f));         // normal axis mirrored
            CHECK(w.get_velocity(b).y() == doctest::Approx(5.0f));           // tangent preserved
        }

        SUBCASE("slide keeps the tangent and drops the into-surface component") {
            world w;
            w.add(1, mk_static(aabb{{4, -5}, {5, 5}}));
            bullet bx; bx.shape = circle{{0, 0}, 0.25f}; bx.velocity = vec{20, 5};
            bx.on_hit = bullet_on_hit::slide;
            const collider_id b = w.add(2, bx);
            REQUIRE(count_kind(w.run(BIG_REGION, 0.5f), event_kind::BULLET_HIT) == 1);
            CHECK(std::fabs(w.get_velocity(b).x()) < 1e-3f);                 // into-surface removed
            CHECK(w.get_velocity(b).y() == doctest::Approx(5.0f));           // grazes along it
            CHECK(std::get<circle>(w.get_shape(b)).center.y() > 0.5f);       // and actually moved
        }

        // A bounce that immediately meets another surface must resolve in the same step too --
        // this is what makes a corner behave, rather than parking the projectile in it.
        SUBCASE("a corner resolves within one step and terminates") {
            world w;
            w.add(1, mk_static(aabb{{4, -5}, {5, 5}}));            // right wall, face x=4
            w.add(2, mk_static(aabb{{-5, 4}, {5, 5}}));            // top wall, face y=4
            bullet bx; bx.shape = circle{{0, 0}, 0.25f}; bx.velocity = vec{20, 20};
            bx.on_hit = bullet_on_hit::bounce;
            const collider_id b = w.add(3, bx);
            const auto& ev = w.run(BIG_REGION, 0.5f);
            CHECK(count_kind(ev, event_kind::BULLET_HIT) >= 1);
            // Both components reversed after bouncing off both faces; the run terminated (the
            // max_bullet_responses cap guarantees it cannot grind).
            CHECK(w.get_velocity(b).x() < 0.0f);
            CHECK(w.get_velocity(b).y() < 0.0f);
        }

        // Each cast reports toi along the CURRENT remaining segment, so a follow-up contact would
        // report a small local fraction (0.2) for something that actually happened late in the step
        // (0.8). Events must carry cumulative step time, or impacts cannot be ordered or placed.
        SUBCASE("follow-up contacts report cumulative step time, not segment-local toi") {
            world w;
            w.add(1, mk_static(aabb{{4, -5}, {5, 5}}));            // right wall, face x=4
            w.add(2, mk_static(aabb{{-5, -5}, {-4, 5}}));          // left wall, face x=-4
            bullet bx; bx.shape = circle{{0, 0}, 0.25f}; bx.velocity = vec{20, 0};
            bx.on_hit = bullet_on_hit::bounce;
            w.add(3, bx);
            // delta = 20: hits the right wall at 3.75 (toi 0.1875), rebounds and crosses to the
            // left wall -- a second contact late in the same step.
            const auto& ev = w.run(BIG_REGION, 1.0f);
            std::vector<float> tois;
            for (const world_event& e : ev) {
                if (e.kind == event_kind::BULLET_HIT) {
                    tois.push_back(e.toi);
                }
            }
            REQUIRE(tois.size() >= 2);
            CHECK(tois[0] == doctest::Approx(0.1875f).epsilon(1e-2));  // first, along the full step
            CHECK(tois[1] > tois[0]);                                  // strictly later in the step
            CHECK(tois[1] > 0.5f);                                     // NOT the small local value
            CHECK(tois[1] <= 1.0f);                                    // still normalized
        }

        SUBCASE("the response cap bounds the work") {
            world_config cfg;
            cfg.max_bullet_responses = 1;                          // no in-step response at all
            world w(cfg);
            w.add(1, mk_static(aabb{{4, -5}, {5, 5}}));
            const collider_id b = w.add(2, mk(vec{20, 0}, bullet_on_hit::bounce));
            REQUIRE(count_kind(w.run(BIG_REGION, 0.5f), event_kind::BULLET_HIT) == 1);
            // Capped: it parks at the contact exactly like `stop` rather than spending the leftover.
            CHECK(std::get<circle>(w.get_shape(b)).center.x() == doctest::Approx(3.75f).epsilon(1e-3));
        }
    }

    TEST_CASE("region cull: off-region bullet skips the cast but still flies") {
        world w;
        const aabb region{{-10, -10}, {10, 10}};
        w.add(1, mk_static(aabb{{104, -5}, {105, 5}}));                    // wall in the path, off-region
        bullet bx; bx.shape = circle{{100, 0}, 0.25f}; bx.velocity = vec{20, 0};
        const collider_id b = w.add(2, bx);
        const auto& ev = w.run(region, 0.5f);
        CHECK(ev.empty());                                                 // no cast -> no event
        CHECK(std::fabs(std::get<circle>(w.get_shape(b)).center.x() - 110.0f) < 1e-4f); // moved full delta
    }

    TEST_CASE("fast bullet entering the region is caught by the swept bound") {
        world w;
        const aabb region{{-10, -10}, {10, 10}};
        w.add(1, mk_static(aabb{{0, -5}, {1, 5}}));                        // wall at origin
        bullet bx; bx.shape = circle{{-12, 0}, 0.25f}; bx.velocity = vec{40, 0};
        w.add(2, bx);
        CHECK(count_kind(w.run(region, 0.5f), event_kind::BULLET_HIT) == 1);
    }

    TEST_CASE("BULLET_EXPIRED when a bullet leaves world bounds; game despawns") {
        world_config cfg; cfg.bounds = aabb{{-10, -10}, {10, 10}};
        world w(cfg);
        bullet bx; bx.shape = circle{{9, 0}, 0.25f}; bx.velocity = vec{5, 0}; // ends at x=14, out
        const collider_id b = w.add(1, bx);
        const auto& ev = w.run(BIG_REGION, 1.0f);
        REQUIRE(count_kind(ev, event_kind::BULLET_EXPIRED) == 1);
        const world_event* e = find_kind(ev, event_kind::BULLET_EXPIRED);
        CHECK(e->mover.value == b.value);
        CHECK(w.is_valid(e->mover));        // still live in the event
        w.remove(e->mover);                 // game despawns in reaction
        CHECK_FALSE(w.is_valid(b));
    }

    TEST_CASE("unbounded world never expires bullets") {
        world w; // no bounds
        bullet bx; bx.shape = circle{{1000, 0}, 0.25f}; bx.velocity = vec{5, 0};
        w.add(1, bx);
        CHECK(w.run(BIG_REGION, 1.0f).empty());
    }

    TEST_CASE("bullets pass through SENSOR and IGNORE bodies (only solids stop them)") {
        for (const response_mode mode : {response_mode::SENSOR, response_mode::IGNORE}) {
            world w;
            w.add(1, mk_static(aabb{{4, -5}, {5, 5}}, mode)); // non-solid body in the path
            bullet bx; bx.shape = circle{{0, 0}, 0.25f}; bx.velocity = vec{20, 0};
            const collider_id b = w.add(2, bx);
            const auto& ev = w.run(BIG_REGION, 0.5f);
            CHECK(count_kind(ev, event_kind::BULLET_HIT) == 0);                // not stopped
            CHECK(std::fabs(std::get<circle>(w.get_shape(b)).center.x() - 10.0f) < 1e-3f); // full delta
        }
    }
}

TEST_SUITE("world::run -- trigger pass") {
    TEST_CASE("enter / stay / leave a sensor fires begin/end exactly once each") {
        world w;
        const collider_id zone = w.add(1, mk_static(aabb{{0, 0}, {4, 4}}, response_mode::SENSOR));
        const collider_id k = w.add(2, mk_kine(aabb{{-2, 1}, {-1, 2}}, vec{0, 0})); // outside

        // not overlapping yet
        {
            const auto& ev = w.run(BIG_REGION, 1.0f);
            CHECK(count_kind(ev, event_kind::TRIGGER_BEGIN) == 0);
            CHECK(count_kind(ev, event_kind::TRIGGER_END) == 0);
        }
        // enter -> BEGIN once, naming (sensor, other)
        w.set_shape(k, shape_t{aabb{{1, 1}, {2, 2}}});
        {
            const auto& ev = w.run(BIG_REGION, 1.0f);
            REQUIRE(count_kind(ev, event_kind::TRIGGER_BEGIN) == 1);
            CHECK(count_kind(ev, event_kind::TRIGGER_END) == 0);
            const world_event* b = find_kind(ev, event_kind::TRIGGER_BEGIN);
            CHECK(b->mover.value == zone.value);
            CHECK(b->target.value == k.value);
        }
        // stay -> no edge
        {
            const auto& ev = w.run(BIG_REGION, 1.0f);
            CHECK(count_kind(ev, event_kind::TRIGGER_BEGIN) == 0);
            CHECK(count_kind(ev, event_kind::TRIGGER_END) == 0);
        }
        // leave -> END once
        w.set_shape(k, shape_t{aabb{{10, 10}, {11, 11}}});
        {
            const auto& ev = w.run(BIG_REGION, 1.0f);
            CHECK(count_kind(ev, event_kind::TRIGGER_BEGIN) == 0);
            CHECK(count_kind(ev, event_kind::TRIGGER_END) == 1);
        }
    }

    TEST_CASE("removing a body while inside a sensor fires END") {
        world w;
        w.add(1, mk_static(aabb{{0, 0}, {4, 4}}, response_mode::SENSOR));
        const collider_id k = w.add(2, mk_kine(aabb{{1, 1}, {2, 2}}, vec{0, 0}));

        CHECK(count_kind(w.run(BIG_REGION, 1.0f), event_kind::TRIGGER_BEGIN) == 1);
        w.remove(k);
        CHECK(count_kind(w.run(BIG_REGION, 1.0f), event_kind::TRIGGER_END) == 1);
    }

    TEST_CASE("slot recycled between runs fires END(old) + BEGIN(new), not suppressed") {
        // The generation-aware pair key must treat a reused slot as a distinct pair, so the diff
        // does not mistake "different body, same slot" for "still overlapping."
        world w;
        w.add(1, mk_static(aabb{{0, 0}, {4, 4}}, response_mode::SENSOR));
        const collider_id k = w.add(2, mk_kine(aabb{{1, 1}, {2, 2}}, vec{0, 0}));
        CHECK(count_kind(w.run(BIG_REGION, 1.0f), event_kind::TRIGGER_BEGIN) == 1);

        w.remove(k);                                       // free the slot between runs
        const collider_id k2 = w.add(3, mk_kine(aabb{{1, 1}, {2, 2}}, vec{0, 0}));
        REQUIRE(k2.value == k.value);                      // same slot reused
        REQUIRE(k2.generation != k.generation);            // new generation

        const auto& ev = w.run(BIG_REGION, 1.0f);
        CHECK(count_kind(ev, event_kind::TRIGGER_END) == 1);   // old pair ended
        CHECK(count_kind(ev, event_kind::TRIGGER_BEGIN) == 1); // new pair began
    }

    TEST_CASE("a solid (BLOCK) overlap is NOT a trigger") {
        world w;
        // a non-sensor static the mover ends up overlapping is reported by movement, never as a trigger
        w.add(1, mk_static(aabb{{0, 0}, {4, 4}}, response_mode::BLOCK));
        w.add(2, mk_kine(aabb{{-2, 1}, {-1, 2}}, vec{5, 0})); // moves into it
        const auto& ev = w.run(BIG_REGION, 1.0f);
        CHECK(count_kind(ev, event_kind::TRIGGER_BEGIN) == 0);
        CHECK(count_kind(ev, event_kind::TRIGGER_END) == 0);
    }

    TEST_CASE("sensor filter: only senses matching categories") {
        world w;
        static_body zone = mk_static(aabb{{0, 0}, {4, 4}}, response_mode::SENSOR);
        zone.filter.category = 0x0001; zone.filter.mask = 0x0002; // senses only category 0x0002
        w.add(1, zone);
        kinematic_body k = mk_kine(aabb{{1, 1}, {2, 2}}, vec{0, 0});
        k.filter.category = 0x0004; k.filter.mask = 0xFFFF;       // not in the zone's mask
        w.add(2, k);
        CHECK(count_kind(w.run(BIG_REGION, 1.0f), event_kind::TRIGGER_BEGIN) == 0);
    }
}

TEST_SUITE("world::run -- event buffer") {
    TEST_CASE("returns a reused buffer, cleared and refilled each call") {
        world w;
        w.add(1, mk_static(aabb{{0, 0}, {4, 4}}, response_mode::SENSOR));
        const collider_id k = w.add(2, mk_kine(aabb{{1, 1}, {2, 2}}, vec{0, 0}));

        const auto& e1 = w.run(BIG_REGION, 1.0f);  // BEGIN
        const std::size_t n1 = e1.size();
        const auto& e2 = w.run(BIG_REGION, 1.0f);  // stay -> empty
        CHECK(&e1 == &e2);          // same buffer object
        CHECK(n1 == 1);
        CHECK(e2.empty());          // cleared, not appended

        w.remove(k);
        CHECK(count_kind(w.run(BIG_REGION, 1.0f), event_kind::TRIGGER_END) == 1);
    }
}
