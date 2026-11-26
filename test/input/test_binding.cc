//
// Created by igor on 26/11/2026.
//

#include <doctest/doctest.h>

#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include <SDL3/SDL_keyboard.h>

#include <neutrino/input/binding.hh>
#include <neutrino/input/input_snapshot.hh>
#include <neutrino/input/keys.hh>
#include <sdlpp/events/keyboard_codes.hh>
#include <sdlpp/input/keyboard.hh>

using namespace neutrino;

namespace {
    constexpr button_state down{.pressed = true, .released = false, .held = true};
    constexpr button_state held_on{.pressed = false, .released = false, .held = true};
    constexpr button_state up{.pressed = false, .released = true, .held = false};
    constexpr button_state tap{.pressed = true, .released = true, .held = false};

    [[nodiscard]] input_snapshot keys(std::initializer_list <std::pair <key, button_state>> states,
                                      modifier mods = modifier::none) {
        input_snapshot in;
        for (const auto& [k, s] : states) {
            in.set_key(k, s);
        }
        in.set_modifiers(mods);
        return in;
    }

    [[nodiscard]] bool same(const button_state& a, const button_state& b) {
        return a.pressed == b.pressed && a.released == b.released && a.held == b.held;
    }
}

TEST_SUITE("neutrino::binding") {
    TEST_CASE("only keys, buttons and matchers make bindings: a number does not, although a keycode is one") {
        static_assert(std::is_constructible_v <binding, key>);
        static_assert(std::is_constructible_v <binding, pad_button>);
        static_assert(std::is_constructible_v <binding, hotkey>);
        static_assert(!std::is_constructible_v <binding, int>);
        static_assert(!std::is_constructible_v <binding, char>);
        static_assert(!std::is_constructible_v <binding, bool>);
        static_assert(!std::is_constructible_v <binding, keycode>);
        static_assert(bindable <key> && bindable <pad_button> && bindable <mouse_click> && bindable <binding>);
        static_assert(!bindable <int> && !bindable <char> && !bindable <bool> && !bindable <keycode>);
        CHECK(binding{hotkey{static_cast <keycode>('s')}} == binding{hotkey{static_cast <keycode>('s')}});
    }

    TEST_CASE("plain inputs and matchers make bindings; + builds the strict ones") {
        CHECK(std::holds_alternative <key>(binding{key::space}));
        CHECK(std::holds_alternative <mouse_button>(binding{mouse_button::left}));
        CHECK(binding{pad_button::south} == binding{gamepad_button{0, pad_button::south}});
        CHECK(binding{mods::ctrl + key::s} == binding{hotkey{mods::ctrl, key::s}});
        CHECK(binding{mods::ctrl + mouse_button::left} == binding{mouse_click{mods::ctrl, mouse_button::left}});
        CHECK(binding{mouse_button::right + mods::alt} == binding{mouse_click{mods::alt, mouse_button::right}});
        CHECK(binding{mods::ctrl + mods::shift + key::w} == binding{hotkey{mods::ctrl | mods::shift, key::w}});
    }

    TEST_CASE("a plain key ignores modifiers; a strict one does not") {
        const auto in = keys({{key::space, held_on}}, mods::lctrl);
        CHECK(check(binding{key::space}, in).held);
        CHECK_FALSE(check(binding{hotkey{key::space}}, in).held);
        CHECK(check(binding{mods::ctrl + key::space}, in).held);
    }

    TEST_CASE("alternatives are one virtual button") {
        const std::vector <binding> left{key::left, key::a};

        CHECK(same(check(left, input_snapshot{}), button_state{}));
        CHECK(same(check(left, keys({{key::a, down}})), down));
        // One held, another pressed: still down, no new press.
        CHECK(same(check(left, keys({{key::left, held_on}, {key::a, down}})), held_on));
        // One released, another still held: no release.
        CHECK(same(check(left, keys({{key::left, up}, {key::a, held_on}})), held_on));
        // One released, another pressed in the same frame: down throughout.
        CHECK(same(check(left, keys({{key::left, up}, {key::a, down}})), held_on));
        CHECK(same(check(left, keys({{key::a, up}})), up));
        // A tap within the frame counts: pressed and released, not held.
        CHECK(same(check(left, keys({{key::a, tap}})), tap));
        CHECK(same(check(std::vector <binding>{}, keys({{key::a, down}})), button_state{}));
    }

    TEST_CASE("text form: modifiers first, ', ' between bindings") {
        const std::vector <binding> card{mods::ctrl + key::f, key::space, pad_button::south, mouse_button::left};
        CHECK(to_string(card) == "Ctrl+F, Space, Pad South, Mouse Left");
        CHECK(to_string(binding{hotkey{key::space}}) == "None+Space");
        CHECK(to_string(binding{mods::lctrl + key::s}) == "LCtrl+S");
        CHECK(to_string(binding{mods::ctrl + mods::shift + key::w}) == "Ctrl+Shift+W");
        CHECK(to_string(binding{mods::rshift + mods::lalt + mods::gui + key::x}) == "RShift+LAlt+GUI+X");
        CHECK(to_string(binding{mods::alt + mouse_button::right}) == "Alt+Mouse Right");
        CHECK(to_string(binding{mouse_click{mouse_button::x1}}) == "None+Mouse X1");
        CHECK(to_string(binding{gamepad_button{1, pad_button::dpad_up}}) == "Pad 2 DPad Up");
        CHECK(to_string(std::vector <binding>{}).empty());
    }

    TEST_CASE("text form reads back what it writes") {
        const std::vector <binding> samples{
            key::space, mouse_button::middle, hotkey{key::space}, mods::lctrl + key::s,
            mods::ctrl + mods::shift + mods::alt + mods::gui + key::f1, mods::rgui + mouse_button::x2,
            mouse_click{mouse_button::left}, gamepad_button{0, pad_button::left_paddle2},
            gamepad_button{3, pad_button::right_shoulder}, key::kp_plus, mods::ctrl + key::kp_plus,
            key::equals, key::comma, mods::shift + key::comma
        };
        for (const auto& b : samples) {
            CAPTURE(to_string(b));
            CHECK(binding_from_string(to_string(b)) == b);
        }
        CHECK(bindings_from_string(to_string(samples)) == samples);
    }

    TEST_CASE("every key reads back, plain and strict") {
        for (int i = 0; i < static_cast <int>(input_snapshot::max_scancodes); ++i) {
            const auto k = static_cast <key>(i);
            CAPTURE(to_string(binding{k}));
            CHECK(binding_from_string(to_string(binding{k})) == binding{k});
            CHECK(binding_from_string(to_string(binding{mods::ctrl + k})) == binding{mods::ctrl + k});
            CHECK(bindings_from_string(to_string(std::vector <binding>{k, mods::alt + k}))
                  == std::vector <binding>{k, mods::alt + k});
        }
    }

    TEST_CASE("a key SDL has no name for is written by number") {
        const auto unnamed = static_cast <key>(1);
        REQUIRE(std::string{sdlpp::get_scancode_name(unnamed)}.empty());
        CHECK(to_string(binding{unnamed}) == "Scancode 1");
        CHECK(binding_from_string("Scancode 1") == binding{unnamed});
    }

    TEST_CASE("keys are written with the engine's names, whatever SDL calls them on this platform now") {
        // SDL renames keys when video starts (macOS: "Left Option"); the text must not follow.
        REQUIRE(SDL_SetScancodeName(SDL_SCANCODE_LALT, "Left Option"));
        CHECK(to_string(binding{key::lalt}) == "Left Alt");
        CHECK(binding_from_string("Left Alt") == binding{key::lalt});
        CHECK(binding_from_string("Left Option") == binding{key::lalt});
        REQUIRE(SDL_SetScancodeName(SDL_SCANCODE_LALT, "Left Alt"));

        // The names SDL gives keys on other platforms read too.
        CHECK(binding_from_string("Left Option") == binding{key::lalt});
        CHECK(binding_from_string("Right Command") == binding{key::rgui});
        CHECK(binding_from_string("Left Windows") == binding{key::lgui});
        CHECK(binding_from_string("Menu") == binding{key::menu});
        CHECK(to_string(binding{key::application}) == "Application");

        // Return2, which SDL also calls "Return", has a name of its own.
        CHECK(to_string(binding{key::return_key}) == "Return");
        CHECK(to_string(binding{static_cast <key>(SDL_SCANCODE_RETURN2)}) == "Return 2");
    }

    TEST_CASE("what is written by number reads back: unnamed buttons, any pad slot, a keycode with no key") {
        const std::vector <binding> numbered{
            static_cast <mouse_button>(7), mods::ctrl + static_cast <mouse_button>(9),
            gamepad_button{0, static_cast <pad_button>(40)}, gamepad_button{2, static_cast <pad_button>(41)},
            gamepad_button{-1, pad_button::south}, gamepad_button{6, pad_button::south},
            hotkey{mods::ctrl, key::unknown}
        };
        CHECK(to_string(numbered) == "Mouse 7, Ctrl+Mouse 9, Pad Button 40, Pad 3 Button 41, Pad 0 South, "
                                     "Pad 7 South, Ctrl+Scancode 0");
        CHECK(bindings_from_string(to_string(numbered)) == numbered);

        // A keycode that maps to no key under this layout (é on a US one) is written as no key,
        // and reads back; so is one SDL makes a scancode past its table of.
        const hotkey nowhere{mods::ctrl, static_cast <keycode>(0xE9)};
        REQUIRE(nowhere.resolve_scancode() == key::unknown);
        CHECK(binding_from_string(to_string(binding{nowhere})) == binding{hotkey{mods::ctrl, key::unknown}});
        const hotkey past_the_table{mods::ctrl, static_cast <keycode>(0x40000FFFu)};
        CHECK(to_string(binding{past_the_table}) == "Ctrl+Scancode 0");
    }

    TEST_CASE("text form is read without regard to case or extra blanks") {
        CHECK(binding_from_string("  ctrl + s ") == binding{mods::ctrl + key::s});
        CHECK(binding_from_string("lctrl+RSHIFT+a") == binding{mods::lctrl + mods::rshift + key::a});
        CHECK(binding_from_string("pad 2   dpad  UP") == binding{gamepad_button{1, pad_button::dpad_up}});
        CHECK(binding_from_string("Pad 1 South") == binding{pad_button::south});
        CHECK(binding_from_string("MOUSE left") == binding{mouse_button::left});
        CHECK(binding_from_string("none+space") == binding{hotkey{key::space}});
        CHECK(binding_from_string("Left  Shift") == binding{key::lshift});
        CHECK(binding_from_string("left\tshift") == binding{key::lshift});
        CHECK(binding_from_string("Ctrl + Keypad   +") == binding{mods::ctrl + key::kp_plus});
    }

    TEST_CASE("names with '+', '=' and ',' are read whole") {
        CHECK(binding_from_string("Keypad +") == binding{key::kp_plus});
        CHECK(binding_from_string("Ctrl+Keypad +") == binding{mods::ctrl + key::kp_plus});
        CHECK(binding_from_string("=") == binding{key::equals});
        CHECK(bindings_from_string("Ctrl+,, Space") == std::vector <binding>{mods::ctrl + key::comma, key::space});
        CHECK(bindings_from_string(",, ,") == std::vector <binding>{key::comma, key::comma});
    }

    TEST_CASE("a hotkey made from a keycode is written as the scancode it maps to") {
        const hotkey by_keycode{mods::ctrl, static_cast <keycode>('s')};
        CHECK(to_string(binding{by_keycode}) == "Ctrl+S");
        CHECK(binding_from_string(to_string(binding{by_keycode})) == binding{mods::ctrl + key::s});
    }

    TEST_CASE("a blank text is no bindings; a malformed one gives nothing") {
        CHECK(bindings_from_string("") == std::vector <binding>{});
        CHECK(bindings_from_string("   ") == std::vector <binding>{});
        for (const char* bad : {"", "Ctrl", "Ctrl+", "Nokey", "None+Ctrl+S", "Ctrl+Pad South",
                                "None+Pad South", "Pad Nothing", "Pad 2", "Pad Button", "Pad Button x",
                                "Mouse Wheel", "Mouse", "Mouse 256", "Scancode -1", "Scancode 99999",
                                "Scancode x"}) {
            CAPTURE(bad);
            CHECK_FALSE(binding_from_string(bad).has_value());
        }
        CHECK_FALSE(bindings_from_string("Space, Nokey").has_value());
        CHECK_FALSE(bindings_from_string("Space,Return").has_value());
        CHECK_FALSE(bindings_from_string("Space, ").has_value());
    }
}

TEST_SUITE("neutrino::input_snapshot::capture_binding") {
    TEST_CASE("nothing went down: nothing") {
        CHECK_FALSE(input_snapshot{}.capture_binding().has_value());
        CHECK_FALSE(keys({{key::s, held_on}}).capture_binding().has_value());
    }

    TEST_CASE("a key alone is the plain key; with modifiers, the strict one on either side") {
        CHECK(keys({{key::s, down}}).capture_binding() == binding{key::s});
        auto in = keys({{key::lctrl, held_on}, {key::s, down}}, mods::lctrl);
        CHECK(in.capture_binding() == binding{mods::ctrl + key::s});
        in = keys({{key::rshift, held_on}, {key::lalt, held_on}, {key::w, down}}, mods::rshift + mods::lalt);
        CHECK(in.capture_binding() == binding{mods::shift + mods::alt + key::w});
    }

    TEST_CASE("a modifier pressed alone is skipped") {
        CHECK_FALSE(keys({{key::lctrl, down}}, mods::lctrl).capture_binding().has_value());
    }

    TEST_CASE("mouse buttons, then pads; keys first") {
        input_snapshot in;
        in.set_mouse(mouse_button::left, down);
        in.set_modifiers(mods::lshift);
        CHECK(in.capture_binding() == binding{mods::shift + mouse_button::left});
        in.set_modifiers(modifier::none);
        CHECK(in.capture_binding() == binding{mouse_button::left});
        in.set_key(key::q, down);
        CHECK(in.capture_binding() == binding{key::q});

        input_snapshot pad;
        pad.set_gamepad_connected(1, true);
        pad.set_gamepad_button(1, pad_button::east, down);
        CHECK(pad.capture_binding() == binding{gamepad_button{1, pad_button::east}});
    }
}
