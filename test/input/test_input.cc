#include <doctest/doctest.h>
#include <neutrino/input/hotkey.hh>
#include <neutrino/input/mouse_click.hh>
#include <neutrino/input/gamepad_button.hh>

// Input queries are global and may run when no application exists (before
// construction, after destruction, or in headless tools). They must degrade
// to "nothing pressed" instead of crashing.
TEST_SUITE("neutrino::input") {
    TEST_CASE("queries are safe without an application") {
        neutrino::hotkey hk(neutrino::mods::ctrl, sdlpp::scancode::a);
        CHECK_FALSE(hk.pressed());
        CHECK_FALSE(hk.held());
        CHECK_FALSE(hk.released());

        neutrino::mouse_click click(sdlpp::mouse_button::left);
        CHECK_FALSE(click.pressed());
        CHECK_FALSE(click.held());
        CHECK_FALSE(click.released());

        CHECK(neutrino::mouse_x() == 0);
        CHECK(neutrino::mouse_y() == 0);
        CHECK(neutrino::mouse_pos() == sdlpp::point_i{0, 0});
        CHECK(neutrino::mouse_wheel() == 0);

        neutrino::gamepad_button btn(sdlpp::gamepad_button::south);
        CHECK_FALSE(btn.pressed());
        CHECK_FALSE(btn.held());
        CHECK_FALSE(btn.released());

        CHECK(neutrino::gamepad_axis(sdlpp::gamepad_axis::leftx) == 0.0f);
        CHECK(neutrino::gamepad_axis(3, sdlpp::gamepad_axis::righty) == 0.0f);
    }
}
