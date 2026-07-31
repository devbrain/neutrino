//
// Tests for neutrino::input_snapshot -- the frame-scoped input value handed to
// base_scene::fixed_update. The rule under test: one frame's snapshot may be delivered to
// several fixed substeps, so EDGES must reach exactly one of them while HELD reaches all.
//
#include <doctest/doctest.h>

#include <neutrino/input/input_snapshot.hh>

#include "input/edge_gate.hh" // internal: the same rule for the POLLED input APIs

#include <stdexcept>

using namespace neutrino;

namespace {
    button_state down_edge() { return button_state{true, false, true}; }   // pressed this frame
    button_state up_edge() { return button_state{false, true, false}; }    // released this frame
} // namespace

TEST_SUITE("neutrino::input input_snapshot") {
    TEST_CASE("without_edges keeps held and the pointer but drops the transitions") {
        const pointer_state p{{12.0f, 34.0f}, {6.0f, 17.0f}, true};
        const input_snapshot in{p, down_edge(), up_edge(), button_state{}};

        const input_snapshot rest = in.without_edges();

        SUBCASE("edges are gone") {
            CHECK_FALSE(rest.mouse(sdlpp::mouse_button::left).pressed);
            CHECK_FALSE(rest.mouse(sdlpp::mouse_button::middle).released);
        }

        SUBCASE("held survives -- it is a state, not a transition") {
            REQUIRE(in.mouse(sdlpp::mouse_button::left).held);
            CHECK(rest.mouse(sdlpp::mouse_button::left).held);
            CHECK_FALSE(rest.mouse(sdlpp::mouse_button::middle).held); // was up already
        }

        SUBCASE("the pointer is untouched") {
            CHECK(rest.pointer().render.x == doctest::Approx(6.0f));
            CHECK(rest.pointer().render.y == doctest::Approx(17.0f));
            CHECK(rest.pointer().on_screen);
        }

        SUBCASE("the original is unchanged (the copy is what gets degraded)") {
            CHECK(in.mouse(sdlpp::mouse_button::left).pressed);
            CHECK(in.mouse(sdlpp::mouse_button::middle).released);
        }
    }

    // The application hands substep 0 the full snapshot and every later substep the
    // without_edges copy. At the default 120 Hz sim on a 60 Hz display that is 2 substeps per
    // frame -- without this split a single click would fire a one-shot action twice.
    TEST_CASE("a press is observed exactly once across a multi-substep frame") {
        const input_snapshot in{pointer_state{}, down_edge(), button_state{}, button_state{}};
        const input_snapshot held_only = in.without_edges();

        int fired = 0;
        int held_seen = 0;
        constexpr int substeps = 4;
        for (int step = 0; step < substeps; ++step) {
            const input_snapshot& s = (step == 0) ? in : held_only;
            if (s.mouse(sdlpp::mouse_button::left).pressed) {
                ++fired;
            }
            if (s.mouse(sdlpp::mouse_button::left).held) {
                ++held_seen;
            }
        }
        CHECK(fired == 1);                 // the one-shot action runs once per physical press
        CHECK(held_seen == substeps);      // continuous state is visible to every substep
    }

    // The snapshot is only half the story: hotkey/mouse_click/gamepad_button poll SDL's per-frame
    // transients, which are cleared once per FRAME (after rendering), not once per substep. The
    // application raises the same one-substep rule for them via the edge gate -- otherwise a scene
    // polling hotkey::pressed() in fixed_update (map_viewer, sprite_demo) fires twice per press at
    // the default 120 Hz sim on a 60 Hz display.
    TEST_CASE("the edge gate masks polled transitions but never held") {
        REQUIRE_FALSE(input_detail::edges_suppressed()); // default: substep 0 semantics

        const sdlpp::button_state press{true, false, true};   // pressed + held
        const sdlpp::button_state release{false, true, false}; // released

        SUBCASE("ungated, transitions pass through") {
            CHECK(input_detail::gate_edges(press).pressed);
            CHECK(input_detail::gate_edges(release).released);
        }

        SUBCASE("gated, transitions are masked and held survives") {
            const input_detail::edge_suppression gate(true);
            CHECK(input_detail::edges_suppressed());
            CHECK_FALSE(input_detail::gate_edges(press).pressed);
            CHECK_FALSE(input_detail::gate_edges(release).released);
            CHECK(input_detail::gate_edges(press).held); // state, not transition
        }

        SUBCASE("the scope guard restores the previous value") {
            {
                const input_detail::edge_suppression gate(true);
                REQUIRE(input_detail::edges_suppressed());
            }
            CHECK_FALSE(input_detail::edges_suppressed());
        }

        // A throwing fixed_update must not leave later frames with edges masked forever.
        SUBCASE("the scope guard unwinds on an exception") {
            try {
                const input_detail::edge_suppression gate(true);
                throw std::runtime_error("scene blew up mid-substep");
            } catch (const std::runtime_error&) {
                // swallowed, as application's fixed-step loop does
            }
            CHECK_FALSE(input_detail::edges_suppressed());
        }

        CHECK_FALSE(input_detail::edges_suppressed()); // no leakage between test cases
    }

    TEST_CASE("a default snapshot reports nothing pressed and an off-screen pointer") {
        const input_snapshot in;
        CHECK_FALSE(in.pointer().on_screen); // so a scene will not steer from {0,0}
        CHECK_FALSE(in.mouse(sdlpp::mouse_button::left).held);
        CHECK_FALSE(in.mouse(sdlpp::mouse_button::right).pressed);
        // Buttons with no tracked state (x1/x2) read as "nothing", not as garbage.
        CHECK_FALSE(in.mouse(sdlpp::mouse_button::x1).held);
    }
}
