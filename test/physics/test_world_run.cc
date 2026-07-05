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
