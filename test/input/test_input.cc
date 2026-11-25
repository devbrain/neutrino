#include <doctest/doctest.h>
#include <neutrino/input/input_snapshot.hh>
#include <neutrino/input/hotkey.hh>
#include <neutrino/input/mouse_click.hh>
#include <neutrino/input/gamepad_button.hh>

TEST_SUITE("neutrino::input") {
    TEST_CASE("queries against a default (neutral) snapshot report nothing pressed") {
        const neutrino::input_snapshot in;

        neutrino::hotkey hk(neutrino::mods::ctrl, sdlpp::scancode::a);
        CHECK_FALSE(hk.pressed(in));
        CHECK_FALSE(hk.held(in));
        CHECK_FALSE(hk.released(in));
        CHECK_FALSE(in.pressed(hk));
        CHECK_FALSE(in.held(hk));
        CHECK_FALSE(in.released(hk));

        neutrino::mouse_click click(sdlpp::mouse_button::left);
        CHECK_FALSE(click.pressed(in));
        CHECK_FALSE(click.held(in));
        CHECK_FALSE(click.released(in));
        CHECK_FALSE(in.pressed(click));
        CHECK_FALSE(in.held(click));
        CHECK_FALSE(in.released(click));

        neutrino::gamepad_button btn(sdlpp::gamepad_button::south);
        CHECK_FALSE(btn.pressed(in));
        CHECK_FALSE(btn.held(in));
        CHECK_FALSE(btn.released(in));
        CHECK_FALSE(in.pressed(btn));
        CHECK_FALSE(in.held(btn));
        CHECK_FALSE(in.released(btn));

        CHECK(in.gamepad_axis(0, sdlpp::gamepad_axis::leftx) == 0.0f);
        CHECK(in.gamepad_axis(3, sdlpp::gamepad_axis::righty) == 0.0f);
        CHECK_FALSE(in.gamepad_connected(0));
        CHECK_FALSE(in.pointer().on_screen);
        CHECK(in.wheel() == 0);
    }
    TEST_CASE("check hotkey operator +") {
        {
            neutrino::hotkey hk = neutrino::mods::ctrl + neutrino::key::a;
            CHECK(hk.modifiers() == neutrino::mods::ctrl);
            auto k = hk.key();
            CHECK(((std::get_if<neutrino::key>(&k) != nullptr) && (std::get<neutrino::key>(k) == neutrino::key::a)));
        }
        {
            neutrino::hotkey hk = neutrino::key::a + neutrino::mods::ctrl;
            CHECK(hk.modifiers() == neutrino::mods::ctrl);
            auto k = hk.key();
            CHECK(((std::get_if<neutrino::key>(&k) != nullptr) && (std::get<neutrino::key>(k) == neutrino::key::a)));
        }
    }
}
