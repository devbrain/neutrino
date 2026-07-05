//
// Created by igor on 28/06/2026.
//
// Swept / crossing triggers (cross-cutting roadmap). run()'s trigger pass diffs SENSOR overlaps
// at frame BOUNDARIES, so a fast body can skip a thin pickup / checkpoint / tripwire between two
// sampled frames without ever overlapping it at an endpoint. world::swept_triggers sweeps a shape
// by a delta and reports every SENSOR it touches along the way -- including ones it passes
// entirely through -- ordered nearest-first by toi. Built on the swept cast; sensors only.
//
#include <doctest/doctest.h>

#include <variant>
#include <vector>

#include <neutrino/physics/collide/world.hh>

using namespace neutrino::physics;

namespace {
    world st_world() {
        world_config cfg;
        cfg.bounds = aabb{vec{0, 0}, vec{64, 64}};
        cfg.up = {0, 1};
        return world(cfg);
    }

    material_props sensor_mat() {
        material_props m;
        m.response = response_mode::SENSOR;
        return m;
    }

    collider_id add_sensor(world& w, entity_id_t eid, const aabb& box, filter_props f = {}) {
        return w.add(eid, static_body{shape_t{box}, sensor_mat(), f});
    }

    collider_id add_solid(world& w, entity_id_t eid, const aabb& box) {
        return w.add(eid, static_body{shape_t{box}, {}, {}});
    }

    int begins_for(const std::vector<world_event>& evs, const world& w, entity_id_t sensor_eid) {
        int n = 0;
        for (const auto& e : evs) {
            if (e.kind == event_kind::TRIGGER_BEGIN && w.get_eid(e.mover) == sensor_eid) ++n;
        }
        return n;
    }
}

TEST_SUITE("world: swept_triggers (crossing triggers)") {
    TEST_CASE("a fast crossing the frame-boundary diff misses is caught by swept_triggers") {
        world w = st_world();
        add_sensor(w, 1, aabb{vec{5, 0}, vec{5.2f, 4}}); // a thin tripwire
        const moving_shape_t start{aabb{vec{0, 1}, vec{1, 2}}};
        w.add(2, kinematic_body{start, {}, {}, vec{600, 0}}); // crosses the wire within one dt
        const vec delta{600.0f / 60.0f, 0.0f};                // = {10, 0}; ends at x[10,11], past the wire

        const auto& evs = w.run(aabb{vec{0, 0}, vec{64, 64}}, 1.0f / 60.0f);
        CHECK(begins_for(evs, w, 1u) == 0); // the static frame-boundary diff missed the crossing

        const auto crossed = w.swept_triggers(start, delta);
        REQUIRE(crossed.size() == 1); // ...but the swept query catches it
        CHECK(w.get_eid(crossed[0].who) == 1u);
        CHECK(crossed[0].toi > 0.0f);
        CHECK(crossed[0].toi < 1.0f); // crossed somewhere mid-sweep
    }

    TEST_CASE("a sensor only at the end of the sweep is reported") {
        world w = st_world();
        add_sensor(w, 1, aabb{vec{9, 0}, vec{11, 4}});
        const auto crossed = w.swept_triggers(moving_shape_t{aabb{vec{0, 1}, vec{1, 2}}}, vec{10, 0});
        REQUIRE(crossed.size() == 1);
        CHECK(w.get_eid(crossed[0].who) == 1u);
    }

    TEST_CASE("a sensor already overlapped at the start reads as toi 0") {
        world w = st_world();
        add_sensor(w, 1, aabb{vec{0, 0}, vec{2, 4}}); // mover starts inside it
        const auto crossed = w.swept_triggers(moving_shape_t{aabb{vec{0, 1}, vec{1, 2}}}, vec{10, 0});
        REQUIRE(crossed.size() == 1);
        CHECK(crossed[0].toi == doctest::Approx(0.0f));
    }

    TEST_CASE("solids are not reported (triggers are sensors only)") {
        world w = st_world();
        add_solid(w, 1, aabb{vec{5, 0}, vec{5.2f, 4}});   // a solid wall in the path
        add_sensor(w, 2, aabb{vec{7, 0}, vec{7.2f, 4}});  // a sensor further along
        const auto crossed = w.swept_triggers(moving_shape_t{aabb{vec{0, 1}, vec{1, 2}}}, vec{10, 0});
        REQUIRE(crossed.size() == 1);
        CHECK(w.get_eid(crossed[0].who) == 2u); // only the sensor
    }

    TEST_CASE("multiple crossed sensors are ordered nearest-first") {
        world w = st_world();
        add_sensor(w, 1, aabb{vec{8, 0}, vec{8.2f, 4}});  // farther
        add_sensor(w, 2, aabb{vec{3, 0}, vec{3.2f, 4}});  // nearer
        add_sensor(w, 3, aabb{vec{5, 0}, vec{5.2f, 4}});  // middle
        const auto crossed = w.swept_triggers(moving_shape_t{aabb{vec{0, 1}, vec{1, 2}}}, vec{12, 0});
        REQUIRE(crossed.size() == 3);
        CHECK(w.get_eid(crossed[0].who) == 2u);
        CHECK(w.get_eid(crossed[1].who) == 3u);
        CHECK(w.get_eid(crossed[2].who) == 1u);
        CHECK(crossed[0].toi <= crossed[1].toi);
        CHECK(crossed[1].toi <= crossed[2].toi);
    }

    TEST_CASE("filter excludes non-matching sensors") {
        world w = st_world();
        filter_props team_a;
        team_a.category = 0x0001;
        team_a.mask = 0x0001;
        filter_props team_b;
        team_b.category = 0x0002;
        team_b.mask = 0x0002;
        add_sensor(w, 1, aabb{vec{3, 0}, vec{3.2f, 4}}, team_a);
        add_sensor(w, 2, aabb{vec{5, 0}, vec{5.2f, 4}}, team_b);
        const auto crossed = w.swept_triggers(moving_shape_t{aabb{vec{0, 1}, vec{1, 2}}}, vec{10, 0}, team_a);
        REQUIRE(crossed.size() == 1);
        CHECK(w.get_eid(crossed[0].who) == 1u);
    }

    TEST_CASE("no sensor in the swept band -> empty") {
        world w = st_world();
        add_sensor(w, 1, aabb{vec{5, 10}, vec{5.2f, 14}}); // off the sweep line (high up)
        const auto crossed = w.swept_triggers(moving_shape_t{aabb{vec{0, 1}, vec{1, 2}}}, vec{10, 0});
        CHECK(crossed.empty());
    }
}
