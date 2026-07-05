//
// Created by igor on 25/06/2026.
//
// Moving platforms / carriers (§19 #1, MP1 — carrying). A carrier_body is a kinematic "solid"
// that moves rigidly on a scripted path and transports the actors riding it. Each frame it
// translates by velocity*dt; a rider on top inherits (velocity + surface_velocity)*dt:
//   moving platform -> surface_velocity {0,0}; conveyor -> velocity {0,0}, surface = belt speed.
// MP1 covers carrying only; pushing/crushing are follow-ups.
//
#include <doctest/doctest.h>

#include <variant>
#include <vector>

#include <neutrino/physics/collide/world.hh>

using namespace neutrino::physics;


namespace {
    world carrier_world() {
        world_config cfg;
        cfg.bounds = aabb{vec{0, 0}, vec{64, 64}};
        cfg.up = {0, 1};
        cfg.skin = 0.01f;
        return world(cfg);
    }

    aabb box_of(const world& w, collider_id id) {
        return std::visit([](const auto& s) { return enclose(s); }, w.get_shape(id));
    }

    // a 1x1 rider resting on top of a carrier whose top is at y (skin gap above it)
    collider_id add_rider(world& w, entity_id_t eid, float x, float top) {
        return w.add(eid, kinematic_body{
                         moving_shape_t{aabb{vec{x, top + 0.01f}, vec{x + 1.0f, top + 1.01f}}}, {}, {}, vec{0, 0}});
    }

    int crush_count(const std::vector<world_event>& evs) {
        int n = 0;
        for (const auto& e : evs) if (e.kind == event_kind::CRUSH) ++n;
        return n;
    }

    constexpr float DT = 1.0f / 60.0f;
}

TEST_SUITE("world: carriers (moving platforms / conveyors)") {
    TEST_CASE("a horizontal moving platform carries its rider") {
        world w = carrier_world();
        const collider_id plat = w.add(1, carrier_body{
                                           moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{60, 0}, vec{0, 0}});
        const collider_id rider = add_rider(w, 2, 12, 10); // on the platform top (y=10)
        const float rx0 = box_of(w, rider).min.x();

        (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);   // dx = +1

        CHECK(box_of(w, rider).min.x() == doctest::Approx(rx0 + 1.0f).epsilon(0.05));
        CHECK(box_of(w, plat).min.x() == doctest::Approx(11.0f).epsilon(0.05)); // platform moved too
    }

    TEST_CASE("a rider in EXACT top contact (zero gap) is still carried") {
        world w = carrier_world();
        w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{60, 0}, vec{0, 0}});
        // bottom == carrier top (y=10), no skin gap -- a spawn/teleport placed flush on top
        const collider_id rider = w.add(2, kinematic_body{
                                            moving_shape_t{aabb{vec{12, 10.0f}, vec{13, 11.0f}}}, {}, {}, vec{0, 0}});
        const float rx0 = box_of(w, rider).min.x();
        const auto& evs = w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);
        CHECK(box_of(w, rider).min.x() == doctest::Approx(rx0 + 1.0f).epsilon(0.05));
        CHECK(crush_count(evs) == 0); // a flush (zero-gap) rider rides; mere contact is NOT a crush
    }

    TEST_CASE("a vertical elevator carries its rider up") {
        world w = carrier_world();
        w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{0, 60}, vec{0, 0}});
        const collider_id rider = add_rider(w, 2, 12, 10);
        const float ry0 = box_of(w, rider).min.y();

        (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);   // dy = +1

        CHECK(box_of(w, rider).min.y() == doctest::Approx(ry0 + 1.0f).epsilon(0.05));
    }

    TEST_CASE("a conveyor drags its rider but does not move itself") {
        world w = carrier_world();
        const collider_id belt = w.add(1, carrier_body{
                                           moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{0, 0}, vec{120, 0}});
        const collider_id rider = add_rider(w, 2, 12, 10);
        const float rx0 = box_of(w, rider).min.x();
        const float bx0 = box_of(w, belt).min.x();

        (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);   // drag = 120*dt = +2

        CHECK(box_of(w, rider).min.x() == doctest::Approx(rx0 + 2.0f).epsilon(0.05));
        CHECK(box_of(w, belt).min.x() == doctest::Approx(bx0)); // belt itself stays put
    }

    TEST_CASE("a moving conveyor carries AND drags (the sum)") {
        world w = carrier_world();
        // platform moves +1/step, belt drags another +2/step -> rider moves +3
        w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{60, 0}, vec{120, 0}});
        const collider_id rider = add_rider(w, 2, 12, 10);
        const float rx0 = box_of(w, rider).min.x();

        (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);

        CHECK(box_of(w, rider).min.x() == doctest::Approx(rx0 + 3.0f).epsilon(0.05));
    }

    TEST_CASE("a circle rider sitting on a carrier top is carried") {
        world w = carrier_world();
        w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{60, 0}, vec{0, 0}});
        // circle centred over the top, bottom (center.y - r) just above the carrier top (y=10)
        const collider_id rider = w.add(2, kinematic_body{
                                            moving_shape_t{circle{vec{13, 11.0f}, 1.0f}}, {}, {}, vec{0, 0}});
        const float cx0 = box_of(w, rider).center().x();
        (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);
        CHECK(box_of(w, rider).center().x() == doctest::Approx(cx0 + 1.0f).epsilon(0.05));
    }

    TEST_CASE("an AABB touching only the carrier's top corner (no real support) is not a rider") {
        world w = carrier_world();
        // a CONVEYOR (no body motion -> no MP2 push) isolates the carry/ride decision: a corner
        // toucher must not be dragged by the belt, since it isn't a rider.
        w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{0, 0}, vec{120, 0}});
        // bottom-left at the carrier's top-right corner (16,10): zero perpendicular overlap.
        const collider_id corner = w.add(2, kinematic_body{
                                             moving_shape_t{aabb{vec{16, 10}, vec{17, 11}}}, {}, {}, vec{0, 0}});
        const float cx0 = box_of(w, corner).min.x();
        (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);
        CHECK(box_of(w, corner).min.x() == doctest::Approx(cx0)); // edge/corner touch is not support
    }

    TEST_CASE("a circle merely near a carrier CORNER (bbox overlaps, circle doesn't touch) is not a rider") {
        world w = carrier_world();
        // conveyor again, to test the ride decision without MP2 push.
        w.add(1, carrier_body{moving_shape_t{aabb{vec{0, -1}, vec{1, 0}}}, {}, {}, vec{0, 0}, vec{120, 0}});
        // circle at (1.9,1) r1: bbox bottom y=0, x-span [0.9,2.9] overlaps the carrier in x, but the
        // circle is ~1.33 from the corner -> not touching, must not be dragged.
        const collider_id rider = w.add(2, kinematic_body{
                                            moving_shape_t{circle{vec{1.9f, 1.0f}, 1.0f}}, {}, {}, vec{0, 0}});
        const float cx0 = box_of(w, rider).center().x();
        (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);
        CHECK(box_of(w, rider).center().x() == doctest::Approx(cx0)); // bbox lied; shape-aware says no
    }

    TEST_CASE("a body that is not riding is not carried") {
        world w = carrier_world();
        w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{60, 0}, vec{0, 0}});
        const collider_id other = w.add(2, kinematic_body{
                                            moving_shape_t{aabb{vec{40, 40}, vec{41, 41}}}, {}, {}, vec{0, 0}});
        const float ox0 = box_of(w, other).min.x();

        (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);

        CHECK(box_of(w, other).min.x() == doctest::Approx(ox0)); // untouched
    }

    TEST_CASE("a rider carried into a wall stops, doesn't tunnel (MP1: blocked, no crush yet)") {
        world w = carrier_world();
        w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{600, 0}, vec{0, 0}}); // dx=+10
        w.add(3, static_body{shape_t{aabb{vec{14, 10}, vec{15, 14}}}, {}, {}});   // wall above-right of the platform
        const collider_id rider = add_rider(w, 2, 12, 10);

        (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);

        CHECK(box_of(w, rider).max.x() <= 14.0f + 0.05f); // stopped at the wall's left face, not through it
    }

    TEST_CASE("a body touching the carrier's SIDE is not carried (contact direction)") {
        world w = carrier_world();
        w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 12}}}, {}, {}, vec{60, 0}, vec{0, 0}});
        // body flush against the LEFT side of the carrier (x meets at 10), not on top
        const collider_id sider = w.add(2, kinematic_body{
                                            moving_shape_t{aabb{vec{9.0f, 9.0f}, vec{10.0f, 11.0f}}}, {}, {}, vec{0, 0}});
        const float sx0 = box_of(w, sider).min.x();
        (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);
        CHECK(box_of(w, sider).min.x() == doctest::Approx(sx0)); // side contact is not "riding"
    }

    TEST_CASE("a rider whose filter rejects the carrier is not carried") {
        world w = carrier_world();
        filter_props cf; cf.category = 0x0002; cf.mask = 0xFFFF;     // carrier in category 2
        w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, cf, vec{60, 0}, vec{0, 0}});
        filter_props rf; rf.category = 0xFFFF; rf.mask = 0x0001;     // rider masks OUT category 2
        const collider_id rider = w.add(2, kinematic_body{
                                            moving_shape_t{aabb{vec{12, 10.01f}, vec{13, 11.01f}}}, {}, rf, vec{0, 0}});
        const float rx0 = box_of(w, rider).min.x();
        (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);
        CHECK(box_of(w, rider).min.x() == doctest::Approx(rx0)); // filtered out -> not carried
    }

    TEST_CASE("a SENSOR carrier (non-solid) carries nothing") {
        world w = carrier_world();
        material_props sens; sens.response = response_mode::SENSOR;
        w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, sens, {}, vec{60, 0}, vec{0, 0}});
        const collider_id rider = add_rider(w, 2, 12, 10);
        const float rx0 = box_of(w, rider).min.x();
        (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);
        CHECK(box_of(w, rider).min.x() == doctest::Approx(rx0)); // non-solid carrier -> no carry
    }

    TEST_CASE("set_shape on a carrier rejects a non-mover shape (segment / triangle)") {
        world w = carrier_world();
        const collider_id c = w.add(1, carrier_body{
                                        moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{0, 0}, vec{0, 0}});
        CHECK_THROWS(w.set_shape(c, shape_t{segment{vec{10, 8}, vec{16, 10}}}));
        CHECK_THROWS(w.set_shape(c, shape_t{triangle{vec{10, 8}, vec{16, 8}, vec{10, 10}}}));
        CHECK_NOTHROW(w.set_shape(c, shape_t{circle{vec{13, 9}, 1.0f}})); // a mover shape is fine
    }

    // --- MP2: pushing (a carrier moving into an actor displaces it along its motion) ---

    TEST_CASE("a carrier moving into an actor pushes it forward, clear of the carrier") {
        world w = carrier_world();
        w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{60, 0}, vec{0, 0}}); // +x
        const collider_id act = w.add(2, kinematic_body{
                                          moving_shape_t{aabb{vec{16.5f, 8}, vec{17.5f, 10}}}, {}, {}, vec{0, 0}});
        const float ax0 = box_of(w, act).min.x();
        (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT); // carrier right 16 -> 17
        CHECK(box_of(w, act).min.x() > ax0);                 // pushed forward
        CHECK(box_of(w, act).min.x() >= 17.0f - 0.001f);     // left edge clear of the carrier's right
    }

    TEST_CASE("a carrier moving AWAY from an actor does not push it") {
        world w = carrier_world();
        w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{-60, 0}, vec{0, 0}}); // -x
        const collider_id act = w.add(2, kinematic_body{
                                          moving_shape_t{aabb{vec{16.5f, 8}, vec{17.5f, 10}}}, {}, {}, vec{0, 0}});
        const float ax0 = box_of(w, act).min.x();
        (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);
        CHECK(box_of(w, act).min.x() == doctest::Approx(ax0)); // untouched
    }

    TEST_CASE("a pushed actor stops at a wall (does not tunnel; residual overlap is MP3's job)") {
        world w = carrier_world();
        w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{60, 0}, vec{0, 0}});
        w.add(3, static_body{shape_t{aabb{vec{18, 8}, vec{19, 10}}}, {}, {}}); // wall
        const collider_id act = w.add(2, kinematic_body{
                                          moving_shape_t{aabb{vec{16.5f, 8}, vec{17.5f, 10}}}, {}, {}, vec{0, 0}});
        (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);
        CHECK(box_of(w, act).max.x() <= 18.0f + 0.001f); // stopped at the wall's left face
    }

    TEST_CASE("a FAST carrier does not tunnel past a thin actor (swept-band push)") {
        world w = carrier_world();
        // carrier [10,8]-[16,10] moving right by +20/frame (vel 1200) -- it would jump well past
        // a thin actor in one step; the swept-band gate must still catch and push it.
        w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{1200, 0}, vec{0, 0}});
        const collider_id thin = w.add(2, kinematic_body{
                                           moving_shape_t{aabb{vec{20.0f, 8}, vec{20.2f, 10}}}, {}, {}, vec{0, 0}});
        const float tx0 = box_of(w, thin).min.x();
        (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT); // carrier right 16 -> 36, past x=20
        CHECK(box_of(w, thin).min.x() > tx0);              // it was pushed, not tunnelled through
        CHECK(box_of(w, thin).min.x() >= 36.0f - 0.05f);   // shoved to the carrier's final leading edge
    }

    TEST_CASE("a moving SENSOR / IGNORE carrier pushes nothing (non-solid, like the ride path)") {
        for (const response_mode resp : {response_mode::SENSOR, response_mode::IGNORE}) {
            world w = carrier_world();
            material_props m; m.response = resp;
            w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, m, {}, vec{60, 0}, vec{0, 0}});
            const collider_id act = w.add(2, kinematic_body{
                                              moving_shape_t{aabb{vec{16.5f, 8}, vec{17.5f, 10}}}, {}, {}, vec{0, 0}});
            const float ax0 = box_of(w, act).min.x();
            (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);
            CHECK(box_of(w, act).min.x() == doctest::Approx(ax0)); // non-solid carrier -> no push
        }
    }

    TEST_CASE("a one-way carrier pushes a CIRCLE only from its blocked face (oriented contact normal)") {
        // block_normal +x: the carrier is solid on its right face, so it pushes when moving +x and
        // passes through when moving -x. A circle actor exercises the aabb-vs-circle swept normal,
        // whose outward side differs from aabb-vs-aabb -- the push keys on the swept CONTACT normal
        // oriented into the carrier's motion hemisphere (which here, axis-aligned, equals the moved
        // face), not on a raw motion-direction guess.
        material_props oneway; oneway.response = response_mode::ONE_WAY; oneway.block_normal = vec{1, 0};

        SUBCASE("moving toward the blocked face (+x) pushes the circle") {
            world w = carrier_world();
            w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, oneway, {}, vec{60, 0}, vec{0, 0}});
            const collider_id act = w.add(2, kinematic_body{
                                              moving_shape_t{circle{vec{16.5f, 9}, 0.6f}}, {}, {}, vec{0, 0}});
            const float ax0 = box_of(w, act).center().x();
            (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);
            CHECK(box_of(w, act).center().x() > ax0); // blocked face leads -> pushed
        }
        SUBCASE("moving from the pass-through side (-x) does not push the circle") {
            world w = carrier_world();
            w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, oneway, {}, vec{-60, 0}, vec{0, 0}});
            const collider_id act = w.add(2, kinematic_body{
                                              moving_shape_t{circle{vec{9.5f, 9}, 0.6f}}, {}, {}, vec{0, 0}});
            const float ax0 = box_of(w, act).center().x();
            (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);
            CHECK(box_of(w, act).center().x() == doctest::Approx(ax0)); // wrong (pass-through) face -> no push
        }
    }

    TEST_CASE("a DIAGONAL one-way carrier keys on the contact face, not the motion direction") {
        // Carrier moves 45° up-right but first contacts an actor to its RIGHT on its RIGHT face.
        // The contact-face normal is {1,0}; the motion direction is ~(0.7,0.7). A motion-based
        // guess would push for BOTH block_normals (both dot > 0.5); the real contact face must
        // push only for block_normal {1,0}.
        const auto setup = [](world& w, vec block_normal) {
            material_props ow; ow.response = response_mode::ONE_WAY; ow.block_normal = block_normal;
            w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, ow, {}, vec{60, 60}, vec{0, 0}});
            return w.add(2, kinematic_body{
                              moving_shape_t{aabb{vec{16.5f, 8}, vec{17.5f, 10}}}, {}, {}, vec{0, 0}});
        };
        SUBCASE("block_normal {1,0} (the contacted face) -> pushes") {
            world w = carrier_world();
            const collider_id act = setup(w, vec{1, 0});
            const float ax0 = box_of(w, act).min.x();
            (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);
            CHECK(box_of(w, act).min.x() > ax0);
        }
        SUBCASE("block_normal {0,1} (not the contacted face) -> does NOT push") {
            world w = carrier_world();
            const collider_id act = setup(w, vec{0, 1});
            const float ax0 = box_of(w, act).min.x();
            (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);
            CHECK(box_of(w, act).min.x() == doctest::Approx(ax0)); // motion-guess would wrongly push here
        }
    }

    TEST_CASE("a conveyor (no body motion) does not push an actor beside it") {
        world w = carrier_world();
        w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{0, 0}, vec{120, 0}});
        const collider_id act = w.add(2, kinematic_body{
                                          moving_shape_t{aabb{vec{15.5f, 8}, vec{16.5f, 10}}}, {}, {}, vec{0, 0}});
        const float ax0 = box_of(w, act).min.x();
        (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);
        CHECK(box_of(w, act).min.x() == doctest::Approx(ax0)); // belt drags top riders only, never pushes
    }

    // --- MP3: crushing (a pinned actor that can't move clear -> CRUSH event) ---

    TEST_CASE("a pushed actor pinned against a wall is crushed (CRUSH names actor + carrier)") {
        world w = carrier_world();
        w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{120, 0}, vec{0, 0}});
        w.add(3, static_body{shape_t{aabb{vec{17, 8}, vec{18, 10}}}, {}, {}}); // wall close on the right
        w.add(2, kinematic_body{moving_shape_t{aabb{vec{16.5f, 8}, vec{17.0f, 10}}}, {}, {}, vec{0, 0}});
        const auto& evs = w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);
        CHECK(crush_count(evs) >= 1);
        bool named = false;
        for (const auto& e : evs)
            if (e.kind == event_kind::CRUSH)
                named = (w.get_eid(e.mover) == 2u && w.get_eid(e.target) == 1u);
        CHECK(named); // mover = the pinned actor, target = the carrier
    }

    TEST_CASE("an elevator carrying a rider into a ceiling crushes it") {
        world w = carrier_world();
        w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{0, 120}, vec{0, 0}}); // up
        w.add(3, static_body{shape_t{aabb{vec{10, 11.2f}, vec{16, 12}}}, {}, {}}); // ceiling just above
        w.add(2, kinematic_body{moving_shape_t{aabb{vec{12, 10.01f}, vec{13, 11.01f}}}, {}, {}, vec{0, 0}}); // rider
        const auto& evs = w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);
        CHECK(crush_count(evs) >= 1); // carried up, blocked by the ceiling, carrier rises into it
    }

    TEST_CASE("a clean push or carry (room to move) does not crush") {
        SUBCASE("clean push") {
            world w = carrier_world();
            w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{60, 0}, vec{0, 0}});
            w.add(2, kinematic_body{moving_shape_t{aabb{vec{16.5f, 8}, vec{17.5f, 10}}}, {}, {}, vec{0, 0}});
            CHECK(crush_count(w.run(aabb{vec{0, 0}, vec{64, 64}}, DT)) == 0);
        }
        SUBCASE("clean carry") {
            world w = carrier_world();
            w.add(1, carrier_body{moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{60, 0}, vec{0, 0}});
            w.add(2, kinematic_body{moving_shape_t{aabb{vec{12, 10.01f}, vec{13, 11.01f}}}, {}, {}, vec{0, 0}});
            CHECK(crush_count(w.run(aabb{vec{0, 0}, vec{64, 64}}, DT)) == 0);
        }
    }

    TEST_CASE("set_velocity / set_surface_velocity work on a carrier") {
        world w = carrier_world();
        const collider_id belt = w.add(1, carrier_body{
                                           moving_shape_t{aabb{vec{10, 8}, vec{16, 10}}}, {}, {}, vec{0, 0}, vec{120, 0}});
        const collider_id rider = add_rider(w, 2, 12, 10);

        w.set_surface_velocity(belt, vec{0, 0}); // switch the belt off
        const float rx0 = box_of(w, rider).min.x();
        (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);
        CHECK(box_of(w, rider).min.x() == doctest::Approx(rx0)); // no drag while off

        w.set_velocity(belt, vec{60, 0}); // now make it a moving platform
        (void)w.run(aabb{vec{0, 0}, vec{64, 64}}, DT);
        CHECK(box_of(w, rider).min.x() == doctest::Approx(rx0 + 1.0f).epsilon(0.05));
    }
}
