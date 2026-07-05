//
// Created by igor on 21/06/2026.
//
// Tests for the collision world (Layer 3) skeleton in
// <neutrino/physics/collide/world.hh>: collider lifecycle (add/remove), the
// generation-tagged handle (collider_id) and its validity/recycling semantics, and
// the mutator precondition guards (set_shape / set_velocity / remove).
//
// Scope note: the world does not yet expose any read-back of the broadphase proxy or
// the stored body, so the "every resident has a fat box that bounds its shape"
// invariant cannot be observed here -- it belongs to the Phase 2 query API. These
// tests cover exactly what the public surface allows today, with ENFORCE-guard checks
// via CHECK_THROWS (ENFORCE throws).
//
#include <doctest/doctest.h>

#include <set>
#include <unordered_map>
#include <vector>

#include <neutrino/physics/collide/world.hh>

using namespace neutrino::physics;

namespace {
    aabb box_at(float x, float y, float w = 1.0f, float h = 1.0f) {
        return aabb{{x, y}, {x + w, y + h}};
    }

    static_body mk_static(const shape_t& s) {
        static_body b;
        b.shape = s;
        return b;
    }

    kinematic_body mk_kine(const moving_shape_t& s, vec v) {
        kinematic_body b;
        b.shape = s;
        b.velocity = v;
        return b;
    }

    bullet mk_bullet(const moving_shape_t& s, vec v) {
        bullet b;
        b.shape = s;
        b.velocity = v;
        return b;
    }
} // namespace

TEST_SUITE("world: handle lifecycle") {
    TEST_CASE("add returns a valid handle") {
        world w;
        const collider_id a = w.add(1, mk_static(box_at(0, 0)));
        const collider_id b = w.add(2, mk_kine(box_at(5, 5), vec{1, 0}));
        CHECK(w.is_valid(a));
        CHECK(w.is_valid(b));
    }

    TEST_CASE("distinct live bodies get distinct slots") {
        world w;
        const collider_id a = w.add(1, mk_static(box_at(0, 0)));
        const collider_id b = w.add(2, mk_static(box_at(1, 1)));
        const collider_id c = w.add(3, mk_static(box_at(2, 2)));
        CHECK(a.value != b.value);
        CHECK(b.value != c.value);
        CHECK(a.value != c.value);
        CHECK(w.is_valid(a));
        CHECK(w.is_valid(b));
        CHECK(w.is_valid(c));
    }

    TEST_CASE("remove invalidates the handle") {
        world w;
        const collider_id a = w.add(1, mk_static(box_at(0, 0)));
        REQUIRE(w.is_valid(a));
        w.remove(a);
        CHECK_FALSE(w.is_valid(a));
    }

    TEST_CASE("remove of a stale handle is an idempotent no-op (no throw)") {
        world w;
        const collider_id a = w.add(1, mk_static(box_at(0, 0)));
        w.remove(a);
        CHECK_NOTHROW(w.remove(a)); // double remove: gated by is_valid, never hits the double-free ENFORCE
        CHECK_NOTHROW(w.remove(a)); // and again
        CHECK_FALSE(w.is_valid(a));
    }

    TEST_CASE("is_valid rejects out-of-range and mismatched-generation handles") {
        world w;
        const collider_id a = w.add(1, mk_static(box_at(0, 0)));

        const collider_id oob{9999u, 0u};                       // value past the pool
        CHECK_FALSE(w.is_valid(oob));

        const collider_id wrong_gen{a.value, a.generation + 7}; // right slot, wrong generation
        CHECK_FALSE(w.is_valid(wrong_gen));

        CHECK(w.is_valid(a)); // the genuine handle still validates
    }

    TEST_CASE("handles into an empty world are invalid (no UB)") {
        const world w;
        CHECK_FALSE(w.is_valid(collider_id{0u, 0u}));
        CHECK_FALSE(w.is_valid(collider_id{123u, 0u}));
    }

    TEST_CASE("default-constructed collider_id is the null sentinel") {
        const collider_id null{};
        CHECK_FALSE(null.valid());                 // the cheap self-check
        CHECK(null.value == collider_id::INVALID);

        world w;
        CHECK_FALSE(w.is_valid(null));             // and the storage check rejects it too

        const collider_id a = w.add(1, mk_static(box_at(0, 0)));
        CHECK(a.valid());                          // a real handle is not null
        CHECK(w.is_valid(a));
    }
}

TEST_SUITE("world: bullet pool lifecycle") {
    TEST_CASE("add returns a BULLET-typed valid handle; remove invalidates") {
        world w;
        const collider_id b = w.add(7, mk_bullet(circle{{0, 0}, 0.5f}, vec{1, 0}));
        CHECK(b.type_id == collider_id::BULLET);
        CHECK(w.is_valid(b));
        CHECK(w.get_eid(b) == 7u);
        w.remove(b);
        CHECK_FALSE(w.is_valid(b));
        CHECK_NOTHROW(w.remove(b)); // idempotent on a stale handle
    }

    TEST_CASE("bullet slot recycles with a fresh generation; old handle goes stale") {
        world w;
        const collider_id a = w.add(1, mk_bullet(circle{{0, 0}, 0.5f}, vec{1, 0}));
        w.remove(a);
        const collider_id b = w.add(2, mk_bullet(circle{{9, 9}, 0.5f}, vec{0, 1}));
        CHECK(b.value == a.value);              // same slot
        CHECK(b.generation == a.generation + 1u);
        CHECK(w.is_valid(b));
        CHECK_FALSE(w.is_valid(a));             // stale handle to the recycled bullet slot
    }

    TEST_CASE("bullet and body pools are independent (same slot index, different type)") {
        world w;
        const collider_id body = w.add(1, mk_static(box_at(0, 0)));
        const collider_id bul = w.add(2, mk_bullet(circle{{0, 0}, 0.5f}, vec{1, 0}));
        CHECK(body.type_id == collider_id::BODY);
        CHECK(bul.type_id == collider_id::BULLET);
        // both are slot 0 of their own pool, but the type tag keeps them distinct + valid
        CHECK(body.value == bul.value);
        CHECK(w.is_valid(body));
        CHECK(w.is_valid(bul));
    }

    TEST_CASE("set_velocity / set_shape on a live bullet; set_shape(segment) throws") {
        world w;
        const collider_id b = w.add(1, mk_bullet(circle{{0, 0}, 0.5f}, vec{1, 0}));
        CHECK_NOTHROW(w.set_velocity(b, vec{5, -2}));     // bullets accept velocity (no kind guard)
        CHECK(w.get_velocity(b).x() == doctest::Approx(5.0f));
        CHECK_NOTHROW(w.set_shape(b, shape_t{circle{{1, 1}, 0.25f}}));
        CHECK_NOTHROW(w.set_shape(b, shape_t{box_at(2, 2)}));
        CHECK_THROWS(w.set_shape(b, shape_t{segment{{0, 0}, {1, 1}}})); // a bullet cannot be a segment
    }

    TEST_CASE("mutating a stale bullet handle throws") {
        world w;
        const collider_id b = w.add(1, mk_bullet(circle{{0, 0}, 0.5f}, vec{1, 0}));
        w.remove(b);
        CHECK_THROWS(w.set_velocity(b, vec{1, 1}));
        CHECK_THROWS(w.set_shape(b, shape_t{circle{{0, 0}, 0.5f}}));
    }
}

TEST_SUITE("world: generation and slot recycling") {
    TEST_CASE("a recycled slot bumps generation and invalidates the old handle") {
        world w;
        const collider_id a = w.add(1, mk_static(box_at(0, 0)));
        w.remove(a);

        // The free list is LIFO and only one slot was freed, so the next add reuses it.
        const collider_id b = w.add(2, mk_static(box_at(9, 9)));
        CHECK(b.value == a.value);            // same physical slot
        CHECK(b.generation != a.generation);  // but a fresh generation
        CHECK(b.generation == a.generation + 1u);

        CHECK(w.is_valid(b));        // new handle is live
        CHECK_FALSE(w.is_valid(a));  // old handle to the recycled slot is dead
    }

    TEST_CASE("repeated recycle of the same slot keeps advancing the generation") {
        world w;
        collider_id prev = w.add(0, mk_static(box_at(0, 0)));
        const uint32_t slot = prev.value;
        for (int i = 0; i < 16; ++i) {
            w.remove(prev);
            const collider_id next = w.add(i, mk_static(box_at(0, 0)));
            CHECK(next.value == slot);                      // same slot reused each round
            CHECK(next.generation == prev.generation + 1u); // strictly increasing
            CHECK_FALSE(w.is_valid(prev));
            CHECK(w.is_valid(next));
            prev = next;
        }
    }

    TEST_CASE("churn: add/remove many, validity stays consistent") {
        world w;
        std::vector<collider_id> ids;
        for (int i = 0; i < 100; ++i) {
            ids.push_back(w.add(static_cast<entity_id_t>(i),
                                mk_kine(box_at(static_cast<float>(i), 0.0f), vec{1, 0})));
        }
        for (const auto& id : ids) {
            CHECK(w.is_valid(id));
        }

        // Remove the even-indexed ones; odd ones must remain valid, evens become invalid.
        for (std::size_t i = 0; i < ids.size(); i += 2) {
            w.remove(ids[i]);
        }
        for (std::size_t i = 0; i < ids.size(); ++i) {
            CHECK(w.is_valid(ids[i]) == (i % 2 == 1));
        }

        // Re-add as many as we removed: kept handles still valid, new handles valid,
        // and no new handle collides (slot+generation) with a still-live one.
        std::set<std::pair<uint32_t, uint32_t>> live;
        for (std::size_t i = 1; i < ids.size(); i += 2) {
            live.emplace(ids[i].value, ids[i].generation);
        }
        for (int i = 0; i < 50; ++i) {
            const collider_id n = w.add(1000 + i, mk_static(box_at(0, 0)));
            CHECK(w.is_valid(n));
            CHECK(live.emplace(n.value, n.generation).second); // unique among the living
        }
        for (std::size_t i = 1; i < ids.size(); i += 2) {
            CHECK(w.is_valid(ids[i]));
        }
    }
}

TEST_SUITE("world: mutator guards") {
    TEST_CASE("set_shape on a live body succeeds and keeps the handle valid") {
        world w;
        const collider_id a = w.add(1, mk_static(box_at(0, 0)));
        CHECK_NOTHROW(w.set_shape(a, box_at(10, 10, 2, 2)));
        CHECK_NOTHROW(w.set_shape(a, circle{{3, 3}, 1.5f})); // shape variant may change
        CHECK_NOTHROW(w.set_shape(a, segment{{0, 0}, {4, 1}}));
        CHECK(w.is_valid(a)); // mutation does not recycle the handle
    }

    TEST_CASE("set_velocity on a live kinematic body succeeds") {
        world w;
        const collider_id k = w.add(1, mk_kine(box_at(0, 0), vec{0, 0}));
        CHECK_NOTHROW(w.set_velocity(k, vec{3, -2}));
        CHECK(w.is_valid(k));
    }

    TEST_CASE("set_velocity on a static body throws (wrong kind)") {
        world w;
        const collider_id s = w.add(1, mk_static(box_at(0, 0)));
        CHECK_THROWS(w.set_velocity(s, vec{1, 0}));
    }

    TEST_CASE("mutating a stale handle throws") {
        world w;
        const collider_id k = w.add(1, mk_kine(box_at(0, 0), vec{1, 0}));
        w.remove(k);
        CHECK_THROWS(w.set_shape(k, box_at(0, 0)));
        CHECK_THROWS(w.set_velocity(k, vec{2, 2}));
    }
}

TEST_SUITE("world: construction") {
    TEST_CASE("custom config constructs and operates") {
        world_config cfg;
        cfg.fatten_margin = 0.5f;
        world w(cfg);
        const collider_id a = w.add(1, mk_kine(box_at(0, 0), vec{2, 0}));
        CHECK(w.is_valid(a));
        CHECK_NOTHROW(w.set_velocity(a, vec{0, -3}));
        w.remove(a);
        CHECK_FALSE(w.is_valid(a));
    }
}

TEST_SUITE("world: collider_id comparison & hashing") {
    TEST_CASE("aggregate init survives the defaulted comparisons") {
        const collider_id a{1, 0, collider_id::BODY}; // must still compile as an aggregate
        CHECK(a.value == 1u);
        CHECK(a.generation == 0u);
        CHECK(a.type_id == collider_id::BODY);
        CHECK(collider_id{}.valid() == false); // default = null sentinel
    }

    TEST_CASE("equality is full identity (value, generation, type)") {
        const collider_id a{1, 0, collider_id::BODY};
        CHECK(a == collider_id{1, 0, collider_id::BODY});
        CHECK(a != collider_id{2, 0, collider_id::BODY}); // value
        CHECK(a != collider_id{1, 1, collider_id::BODY}); // generation
        CHECK(a != collider_id{1, 0, collider_id::BULLET}); // type
    }

    TEST_CASE("ordering (<=>) is usable for std::set") {
        std::set<collider_id> s{
            {1, 0, collider_id::BODY},
            {1, 0, collider_id::BODY},   // duplicate -> deduped
            {1, 1, collider_id::BODY},
            {1, 0, collider_id::BULLET},
        };
        CHECK(s.size() == 3u);
        CHECK(s.count(collider_id{1, 0, collider_id::BODY}) == 1u);
        CHECK((collider_id{1, 0, collider_id::BODY} <=> collider_id{1, 1, collider_id::BODY})
              == std::strong_ordering::less);
    }

    TEST_CASE("std::hash keys unordered containers") {
        std::unordered_map<collider_id, int> m;
        m[collider_id{1, 0, collider_id::BODY}] = 7;
        m[collider_id{1, 1, collider_id::BODY}] = 9;   // distinct generation -> distinct key
        CHECK(m.size() == 2u);
        CHECK(m.at(collider_id{1, 0, collider_id::BODY}) == 7);
        CHECK(m.at(collider_id{1, 1, collider_id::BODY}) == 9);
    }
}

TEST_SUITE("world: static grid configuration") {
    TEST_CASE("a grid config with matching bounds constructs cleanly") {
        world_config cfg;
        cfg.bounds = aabb{vec{0, 0}, vec{64, 48}};
        cfg.grid = world_config::grid_config{vec{16, 16}}; // 4x3 cells, exact tiling
        CHECK_NOTHROW(world{cfg});
    }

    TEST_CASE("no grid config -> pure-dynamic world is fine") {
        CHECK_NOTHROW(world{world_config{}});
    }

    TEST_CASE("a grid config without bounds is rejected") {
        world_config cfg;
        cfg.grid = world_config::grid_config{vec{16, 16}};
        CHECK_THROWS(world{cfg});
    }

    TEST_CASE("bounds that do not tile evenly into tile_size are rejected") {
        world_config cfg;
        cfg.bounds = aabb{vec{0, 0}, vec{50, 48}}; // 50 / 16 is not integral
        cfg.grid = world_config::grid_config{vec{16, 16}};
        CHECK_THROWS(world{cfg});
    }
}

TEST_SUITE("world: tile_body lifecycle (unified add/remove/getters)") {
    static world grid_world() {
        world_config cfg;
        cfg.bounds = aabb{vec{0, 0}, vec{8, 8}};
        cfg.grid = world_config::grid_config{vec{2, 2}}; // 4x4 cells
        return world(cfg);
    }

    TEST_CASE("add(tile_body) returns a TILE handle; getters read it back") {
        world w = grid_world();
        const collider_id t = w.add(101, tile_body{aabb{vec{0, 0}, vec{2, 2}}, {}, {}});
        CHECK(t.type_id == collider_id::TILE);
        CHECK(w.is_valid(t));
        CHECK(w.get_eid(t) == 101u);
        CHECK(std::holds_alternative<aabb>(w.get_shape(t)));
        const bool zero_vel = (w.get_velocity(t) == vec{0, 0});
        CHECK(zero_vel); // tiles are static
    }

    TEST_CASE("a slope tile is just a segment, stored verbatim") {
        world w = grid_world();
        const collider_id t = w.add(7, tile_body{segment{vec{4, 4}, vec{6, 6}}, {}, {}});
        CHECK(std::holds_alternative<segment>(w.get_shape(t)));
    }

    TEST_CASE("distinct cells get distinct handles; remove is idempotent") {
        world w = grid_world();
        const collider_id a = w.add(1, tile_body{aabb{vec{0, 0}, vec{2, 2}}, {}, {}}); // cell (0,0)
        const collider_id b = w.add(2, tile_body{aabb{vec{4, 4}, vec{6, 6}}, {}, {}}); // cell (2,2)
        CHECK(a.value != b.value);
        w.remove(a);
        CHECK_FALSE(w.is_valid(a));
        CHECK_NOTHROW(w.remove(a)); // idempotent
        CHECK(w.is_valid(b));
    }

    TEST_CASE("adding into an occupied cell overwrites (same handle, new payload)") {
        world w = grid_world();
        const collider_id a = w.add(1, tile_body{aabb{vec{0, 0}, vec{2, 2}}, {}, {}});
        const collider_id b = w.add(2, tile_body{circle{vec{1, 1}, 0.5f}, {}, {}}); // same cell (0,0)
        CHECK(a.value == b.value);     // same cell handle
        CHECK(w.get_eid(b) == 2u);     // new payload won
        CHECK(std::holds_alternative<circle>(w.get_shape(b)));
    }

    TEST_CASE("set_shape reshapes a tile in place; set_velocity is rejected") {
        world w = grid_world();
        const collider_id t = w.add(1, tile_body{aabb{vec{0, 0}, vec{2, 2}}, {}, {}});
        w.set_shape(t, shape_t{circle{vec{1, 1}, 0.4f}});
        CHECK(std::holds_alternative<circle>(w.get_shape(t)));
        CHECK_THROWS(w.set_velocity(t, vec{1, 0}));
    }

    TEST_CASE("add(tile_body) without a grid, or out of bounds, is rejected") {
        world none(world_config{});
        CHECK_THROWS(none.add(1, tile_body{aabb{vec{0, 0}, vec{2, 2}}, {}, {}}));
        world w = grid_world();
        CHECK_THROWS(w.add(1, tile_body{aabb{vec{100, 100}, vec{102, 102}}, {}, {}}));
    }
}
