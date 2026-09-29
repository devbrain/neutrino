//
// Tests for neutrino::input_snapshot -- the frame-scoped input value handed to
// base_scene::fixed_update. The rule under test: one frame's snapshot may be delivered to
// several fixed substeps, so EDGES must reach exactly one of them while HELD reaches all.
//
#include <doctest/doctest.h>

#include <neutrino/input/input_snapshot.hh>
#include <neutrino/input/hotkey.hh>
#include <neutrino/input/mouse_click.hh>
#include <neutrino/input/gamepad_button.hh>

#include <stdexcept>
#include <type_traits>

using namespace neutrino;

namespace {
    button_state down_edge() { return button_state{true, false, true}; }   // pressed this frame
    button_state up_edge() { return button_state{false, true, false}; }    // released this frame
} // namespace

TEST_SUITE("neutrino::input input_snapshot") {
    TEST_CASE("without_edges keeps held, axes, and the pointer but drops transitions") {
        pointer_state p{{12.0f, 34.0f}, {6.0f, 17.0f}, true};
        input_snapshot in{p, down_edge(), up_edge(), button_state{}};
        in.set_wheel(5);
        in.set_key(sdlpp::scancode::space, down_edge());
        in.set_key(sdlpp::scancode::a, button_state{false, false, true});
        in.set_modifiers(mods::ctrl);

        in.set_gamepad_connected(0, true);
        in.set_gamepad_button(0, sdlpp::gamepad_button::south, down_edge());
        in.set_gamepad_axis(0, sdlpp::gamepad_axis::leftx, 0.85f);

        const input_snapshot rest = in.without_edges();

        SUBCASE("mouse edges are gone, wheel is reset") {
            CHECK_FALSE(rest.mouse(sdlpp::mouse_button::left).pressed);
            CHECK_FALSE(rest.mouse(sdlpp::mouse_button::middle).released);
            CHECK(rest.wheel() == 0);
        }

        SUBCASE("mouse held survives") {
            REQUIRE(in.mouse(sdlpp::mouse_button::left).held);
            CHECK(rest.mouse(sdlpp::mouse_button::left).held);
            CHECK_FALSE(rest.mouse(sdlpp::mouse_button::middle).held);
        }

        SUBCASE("keyboard edges are cleared, held survives") {
            CHECK(in.pressed(sdlpp::scancode::space));
            CHECK(in.held(sdlpp::scancode::space));
            CHECK_FALSE(rest.pressed(sdlpp::scancode::space));
            CHECK(rest.held(sdlpp::scancode::space));
            CHECK(rest.held(sdlpp::scancode::a));
            CHECK(rest.modifiers() == mods::ctrl);
        }

        SUBCASE("gamepad edges are cleared, held and axes survive") {
            CHECK(in.gamepad_button_state(0, sdlpp::gamepad_button::south).pressed);
            CHECK(in.gamepad_button_state(0, sdlpp::gamepad_button::south).held);
            CHECK_FALSE(rest.gamepad_button_state(0, sdlpp::gamepad_button::south).pressed);
            CHECK(rest.gamepad_button_state(0, sdlpp::gamepad_button::south).held);
            CHECK(rest.gamepad_axis(0, sdlpp::gamepad_axis::leftx) == doctest::Approx(0.85f));
            CHECK(rest.gamepad_connected(0));
        }

        SUBCASE("the pointer is untouched") {
            CHECK(rest.pointer().render.x == doctest::Approx(6.0f));
            CHECK(rest.pointer().render.y == doctest::Approx(17.0f));
            CHECK(rest.pointer().on_screen);
        }

        SUBCASE("the two spaces are separate types") {
            static_assert(!std::is_convertible_v<decltype(p.window), decltype(p.render)>);
            static_assert(!std::is_convertible_v<decltype(p.render), decltype(p.window)>);
            static_assert(std::is_same_v<decltype(p.window), window_pos>);
            static_assert(std::is_same_v<decltype(p.render), render_pos>);
            CHECK(p.window == window_pos{12.0f, 34.0f});
        }
    }

    TEST_CASE("hotkey and mouse_click matching against snapshot") {
        input_snapshot in;
        in.set_key(sdlpp::scancode::s, down_edge());
        in.set_mouse(sdlpp::mouse_button::left, down_edge());

        const hotkey hk_plain{sdlpp::scancode::s};
        const hotkey hk_ctrl_s{mods::ctrl, sdlpp::scancode::s};
        const mouse_click click_plain{sdlpp::mouse_button::left};
        const mouse_click click_ctrl{mods::ctrl, sdlpp::mouse_button::left};

        SUBCASE("no modifiers held") {
            CHECK(hk_plain.pressed(in));
            CHECK(in.pressed(hk_plain));
            CHECK_FALSE(hk_ctrl_s.pressed(in));
            CHECK_FALSE(in.pressed(hk_ctrl_s));

            CHECK(click_plain.pressed(in));
            CHECK(in.pressed(click_plain));
            CHECK_FALSE(click_ctrl.pressed(in));
        }

        SUBCASE("with ctrl held") {
            in.set_modifiers(mods::ctrl);
            CHECK_FALSE(hk_plain.pressed(in)); // strict matching: requires no modifiers
            CHECK(hk_ctrl_s.pressed(in));
            CHECK(in.pressed(hk_ctrl_s));

            CHECK_FALSE(click_plain.pressed(in));
            CHECK(click_ctrl.pressed(in));
        }
    }

    TEST_CASE("gamepad_button matching against snapshot") {
        input_snapshot in;
        in.set_gamepad_connected(0, true);
        in.set_gamepad_button(0, sdlpp::gamepad_button::south, down_edge());

        const gamepad_button btn{sdlpp::gamepad_button::south};
        CHECK(btn.pressed(in));
        CHECK(btn.held(in));
        CHECK(in.pressed(btn));
        CHECK(in.held(btn));

        const gamepad_button p2_btn{1, sdlpp::gamepad_button::south};
        CHECK_FALSE(p2_btn.pressed(in)); // slot 1 not pressed
    }

    TEST_CASE("a press is observed exactly once across a multi-substep frame") {
        input_snapshot in;
        in.set_mouse(sdlpp::mouse_button::left, down_edge());
        in.set_key(sdlpp::scancode::space, down_edge());
        in.set_gamepad_connected(0, true);
        in.set_gamepad_button(0, sdlpp::gamepad_button::south, down_edge());

        const input_snapshot held_only = in.without_edges();

        const hotkey jump{sdlpp::scancode::space};
        const gamepad_button gp_jump{sdlpp::gamepad_button::south};

        int mouse_fired = 0;
        int key_fired = 0;
        int gp_fired = 0;
        int key_held_seen = 0;

        constexpr int substeps = 4;
        for (int step = 0; step < substeps; ++step) {
            const input_snapshot& s = (step == 0) ? in : held_only;
            if (s.mouse(sdlpp::mouse_button::left).pressed) {
                ++mouse_fired;
            }
            if (s.pressed(jump)) {
                ++key_fired;
            }
            if (s.pressed(gp_jump)) {
                ++gp_fired;
            }
            if (s.held(jump)) {
                ++key_held_seen;
            }
        }
        CHECK(mouse_fired == 1);
        CHECK(key_fired == 1);
        CHECK(gp_fired == 1);
        CHECK(key_held_seen == substeps);
    }

    TEST_CASE("a default snapshot reports nothing pressed and an off-screen pointer") {
        const input_snapshot in;
        CHECK_FALSE(in.pointer().on_screen);
        CHECK_FALSE(in.mouse(sdlpp::mouse_button::left).held);
        CHECK_FALSE(in.mouse(sdlpp::mouse_button::right).pressed);
        CHECK_FALSE(in.mouse(sdlpp::mouse_button::x1).held);
        CHECK_FALSE(in.held(sdlpp::scancode::space));
        CHECK_FALSE(in.pressed(sdlpp::scancode::escape));
        CHECK(in.modifiers() == mods::none);
        CHECK_FALSE(in.gamepad_connected(0));
        CHECK(in.gamepad_axis(0, sdlpp::gamepad_axis::leftx) == 0.0f);
        CHECK(in.wheel() == 0);
    }
}
