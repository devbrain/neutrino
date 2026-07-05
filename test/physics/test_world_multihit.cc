//
// Created by igor on 28/06/2026.
//
// Multi-hit queries (cross-cutting roadmap). raycast / cast return only the NEAREST hit;
// raycast_all / cast_all return EVERY collider crossed, ordered nearest-first by toi, with an
// optional max-hit cap. Covers beams, piercing shots, melee/sword arcs, explosion sweeps and
// boss multi-hurtbox scans -- across both dynamic residents and static tiles.
//
#include <doctest/doctest.h>

#include <variant>
#include <vector>

#include <neutrino/physics/collide/world.hh>

using namespace neutrino::physics;


namespace {
    world mh_world() {
        world_config cfg;
        cfg.bounds = aabb{vec{0, 0}, vec{64, 64}};
        cfg.up = {0, 1};
        return world(cfg);
    }

    // a grid world so we can mix tiles into the line of fire
    world mh_grid_world() {
        world_config cfg;
        cfg.bounds = aabb{vec{0, 0}, vec{16, 16}};
        cfg.grid = world_config::grid_config{vec{2, 2}};
        return world(cfg);
    }
}

TEST_SUITE("world: raycast_all / cast_all (multi-hit)") {
    TEST_CASE("raycast_all returns every body crossed, ordered nearest-first") {
        world w = mh_world();
        w.add(10, static_body{shape_t{aabb{vec{8, 0}, vec{9, 2}}}, {}, {}});  // farthest
        w.add(20, static_body{shape_t{aabb{vec{2, 0}, vec{3, 2}}}, {}, {}});  // nearest
        w.add(30, static_body{shape_t{aabb{vec{5, 0}, vec{6, 2}}}, {}, {}});  // middle

        const auto hits = w.raycast_all(segment{vec{0, 1}, vec{16, 1}});
        REQUIRE(hits.size() == 3);
        CHECK(w.get_eid(hits[0].who) == 20u); // x=2
        CHECK(w.get_eid(hits[1].who) == 30u); // x=5
        CHECK(w.get_eid(hits[2].who) == 10u); // x=8
        CHECK(hits[0].toi < hits[1].toi);
        CHECK(hits[1].toi < hits[2].toi);
    }

    TEST_CASE("max_hits keeps only the nearest N") {
        world w = mh_world();
        w.add(10, static_body{shape_t{aabb{vec{8, 0}, vec{9, 2}}}, {}, {}});
        w.add(20, static_body{shape_t{aabb{vec{2, 0}, vec{3, 2}}}, {}, {}});
        w.add(30, static_body{shape_t{aabb{vec{5, 0}, vec{6, 2}}}, {}, {}});

        const auto hits = w.raycast_all(segment{vec{0, 1}, vec{16, 1}}, {}, 2);
        REQUIRE(hits.size() == 2);
        CHECK(w.get_eid(hits[0].who) == 20u);
        CHECK(w.get_eid(hits[1].who) == 30u);
    }

    TEST_CASE("nothing crossed -> empty") {
        world w = mh_world();
        w.add(10, static_body{shape_t{aabb{vec{2, 10}, vec{3, 12}}}, {}, {}}); // above the ray
        const auto hits = w.raycast_all(segment{vec{0, 1}, vec{16, 1}});
        CHECK(hits.empty());
    }

    TEST_CASE("raycast_all spans bodies AND tiles, merged in toi order") {
        world w = mh_grid_world();
        w.add(1, tile_body{aabb{vec{4, 0}, vec{6, 2}}, {}, {}});              // tile, cell (2,0)
        w.add(2, static_body{shape_t{aabb{vec{8, 0}, vec{10, 2}}}, {}, {}});  // body, farther
        w.add(3, tile_body{aabb{vec{12, 0}, vec{14, 2}}, {}, {}});            // tile, farthest

        const auto hits = w.raycast_all(segment{vec{0, 1}, vec{16, 1}});
        REQUIRE(hits.size() == 3);
        CHECK(hits[0].who.type_id == collider_id::TILE);
        CHECK(w.get_eid(hits[0].who) == 1u);
        CHECK(hits[1].who.type_id == collider_id::BODY);
        CHECK(w.get_eid(hits[1].who) == 2u);
        CHECK(hits[2].who.type_id == collider_id::TILE);
        CHECK(w.get_eid(hits[2].who) == 3u);
    }

    TEST_CASE("a ONE_WAY platform is still REPORTED from the wrong side (geometric, not response-gated)") {
        // raycast_all is a geometric query: it reports every filtered collider regardless of material
        // response, so the caller (a beam) decides what stops it. Contrast move_and_slide, which
        // applies the solid/one-way response.
        world w = mh_world();
        material_props oneway;
        oneway.response = response_mode::ONE_WAY;
        oneway.block_normal = {0, 1};
        w.add(5, static_body{shape_t{aabb{vec{4, 0}, vec{6, 2}}}, oneway, {}});

        const auto hits = w.raycast_all(segment{vec{5, -2}, vec{5, 8}}); // crossing from below
        REQUIRE(hits.size() == 1);
        CHECK(w.get_eid(hits[0].who) == 5u);
    }

    TEST_CASE("filter excludes non-matching colliders") {
        world w = mh_world();
        filter_props team_a;
        team_a.category = 0x0001;
        team_a.mask = 0x0001;
        filter_props team_b;
        team_b.category = 0x0002;
        team_b.mask = 0x0002;
        w.add(10, static_body{shape_t{aabb{vec{2, 0}, vec{3, 2}}}, {}, team_a});
        w.add(20, static_body{shape_t{aabb{vec{5, 0}, vec{6, 2}}}, {}, team_b});

        const auto hits = w.raycast_all(segment{vec{0, 1}, vec{16, 1}}, team_a);
        REQUIRE(hits.size() == 1);
        CHECK(w.get_eid(hits[0].who) == 10u);
    }

    TEST_CASE("cast_all sweeps a shape through every collider, nearest-first") {
        world w = mh_world();
        w.add(10, static_body{shape_t{aabb{vec{10, 0}, vec{11, 4}}}, {}, {}}); // farther
        w.add(20, static_body{shape_t{aabb{vec{4, 0}, vec{5, 4}}}, {}, {}});   // nearer

        // sweep a 1x1 box rightward across both
        const auto hits = w.cast_all(moving_shape_t{aabb{vec{0, 1}, vec{1, 2}}}, vec{16, 0});
        REQUIRE(hits.size() == 2);
        CHECK(w.get_eid(hits[0].who) == 20u);
        CHECK(w.get_eid(hits[1].who) == 10u);
        CHECK(hits[0].toi < hits[1].toi);
    }

    TEST_CASE("cast_all respects max_hits") {
        world w = mh_world();
        w.add(10, static_body{shape_t{aabb{vec{10, 0}, vec{11, 4}}}, {}, {}});
        w.add(20, static_body{shape_t{aabb{vec{4, 0}, vec{5, 4}}}, {}, {}});

        const auto hits = w.cast_all(moving_shape_t{aabb{vec{0, 1}, vec{1, 2}}}, vec{16, 0}, {}, 1);
        REQUIRE(hits.size() == 1);
        CHECK(w.get_eid(hits[0].who) == 20u); // the nearer one
    }
}
