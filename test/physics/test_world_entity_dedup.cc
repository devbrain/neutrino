//
// Created by igor on 28/06/2026.
//
// Entity-level collider grouping (cross-cutting roadmap). One entity often owns several colliders
// by role -- hurtbox, hitbox, weak points, shield zones -- all sharing its eid, so a multi-hit
// query (raycast_all / cast_all / swept_triggers) reports it once per collider. dedup_by_entity
// collapses such results to one contact per eid, keeping the nearest collider of each, preserving
// nearest-first order.
//
#include <doctest/doctest.h>

#include <variant>
#include <vector>

#include <neutrino/physics/collide/world.hh>

using namespace neutrino::physics;


namespace {
    world ed_world() {
        world_config cfg;
        cfg.bounds = aabb{vec{0, 0}, vec{64, 64}};
        cfg.up = {0, 1};
        return world(cfg);
    }

    collider_id add_box(world& w, entity_id_t eid, const aabb& box) {
        return w.add(eid, static_body{shape_t{box}, {}, {}});
    }

    collider_id add_sensor(world& w, entity_id_t eid, const aabb& box) {
        material_props m;
        m.response = response_mode::SENSOR;
        return w.add(eid, static_body{shape_t{box}, m, {}});
    }
}

TEST_SUITE("world: dedup_by_entity (entity-level collider grouping)") {
    TEST_CASE("collapses colliders sharing an eid to the nearest one") {
        world w = ed_world();
        const collider_id near7 = add_box(w, 7, aabb{vec{2, 0}, vec{3, 2}}); // entity 7, nearest collider
        add_box(w, 7, aabb{vec{5, 0}, vec{6, 2}});                           // entity 7, farther collider
        add_box(w, 9, aabb{vec{8, 0}, vec{9, 2}});                           // entity 9

        const auto raw = w.raycast_all(segment{vec{0, 1}, vec{16, 1}});
        REQUIRE(raw.size() == 3); // three colliders before grouping

        const auto ents = w.dedup_by_entity(raw);
        REQUIRE(ents.size() == 2); // two entities after
        CHECK(w.get_eid(ents[0].who) == 7u);
        CHECK(ents[0].who == near7); // the NEAREST collider of entity 7 is the one kept
        CHECK(w.get_eid(ents[1].who) == 9u);
        CHECK(ents[0].toi < ents[1].toi); // still nearest-first
    }

    TEST_CASE("entities with distinct eids pass through unchanged") {
        world w = ed_world();
        add_box(w, 1, aabb{vec{2, 0}, vec{3, 2}});
        add_box(w, 2, aabb{vec{5, 0}, vec{6, 2}});
        add_box(w, 3, aabb{vec{8, 0}, vec{9, 2}});

        const auto ents = w.dedup_by_entity(w.raycast_all(segment{vec{0, 1}, vec{16, 1}}));
        CHECK(ents.size() == 3);
    }

    TEST_CASE("works on cast_all results") {
        world w = ed_world();
        add_box(w, 4, aabb{vec{4, 0}, vec{5, 4}});   // entity 4, two colliders along the sweep
        add_box(w, 4, aabb{vec{10, 0}, vec{11, 4}});
        add_box(w, 5, aabb{vec{7, 0}, vec{8, 4}});   // entity 5

        const auto ents = w.dedup_by_entity(
            w.cast_all(moving_shape_t{aabb{vec{0, 1}, vec{1, 2}}}, vec{16, 0}));
        REQUIRE(ents.size() == 2);
        CHECK(w.get_eid(ents[0].who) == 4u); // nearest entity first
        CHECK(w.get_eid(ents[1].who) == 5u);
    }

    TEST_CASE("works on swept_triggers results (sensors sharing an eid)") {
        world w = ed_world();
        add_sensor(w, 3, aabb{vec{3, 0}, vec{3.2f, 4}}); // one trigger entity, two sensor colliders
        add_sensor(w, 3, aabb{vec{6, 0}, vec{6.2f, 4}});

        const auto ents = w.dedup_by_entity(
            w.swept_triggers(moving_shape_t{aabb{vec{0, 1}, vec{1, 2}}}, vec{10, 0}));
        REQUIRE(ents.size() == 1);
        CHECK(w.get_eid(ents[0].who) == 3u);
    }

    TEST_CASE("empty in -> empty out") {
        world w = ed_world();
        CHECK(w.dedup_by_entity({}).empty());
    }
}
