//
// Created by igor on 25/06/2026.
//
// Integration tests for the world <-> static grid fan-out (G4): tiles added via the
// unified add(tile_body) participate in the world's queries (raycast / cast /
// overlap-triggers) and in move_and_slide, merged with the BVH residents (nearest
// for closest-hit queries, union for overlap). Tiles are bucketed into the single
// cell containing their centre, so every tile here is cell-sized.
//
#include <doctest/doctest.h>

#include <cmath>
#include <cstdint>
#include <optional>
#include <set>
#include <utility>
#include <variant>
#include <vector>

#include <neutrino/physics/collide/world.hh>

using namespace neutrino::physics;


namespace {
    // 8x8 cells of 2x2 over [0,16]^2.
    world grid_world() {
        world_config cfg;
        cfg.bounds = aabb{vec{0, 0}, vec{16, 16}};
        cfg.grid = world_config::grid_config{vec{2, 2}};
        return world(cfg);
    }

    aabb tight_of(const world& w, collider_id id) {
        return std::visit([](const auto& s) { return enclose(s); }, w.get_shape(id));
    }
}

TEST_SUITE("world+grid: raycast") {
    TEST_CASE("a ray hits a tile and reports its TILE handle + eid + toi") {
        world w = grid_world();
        w.add(1, tile_body{aabb{vec{6, 0}, vec{8, 2}}, {}, {}}); // cell (3,0)
        const auto hit = w.raycast(segment{vec{0, 1}, vec{16, 1}});
        REQUIRE(hit.has_value());
        CHECK(hit->who.type_id == collider_id::TILE);
        CHECK(w.get_eid(hit->who) == 1u);
        CHECK(hit->toi == doctest::Approx(0.375f)); // left face x=6 over a length-16 ray
    }

    TEST_CASE("the nearer of a tile and a body wins the merge") {
        SUBCASE("body is nearer") {
            world w = grid_world();
            w.add(10, tile_body{aabb{vec{10, 0}, vec{12, 2}}, {}, {}});          // far tile
            w.add(20, static_body{shape_t{aabb{vec{4, 0}, vec{5, 2}}}, {}, {}}); // near body
            const auto hit = w.raycast(segment{vec{0, 1}, vec{16, 1}});
            REQUIRE(hit.has_value());
            CHECK(hit->who.type_id == collider_id::BODY);
            CHECK(w.get_eid(hit->who) == 20u);
        }
        SUBCASE("tile is nearer") {
            world w = grid_world();
            w.add(10, tile_body{aabb{vec{4, 0}, vec{6, 2}}, {}, {}});             // near tile
            w.add(20, static_body{shape_t{aabb{vec{10, 0}, vec{11, 2}}}, {}, {}});// far body
            const auto hit = w.raycast(segment{vec{0, 1}, vec{16, 1}});
            REQUIRE(hit.has_value());
            CHECK(hit->who.type_id == collider_id::TILE);
            CHECK(w.get_eid(hit->who) == 10u);
        }
    }

    TEST_CASE("a slope (segment) tile is crossed like a wall") {
        world w = grid_world();
        w.add(3, tile_body{segment{vec{4, 4}, vec{6, 6}}, {}, {}}); // diagonal in cell (2,2)
        // a ray crossing that diagonal
        const auto hit = w.raycast(segment{vec{4, 6}, vec{6, 4}});
        REQUIRE(hit.has_value());
        CHECK(hit->who.type_id == collider_id::TILE);
        CHECK(w.get_eid(hit->who) == 3u);
    }

    TEST_CASE("filter excludes a tile") {
        world w = grid_world();
        filter_props tf; tf.category = 0x0002; tf.mask = 0xFFFF;
        w.add(1, tile_body{aabb{vec{6, 0}, vec{8, 2}}, {}, tf});
        filter_props qf; qf.category = 0xFFFF; qf.mask = 0x0001; // does not include the tile's category
        CHECK_FALSE(w.raycast(segment{vec{0, 1}, vec{16, 1}}, qf).has_value());
    }
}

TEST_SUITE("world+grid: triggers") {
    TEST_CASE("a sensor body senses an overlapping tile (TRIGGER_BEGIN, then END on exit)") {
        world w = grid_world();
        const collider_id tile = w.add(2, tile_body{aabb{vec{4, 4}, vec{6, 6}}, {}, {}}); // cell (2,2)
        material_props sens; sens.response = response_mode::SENSOR;
        const collider_id s = w.add(99, kinematic_body{
                                        moving_shape_t{aabb{vec{4.5f, 4.5f}, vec{5.5f, 5.5f}}}, sens, {}, vec{0, 0}});

        auto count_kind = [](const std::vector<world_event>& evs, event_kind k) {
            int n = 0;
            for (const auto& e : evs) {
                if (e.kind != k) continue;
                if (e.target.type_id == collider_id::TILE || e.mover.type_id == collider_id::TILE) ++n;
            }
            return n;
        };

        const auto& f1 = w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f);
        CHECK(count_kind(f1, event_kind::TRIGGER_BEGIN) == 1);

        // still overlapping next frame -> no repeat begin
        const auto& f2 = w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f);
        CHECK(count_kind(f2, event_kind::TRIGGER_BEGIN) == 0);

        // remove the tile -> the overlap ends
        w.remove(tile);
        const auto& f3 = w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f);
        CHECK(count_kind(f3, event_kind::TRIGGER_END) == 1);
        (void)s;
    }
}

TEST_SUITE("world+grid: sensor tiles") {
    static int count_kind(const std::vector<world_event>& evs, event_kind k) {
        int n = 0;
        for (const auto& e : evs) if (e.kind == k) ++n;
        return n;
    }

    TEST_CASE("a SENSOR tile senses a plain body over it (BEGIN, then END on exit)") {
        world w = grid_world();
        material_props sens; sens.response = response_mode::SENSOR;
        w.add(1, tile_body{aabb{vec{4, 4}, vec{6, 6}}, sens, {}}); // sensor tile, cell (2,2)
        const collider_id b = w.add(2, kinematic_body{
                                       moving_shape_t{aabb{vec{4.5f, 4.5f}, vec{5.5f, 5.5f}}}, {}, {}, vec{0, 0}});

        const auto& f1 = w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f);
        CHECK(count_kind(f1, event_kind::TRIGGER_BEGIN) == 1);
        bool named = false;
        for (const auto& e : f1)
            if (e.kind == event_kind::TRIGGER_BEGIN)
                named = e.mover.type_id == collider_id::TILE && w.get_eid(e.mover) == 1u
                        && w.get_eid(e.target) == 2u;
        CHECK(named); // the sensor side is the tile, the other side is the body

        const auto& f2 = w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f);
        CHECK(count_kind(f2, event_kind::TRIGGER_BEGIN) == 0); // still inside -> no repeat

        w.set_shape(b, shape_t{aabb{vec{0.1f, 0.1f}, vec{1.0f, 1.0f}}}); // body leaves the tile
        const auto& f3 = w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f);
        CHECK(count_kind(f3, event_kind::TRIGGER_END) == 1);
    }

    TEST_CASE("a sensor body and a sensor tile overlapping fire ONE deduped trigger") {
        world w = grid_world();
        material_props sens; sens.response = response_mode::SENSOR;
        w.add(1, tile_body{aabb{vec{4, 4}, vec{6, 6}}, sens, {}});
        w.add(2, kinematic_body{moving_shape_t{aabb{vec{4.5f, 4.5f}, vec{5.5f, 5.5f}}}, sens, {}, vec{0, 0}});
        const auto& f = w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f);
        CHECK(count_kind(f, event_kind::TRIGGER_BEGIN) == 1); // produced from both sides -> deduped
    }

    TEST_CASE("overwriting a sensor tile with a plain tile stops it triggering (stale entry pruned)") {
        world w = grid_world();
        material_props sens; sens.response = response_mode::SENSOR;
        w.add(1, tile_body{aabb{vec{4, 4}, vec{6, 6}}, sens, {}});
        w.add(2, kinematic_body{moving_shape_t{aabb{vec{4.5f, 4.5f}, vec{5.5f, 5.5f}}}, {}, {}, vec{0, 0}});
        CHECK(count_kind(w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f), event_kind::TRIGGER_BEGIN) == 1);

        w.add(3, tile_body{aabb{vec{4, 4}, vec{6, 6}}, {}, {}}); // overwrite same cell, non-sensor
        CHECK(count_kind(w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f), event_kind::TRIGGER_END) == 1);
        CHECK(count_kind(w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f), event_kind::TRIGGER_BEGIN) == 0);
    }

    TEST_CASE("a sensor tile is passable: a mover falls through it, doesn't land") {
        // SENSOR floor (does not block) vs the same as a BLOCK floor (does), same drop.
        auto drop_onto = [](response_mode resp) {
            world w = grid_world();
            material_props m; m.response = resp; m.block_normal = vec{0, 1};
            w.add(1, tile_body{aabb{vec{2, 0}, vec{4, 2}}, m, {}}); // floor tile, cell (1,0)
            const collider_id b = w.add(2, kinematic_body{
                                           moving_shape_t{aabb{vec{2.5f, 3.0f}, vec{3.5f, 4.0f}}}, {}, {}, vec{0, -120}});
            (void)w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f); // dy = -2
            return std::visit([](const auto& s) { return enclose(s); }, w.get_shape(b)).min.y();
        };
        CHECK(drop_onto(response_mode::BLOCK) >= 2.0f - 0.05f); // BLOCK floor stops it at the top
        CHECK(drop_onto(response_mode::SENSOR) < 2.0f);         // SENSOR floor: it falls through
    }
}

TEST_SUITE("world+grid: move_and_slide") {
    TEST_CASE("a falling body lands on a tile floor (does not tunnel through)") {
        world w = grid_world();
        material_props floor; floor.response = response_mode::BLOCK; floor.block_normal = vec{0, 1};
        w.add(5, tile_body{aabb{vec{2, 0}, vec{4, 2}}, floor, {}}); // floor tile, cell (1,0)
        const collider_id b = w.add(6, kinematic_body{
                                        moving_shape_t{aabb{vec{2.5f, 3.0f}, vec{3.5f, 4.0f}}}, {}, {}, vec{0, -120}});

        const float y0 = tight_of(w, b).min.y();
        const auto& evs = w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f); // dy = -2 (into the floor)
        const float y1 = tight_of(w, b).min.y();

        CHECK(y1 < y0);              // it fell
        CHECK(y1 >= 2.0f - 0.05f);   // but stopped at/above the floor top y=2 (within skin)

        bool collided_with_tile = false;
        for (const auto& e : evs) {
            if (e.kind == event_kind::COLLISION && e.target.type_id == collider_id::TILE) {
                collided_with_tile = true;
                CHECK(w.get_eid(e.target) == 5u);
            }
        }
        CHECK(collided_with_tile);
    }

    TEST_CASE("a body slides along a tile wall instead of stopping dead") {
        world w = grid_world();
        material_props wall; wall.response = response_mode::BLOCK; wall.block_normal = vec{-1, 0};
        w.add(7, tile_body{aabb{vec{6, 0}, vec{8, 2}}, wall, {}}); // wall tile, cell (3,0)
        // move down-right into the wall's left face; should slide down along it
        const collider_id b = w.add(8, kinematic_body{
                                        moving_shape_t{aabb{vec{4.0f, 0.5f}, vec{5.0f, 1.5f}}}, {}, {}, vec{120, -60}});
        const float x0 = tight_of(w, b).min.x();
        (void)w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f);
        const aabb after = tight_of(w, b);
        CHECK(after.min.x() > x0);          // advanced toward the wall
        CHECK(after.max.x() <= 6.0f + 0.05f); // but did not cross the wall's left face x=6
    }
}

TEST_SUITE("world+grid: cast (aiming probe)") {
    TEST_CASE("a swept cast reports the earliest tile contact") {
        world w = grid_world();
        w.add(1, tile_body{aabb{vec{6, 0}, vec{8, 2}}, {}, {}}); // cell (3,0)
        // sweep a small box rightward through the tile
        const auto hit = w.cast(moving_shape_t{aabb{vec{0, 0.4f}, vec{1, 1.4f}}}, vec{16, 0});
        REQUIRE(hit.has_value());
        CHECK(hit->who.type_id == collider_id::TILE);
        CHECK(w.get_eid(hit->who) == 1u);
    }
}

TEST_SUITE("world+grid: invariants & identity") {
    TEST_CASE("a tile must fit within a single cell (add and set_shape ENFORCE)") {
        world w = grid_world();
        // spans two cells (x 0..4 over 2-wide cells) -> rejected
        CHECK_THROWS(w.add(1, tile_body{aabb{vec{0, 0}, vec{4, 2}}, {}, {}}));
        const collider_id t = w.add(2, tile_body{aabb{vec{0, 0}, vec{2, 2}}, {}, {}}); // cell-sized
        REQUIRE(w.is_valid(t));
        CHECK_THROWS(w.set_shape(t, shape_t{aabb{vec{0, 0}, vec{4, 2}}})); // would overhang
        CHECK_NOTHROW(w.set_shape(t, shape_t{circle{vec{1, 1}, 0.5f}}));   // fits
    }

    TEST_CASE("trigger keys include type_id: BODY and TILE sharing value+gen don't alias") {
        world w = grid_world();
        w.add(100, tile_body{aabb{vec{0, 0}, vec{2, 2}}, {}, {}});                        // TILE cell 0
        w.add(200, static_body{shape_t{aabb{vec{0.2f, 0.2f}, vec{1.8f, 1.8f}}}, {}, {}}); // BODY slot 0
        material_props sens; sens.response = response_mode::SENSOR;
        w.add(300, kinematic_body{moving_shape_t{aabb{vec{0.1f, 0.1f}, vec{1.9f, 1.9f}}}, sens, {}, vec{0, 0}});

        const auto& evs = w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f);
        int begins = 0;
        bool tile_seen = false, body_seen = false;
        for (const auto& e : evs) {
            if (e.kind != event_kind::TRIGGER_BEGIN) continue;
            ++begins;
            if (e.target.type_id == collider_id::TILE && w.get_eid(e.target) == 100u) tile_seen = true;
            if (e.target.type_id == collider_id::BODY && w.get_eid(e.target) == 200u) body_seen = true;
        }
        CHECK(begins == 2);          // both pairs fire -- no aliasing despite value==0,gen==0 for both
        CHECK(tile_seen);
        CHECK(body_seen);
    }

    TEST_CASE("clear() empties the world (bodies, bullets, tiles) and is reusable") {
        world w = grid_world();
        const collider_id body = w.add(1, kinematic_body{
                                          moving_shape_t{aabb{vec{1, 1}, vec{2, 2}}}, {}, {}, vec{0, 0}});
        const collider_id tile = w.add(2, tile_body{aabb{vec{4, 4}, vec{6, 6}}, {}, {}});
        const collider_id bul = w.add(3, bullet{
                                         moving_shape_t{aabb{vec{0, 0}, vec{1, 1}}}, {}, {}, vec{1, 0}});
        REQUIRE(w.is_valid(body));
        REQUIRE(w.is_valid(tile));
        REQUIRE(w.is_valid(bul));

        w.clear();
        CHECK_FALSE(w.is_valid(body)); // all handles invalid (monotonic generations)
        CHECK_FALSE(w.is_valid(tile));
        CHECK_FALSE(w.is_valid(bul));

        // reusable, and no aliasing with the pre-clear handles even if slots/cells are reused
        const collider_id tile2 = w.add(20, tile_body{aabb{vec{4, 4}, vec{6, 6}}, {}, {}});
        CHECK(w.is_valid(tile2));
        CHECK(w.get_eid(tile2) == 20u);
        CHECK_FALSE(w.is_valid(tile));
        const auto hit = w.raycast(segment{vec{3, 5}, vec{16, 5}}); // finds the new tile only
        REQUIRE(hit.has_value());
        CHECK(hit->who.type_id == collider_id::TILE);
        CHECK(w.get_eid(hit->who) == 20u);
    }

    TEST_CASE("clear() on a pure-dynamic world (no grid) is fine") {
        world w(world_config{});
        w.add(1, static_body{shape_t{aabb{vec{0, 0}, vec{1, 1}}}, {}, {}});
        CHECK_NOTHROW(w.clear());
    }

    TEST_CASE("a TILE handle to an overwritten / removed cell goes invalid (per-cell generation)") {
        world w = grid_world();
        const collider_id a = w.add(1, tile_body{aabb{vec{0, 0}, vec{2, 2}}, {}, {}}); // cell 0
        REQUIRE(w.is_valid(a));

        // overwrite the same cell -> a is now stale, the new handle is live
        const collider_id b = w.add(2, tile_body{circle{vec{1, 1}, 0.5f}, {}, {}});
        CHECK(a.value == b.value);          // same cell
        CHECK_FALSE(w.is_valid(a));         // generation closed the alias (was: silently valid)
        CHECK(w.is_valid(b));
        CHECK(w.get_eid(b) == 2u);

        // remove then refill the cell -> b stale, the newest handle live
        w.remove(b);
        CHECK_FALSE(w.is_valid(b));
        const collider_id c = w.add(3, tile_body{aabb{vec{0, 0}, vec{2, 2}}, {}, {}});
        CHECK(c.value == b.value);
        CHECK_FALSE(w.is_valid(b));
        CHECK(w.is_valid(c));
        CHECK(w.get_eid(c) == 3u);
    }
}

TEST_SUITE("world+grid: grid<->BVH agreement (G4.5)") {
    // Deterministic LCG so a pass is reproducible (no Math.random-style flakiness).
    struct rng {
        uint32_t s;
        uint32_t next() { s = s * 1664525u + 1013904223u; return s; }
        float f(float lo, float hi) { return lo + (hi - lo) * (static_cast<float>(next() >> 8) / static_cast<float>(1 << 24)); }
    };

    // A scene of N isolated, cell-aligned tiles (a small box centred in distinct 2x2 cells, with
    // margin so tiles never touch -> no exact-edge ties between neighbours). Built BOTH as grid
    // tiles and as BVH static bodies, with identical eids, so the two query paths can be compared.
    struct scene {
        world tiles;   // grid path
        world bodies;  // BVH path
    };

    scene build(rng& r, int n) {
        world_config cfg;
        cfg.bounds = aabb{vec{0, 0}, vec{16, 16}};
        cfg.grid = world_config::grid_config{vec{2, 2}}; // 8x8 cells
        scene sc{world(cfg), world(world_config{})};
        std::set<uint32_t> used;
        entity_id_t eid = 1;
        int placed = 0, guard = 0;
        while (placed < n && guard++ < 1000) {
            const uint32_t cx = r.next() % 8u, cy = r.next() % 8u;
            const uint32_t cell = cy * 8u + cx;
            if (!used.insert(cell).second) continue;
            // 0.8 x 0.8 box centred in the 2x2 cell (origin 0,0): centre = (cx*2+1, cy*2+1)
            const float ox = cx * 2.0f + 1.0f, oy = cy * 2.0f + 1.0f;
            const aabb box{vec{ox - 0.4f, oy - 0.4f}, vec{ox + 0.4f, oy + 0.4f}};
            sc.tiles.add(eid, tile_body{shape_t{box}, {}, {}});
            sc.bodies.add(eid, static_body{shape_t{box}, {}, {}});
            ++eid; ++placed;
        }
        return sc;
    }

    // (eid, toi) of a raycast hit, or nullopt.
    std::optional<std::pair<entity_id_t, float>> ray_hit(const world& w, const segment& s) {
        const auto h = w.raycast(s);
        if (!h) return std::nullopt;
        return std::make_pair(w.get_eid(h->who), h->toi);
    }

    TEST_CASE("raycast agrees between the grid and the BVH over random scenes/rays") {
        rng r{0xC0FFEEu};
        int compared = 0, hits = 0;
        for (int trial = 0; trial < 300; ++trial) {
            scene sc = build(r, 10);
            for (int q = 0; q < 5; ++q) {
                const segment s{vec{r.f(0.1f, 15.9f), r.f(0.1f, 15.9f)},
                                vec{r.f(0.1f, 15.9f), r.f(0.1f, 15.9f)}};
                const auto a = ray_hit(sc.tiles, s);
                const auto b = ray_hit(sc.bodies, s);
                REQUIRE(a.has_value() == b.has_value());
                if (a) {
                    CHECK(a->first == b->first);                 // same tile/body eid
                    CHECK(a->second == doctest::Approx(b->second).epsilon(0.001)); // same toi
                    ++hits;
                }
                ++compared;
            }
        }
        CHECK(compared == 1500);
        CHECK(hits > 100); // sanity: the rays actually hit things often enough to be meaningful
    }

    // (eid, toi) of a swept cast, or nullopt.
    std::optional<std::pair<entity_id_t, float>> cast_hit(const world& w, const aabb& m, vec d) {
        const auto h = w.cast(moving_shape_t{m}, d);
        if (!h) return std::nullopt;
        return std::make_pair(w.get_eid(h->who), h->toi);
    }

    TEST_CASE("swept cast agrees between the grid and the BVH over random scenes/sweeps") {
        rng r{0x1234567u};
        int compared = 0, hits = 0;
        for (int trial = 0; trial < 300; ++trial) {
            scene sc = build(r, 10);
            for (int q = 0; q < 5; ++q) {
                const float ox = r.f(0.2f, 15.0f), oy = r.f(0.2f, 15.0f);
                const aabb mover{vec{ox, oy}, vec{ox + 0.3f, oy + 0.3f}}; // small mover
                const vec delta{r.f(-12.0f, 12.0f), r.f(-12.0f, 12.0f)};
                const auto a = cast_hit(sc.tiles, mover, delta);
                const auto b = cast_hit(sc.bodies, mover, delta);
                REQUIRE(a.has_value() == b.has_value());
                if (a) {
                    CHECK(a->first == b->first);
                    CHECK(a->second == doctest::Approx(b->second).epsilon(0.001));
                    ++hits;
                }
                ++compared;
            }
        }
        CHECK(compared == 1500);
        CHECK(hits > 100);
    }
}

TEST_SUITE("world+grid: triangle (solid slope) tiles") {
    // A solid slope filling the lower-left of cell (2,2) = [4,6]^2: legs on the bottom (y=4) and
    // left (x=4), hypotenuse (6,4)-(4,6). A segment-only slope is just that hypotenuse.
    static const triangle SLOPE{vec{4, 4}, vec{6, 4}, vec{4, 6}};
    static const segment  DIAG{vec{6, 4}, vec{4, 6}};

    TEST_CASE("a triangle slope tile is hit by raycast / cast and reports its handle") {
        world w = grid_world();
        w.add(1, tile_body{shape_t{SLOPE}, {}, {}});
        const auto r = w.raycast(segment{vec{0, 4.5f}, vec{16, 4.5f}}); // ray crossing the slope
        REQUIRE(r.has_value());
        CHECK(r->who.type_id == collider_id::TILE);
        CHECK(w.get_eid(r->who) == 1u);
    }

    TEST_CASE("a solid slope blocks the open bottom face that a segment-only slope leaks") {
        // mover rising into the slope from BELOW its bottom leg (y=4).
        auto rise_into = [](const shape_t& slope) {
            world w = grid_world();
            material_props m; m.response = response_mode::BLOCK; m.block_normal = vec{0, -1};
            w.add(1, tile_body{slope, m, {}});
            const collider_id b = w.add(2, kinematic_body{
                                           moving_shape_t{aabb{vec{4.4f, 2.0f}, vec{4.9f, 2.5f}}}, {}, {}, vec{0, 240}});
            (void)w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f); // dy = +4
            return std::visit([](const auto& s) { return enclose(s); }, w.get_shape(b)).max.y();
        };
        const float top_triangle = rise_into(shape_t{SLOPE});
        const float top_segment  = rise_into(shape_t{DIAG});
        CHECK(top_triangle < 4.1f); // solid slope: stopped at the bottom leg y=4
        CHECK(top_segment  > 4.5f); // segment-only: leaked up into the solid, only the diagonal stops it
    }

    TEST_CASE("a solid slope blocks a mover approaching from every side (no leak)") {
        auto blocked_from = [](vec start_min, vec start_max, vec vel) {
            world w = grid_world();
            material_props m; m.response = response_mode::BLOCK;
            w.add(1, tile_body{shape_t{SLOPE}, m, {}});
            const aabb mover{start_min, start_max};
            // a transient swept cast against the world: a hit means the slope blocks this approach
            const auto h = w.cast(moving_shape_t{mover}, vel);
            return h.has_value() && h->who.type_id == collider_id::TILE;
        };
        CHECK(blocked_from(vec{1.0f, 4.4f}, vec{1.5f, 4.9f}, vec{6, 0}));   // from the left
        CHECK(blocked_from(vec{4.4f, 1.0f}, vec{4.9f, 1.5f}, vec{0, 6}));   // from below
        CHECK(blocked_from(vec{7.0f, 7.0f}, vec{7.5f, 7.5f}, vec{-6, -6})); // from the upper-right (hypotenuse)
    }
}

TEST_SUITE("world+grid: tile boundary compilation (§19 #4)") {
    TEST_CASE("mergeable solid tiles compile to merged residents on the first run; floor stays solid") {
        world w = grid_world(); // 8x8 cells of 2x2 over [0,16]^2
        material_props m; m.response = response_mode::BLOCK; m.block_normal = vec{0, 1};
        std::vector<collider_id> tiles;
        for (uint32_t cx = 0; cx < 8; ++cx) {
            const float x0 = cx * 2.0f;
            tiles.push_back(w.add(100 + cx, tile_body{shape_t{aabb{vec{x0, 0}, vec{x0 + 2, 2}}}, m, {}, true}));
        }
        for (const auto t : tiles) CHECK(w.is_valid(t)); // valid as TILEs before compile

        (void)w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f); // first run compiles

        for (const auto t : tiles) CHECK_FALSE(w.is_valid(t)); // merged away (handles invalidated)
        // the floor still collides, now as merged BODY geometry, across the whole row
        const auto a = w.raycast(segment{vec{5, 8}, vec{5, -1}});
        REQUIRE(a.has_value());
        CHECK(a->who.type_id == collider_id::BODY);
        CHECK(w.raycast(segment{vec{15, 8}, vec{15, -1}}).has_value());
    }

    TEST_CASE("the bake is one-shot: mergeable tiles are rejected after the first run; clear() re-enables it") {
        world w = grid_world();
        w.add(1, tile_body{shape_t{aabb{vec{0, 0}, vec{2, 2}}}, {}, {}, true});
        (void)w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f); // bake
        CHECK_THROWS(w.add(2, tile_body{shape_t{aabb{vec{2, 0}, vec{4, 2}}}, {}, {}, true})); // post-bake mergeable
        CHECK_NOTHROW(w.add(3, tile_body{shape_t{aabb{vec{8, 8}, vec{10, 10}}}, {}, {}, false})); // non-mergeable ok
        w.clear();
        CHECK_NOTHROW(w.add(4, tile_body{shape_t{aabb{vec{0, 0}, vec{2, 2}}}, {}, {}, true})); // bake reset
    }

    TEST_CASE("a mergeable tile that survived the bake (slope) cannot be reshaped into static AABB geometry") {
        world w = grid_world();
        // mergeable=true but a segment -> not an aabb -> survives the bake in the grid
        const collider_id slope = w.add(1, tile_body{shape_t{segment{vec{4, 4}, vec{6, 6}}}, {}, {}, true});
        (void)w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f);
        REQUIRE(w.is_valid(slope));
        // reshaping it post-bake (into anything) is rejected -- it would smuggle un-baked static geometry
        CHECK_THROWS(w.set_shape(slope, shape_t{aabb{vec{4, 4}, vec{6, 6}}}));
    }

    TEST_CASE("non-mergeable tiles and slopes are left in the grid (keep their TILE handles)") {
        world w = grid_world();
        const collider_id plain = w.add(1, tile_body{shape_t{aabb{vec{0, 0}, vec{2, 2}}}, {}, {}, false});
        const collider_id slope = w.add(2, tile_body{shape_t{segment{vec{4, 4}, vec{6, 6}}}, {}, {}, true}); // mergeable but not an aabb
        (void)w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f);
        CHECK(w.is_valid(plain));
        CHECK(plain.type_id == collider_id::TILE);
        CHECK(w.is_valid(slope)); // a segment can't merge into an aabb -> kept
        CHECK(slope.type_id == collider_id::TILE);
    }

    TEST_CASE("a destructible (non-mergeable) tile still reports per-tile hits after compilation") {
        world w = grid_world();
        material_props block; block.response = response_mode::BLOCK;
        // a seamless floor (mergeable) plus one addressable brick (not mergeable) sitting on it
        for (uint32_t cx = 0; cx < 4; ++cx)
            w.add(10 + cx, tile_body{shape_t{aabb{vec{cx * 2.0f, 0}, vec{cx * 2.0f + 2, 2}}}, block, {}, true});
        const collider_id brick = w.add(99, tile_body{shape_t{aabb{vec{4, 2}, vec{6, 4}}}, block, {}, false});
        (void)w.run(aabb{vec{0, 0}, vec{16, 16}}, 1.0f / 60.0f);
        // the brick is still a live TILE and a ray into it reports its eid
        REQUIRE(w.is_valid(brick));
        const auto hit = w.raycast(segment{vec{0, 3}, vec{16, 3}}); // at y=3, through the brick row
        REQUIRE(hit.has_value());
        CHECK(hit->who.type_id == collider_id::TILE);
        CHECK(w.get_eid(hit->who) == 99u);
    }
}
