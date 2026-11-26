//
// Created by igor on 26/11/2026.
//

#include <neutrino/input/binding.hh>

#include <array>
#include <cctype>
#include <charconv>
#include <cstddef>
#include <string>
#include <utility>

#include <neutrino/detail/overloaded.hh>
#include <neutrino/input/input_snapshot.hh>
#include <sdlpp/events/keyboard_codes.hh>
#include <sdlpp/input/keyboard.hh>

#include "binding_text.hh"
#include "key_names.hh"

namespace neutrino {
    namespace {
        // ---- Names -------------------------------------------------------------------------

        struct mouse_name {
            mouse_button button;
            std::string_view name;
        };

        constexpr std::array <mouse_name, 5> mouse_names{{
            {mouse_button::left, "Left"},
            {mouse_button::middle, "Middle"},
            {mouse_button::right, "Right"},
            {mouse_button::x1, "X1"},
            {mouse_button::x2, "X2"},
        }};

        struct pad_name {
            pad_button button;
            std::string_view name;
        };

        constexpr std::array <pad_name, 26> pad_names{{
            {pad_button::south, "South"},
            {pad_button::east, "East"},
            {pad_button::west, "West"},
            {pad_button::north, "North"},
            {pad_button::back, "Back"},
            {pad_button::guide, "Guide"},
            {pad_button::start, "Start"},
            {pad_button::left_stick, "Left Stick"},
            {pad_button::right_stick, "Right Stick"},
            {pad_button::left_shoulder, "Left Shoulder"},
            {pad_button::right_shoulder, "Right Shoulder"},
            {pad_button::dpad_up, "DPad Up"},
            {pad_button::dpad_down, "DPad Down"},
            {pad_button::dpad_left, "DPad Left"},
            {pad_button::dpad_right, "DPad Right"},
            {pad_button::misc1, "Misc 1"},
            {pad_button::misc2, "Misc 2"},
            {pad_button::misc3, "Misc 3"},
            {pad_button::misc4, "Misc 4"},
            {pad_button::misc5, "Misc 5"},
            {pad_button::misc6, "Misc 6"},
            {pad_button::left_paddle1, "Left Paddle 1"},
            {pad_button::left_paddle2, "Left Paddle 2"},
            {pad_button::right_paddle1, "Right Paddle 1"},
            {pad_button::right_paddle2, "Right Paddle 2"},
            {pad_button::touchpad, "Touchpad"},
        }};

        // A modifier group: its name, and its left and right bits.
        struct modifier_group {
            std::string_view name;
            modifier left;
            modifier right;
        };

        // The order they are written in: "Ctrl+Shift+Alt+GUI+key".
        constexpr std::array <modifier_group, 4> modifier_groups{{
            {"Ctrl", modifier::lctrl, modifier::rctrl},
            {"Shift", modifier::lshift, modifier::rshift},
            {"Alt", modifier::lalt, modifier::ralt},
            {"GUI", modifier::lgui, modifier::rgui},
        }};

        constexpr std::string_view mouse_prefix = "Mouse";
        constexpr std::string_view pad_prefix = "Pad";
        constexpr std::string_view scancode_prefix = "Scancode";
        constexpr std::string_view pad_button_word = "Button";
        constexpr std::string_view strict_none = "None";

        // ---- Text helpers ------------------------------------------------------------------

        [[nodiscard]] bool same_text(std::string_view a, std::string_view b) noexcept {
            if (a.size() != b.size()) {
                return false;
            }
            for (std::size_t i = 0; i < a.size(); ++i) {
                if (std::tolower(static_cast <unsigned char>(a[i])) != std::tolower(static_cast <unsigned char>(b[i]))) {
                    return false;
                }
            }
            return true;
        }

        // Words separated by any run of blanks, matched without regard to case: "dpad  UP" is
        // "DPad Up".
        [[nodiscard]] bool same_words(std::string_view text, std::string_view name) noexcept {
            text = input_detail::trim(text);
            while (!text.empty() && !name.empty()) {
                const auto text_end = text.find_first_of(" \t");
                const auto name_end = name.find(' ');
                if (!same_text(text.substr(0, text_end), name.substr(0, name_end))) {
                    return false;
                }
                text = text_end == std::string_view::npos ? std::string_view{} : input_detail::trim(text.substr(text_end));
                name = name_end == std::string_view::npos ? std::string_view{} : name.substr(name_end + 1);
            }
            return text.empty() && name.empty();
        }

        // The first word of @p text, if it is @p word, and what follows it.
        [[nodiscard]] std::optional <std::string_view> after_word(std::string_view text, std::string_view word) noexcept {
            text = input_detail::trim(text);
            const auto end = text.find_first_of(" \t");
            if (!same_text(text.substr(0, end), word)) {
                return std::nullopt;
            }
            return end == std::string_view::npos ? std::string_view{} : input_detail::trim(text.substr(end));
        }

        [[nodiscard]] std::optional <int> parse_int(std::string_view text) noexcept {
            int value = 0;
            const auto* const last = text.data() + text.size();
            const auto [ptr, ec] = std::from_chars(text.data(), last, value);
            if (ec != std::errc{} || ptr != last) {
                return std::nullopt;
            }
            return value;
        }

        // ---- To text -----------------------------------------------------------------------

        // The engine's own names (key_names.hh), not SDL's current ones: those change per
        // platform once video starts. A key with no name, and a keycode that maps to no key
        // under the current layout (key::unknown), are written by number. SDL makes a scancode
        // of any keycode carrying its scancode flag, even one past its table: such a key never
        // fires, and is written as no key.
        [[nodiscard]] std::string key_name(key k) {
            if (static_cast <int>(k) < 0 || static_cast <int>(k) >= static_cast <int>(input_snapshot::max_scancodes)) {
                k = key::unknown;
            }
            for (const auto& [code, name] : input_detail::key_names) {
                if (code == static_cast <int>(k)) {
                    return std::string{name};
                }
            }
            return std::string{scancode_prefix} + " " + std::to_string(static_cast <int>(k));
        }

        [[nodiscard]] std::string mouse_text(mouse_button button) {
            for (const auto& [b, name] : mouse_names) {
                if (b == button) {
                    return std::string{mouse_prefix} + " " + std::string{name};
                }
            }
            return std::string{mouse_prefix} + " " + std::to_string(static_cast <int>(button));
        }

        [[nodiscard]] std::string pad_text(const gamepad_button& g) {
            std::string text{pad_prefix};
            text += ' ';
            if (g.gamepad_index() != 0) {
                text += std::to_string(g.gamepad_index() + 1);
                text += ' ';
            }
            for (const auto& [b, name] : pad_names) {
                if (b == g.button()) {
                    return text + std::string{name};
                }
            }
            return text + std::string{pad_button_word} + " " + std::to_string(static_cast <int>(g.button()));
        }

        // A strict binding's modifiers, each followed by '+': "Ctrl+Shift+", "LCtrl+", "None+".
        [[nodiscard]] std::string modifiers_text(modifier mods) {
            if (mods == modifier::none) {
                return std::string{strict_none} + "+";
            }
            std::string text;
            for (const auto& [name, left, right] : modifier_groups) {
                const bool l = (mods & left) != modifier::none;
                const bool r = (mods & right) != modifier::none;
                if (l && r) {
                    text += name;
                } else if (l) {
                    text += 'L';
                    text += name;
                } else if (r) {
                    text += 'R';
                    text += name;
                } else {
                    continue;
                }
                text += '+';
            }
            return text;
        }

        // ---- From text ---------------------------------------------------------------------

        [[nodiscard]] std::optional <modifier> modifier_token(std::string_view token) noexcept {
            if (same_text(token, strict_none)) {
                return modifier::none;
            }
            for (const auto& [name, left, right] : modifier_groups) {
                if (same_text(token, name)) {
                    return left | right;
                }
                if (token.size() == name.size() + 1 && same_text(token.substr(1), name)) {
                    if (token[0] == 'L' || token[0] == 'l') {
                        return left;
                    }
                    if (token[0] == 'R' || token[0] == 'r') {
                        return right;
                    }
                }
            }
            return std::nullopt;
        }

        // "dpad  UP" as "dpad up": SDL matches names without regard to case, but not blanks.
        [[nodiscard]] std::string single_blanks(std::string_view text) {
            std::string result;
            bool blank = false;
            for (const char c : input_detail::trim(text)) {
                if (c == ' ' || c == '\t') {
                    blank = true;
                    continue;
                }
                if (blank) {
                    result += ' ';
                    blank = false;
                }
                result += c;
            }
            return result;
        }

        [[nodiscard]] std::optional <mouse_button> parse_mouse(std::string_view name) noexcept {
            for (const auto& [b, n] : mouse_names) {
                if (same_words(name, n)) {
                    return b;
                }
            }
            // A button with no name, as mouse_text() writes it.
            const auto number = parse_int(input_detail::trim(name));
            if (number && *number >= 0 && *number <= 255) {
                return static_cast <mouse_button>(*number);
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional <gamepad_button> parse_pad(std::string_view rest) noexcept {
            // "Pad South" is slot 0; "Pad 2 South" slot 1. Any number reads back (a slot no pad
            // is in never fires), as pad_text() writes it.
            int slot = 0;
            const auto first_end = rest.find_first_of(" \t");
            if (first_end != std::string_view::npos) {
                if (const auto number = parse_int(rest.substr(0, first_end))) {
                    slot = *number - 1;
                    rest = input_detail::trim(rest.substr(first_end));
                }
            }
            for (const auto& [b, n] : pad_names) {
                if (same_words(rest, n)) {
                    return gamepad_button{slot, b};
                }
            }
            // A button with no name: "Button <n>".
            if (const auto number = after_word(rest, pad_button_word)) {
                if (const auto value = parse_int(*number)) {
                    return gamepad_button{slot, static_cast <pad_button>(*value)};
                }
            }
            return std::nullopt;
        }

        [[nodiscard]] std::optional <key> parse_key(std::string_view name) {
            if (const auto number = after_word(name, scancode_prefix)) {
                const auto value = parse_int(*number);
                if (!value || *value < 0 || *value >= static_cast <int>(input_snapshot::max_scancodes)) {
                    return std::nullopt;
                }
                return static_cast <key>(*value);   // 0: a keycode that mapped to no key
            }
            for (const auto& [code, n] : input_detail::key_names) {
                if (same_words(name, n)) {
                    return static_cast <key>(code);
                }
            }
            for (const auto& [code, n] : input_detail::platform_key_names) {
                if (same_words(name, n)) {
                    return static_cast <key>(code);
                }
            }
            // The names SDL uses on this platform now, for a hand-edited text.
            const auto k = sdlpp::get_scancode_from_name(single_blanks(name));
            if (k == key::unknown) {
                return std::nullopt;
            }
            return k;
        }
    }

    button_state check(const binding& b, const input_snapshot& in) noexcept {
        return std::visit(detail::overloaded{
                              [&in](key k) { return in.key(k); },
                              [&in](mouse_button m) { return in.mouse(m); },
                              [&in](const auto& matcher) {
                                  return button_state{
                                      .pressed = matcher.pressed(in),
                                      .released = matcher.released(in),
                                      .held = matcher.held(in)
                                  };
                              }
                          }, b);
    }

    button_state check(std::span <const binding> bindings, const input_snapshot& in) noexcept {
        bool down_before = false;   // some binding was down before this frame
        bool down_now = false;      // some binding is down now
        bool tapped = false;        // some binding went down this frame, possibly up again
        for (const auto& b : bindings) {
            const auto s = check(b, in);
            down_now = down_now || s.held;
            tapped = tapped || s.pressed;
            // Down before the frame: still held and not pressed in it, or released in it
            // without a press (a press and a release in one frame is a tap from up).
            down_before = down_before || (s.held ? !s.pressed : s.released && !s.pressed);
        }
        return button_state{
            .pressed = !down_before && (down_now || tapped),
            .released = (down_before || tapped) && !down_now,
            .held = down_now
        };
    }

    std::string to_string(const binding& b) {
        return std::visit(detail::overloaded{
                              [](key k) { return key_name(k); },
                              [](mouse_button m) { return mouse_text(m); },
                              [](const hotkey& h) { return modifiers_text(h.modifiers()) + key_name(h.resolve_scancode()); },
                              [](const mouse_click& c) { return modifiers_text(c.modifiers()) + mouse_text(c.button()); },
                              [](const gamepad_button& g) { return pad_text(g); }
                          }, b);
    }

    std::string to_string(std::span <const binding> bindings) {
        std::string text;
        for (const auto& b : bindings) {
            if (!text.empty()) {
                text += ", ";
            }
            text += to_string(b);
        }
        return text;
    }

    std::optional <binding> binding_from_string(std::string_view text) {
        text = input_detail::trim(text);
        modifier mods = modifier::none;
        bool strict = false;      // a modifier or "None" was given
        bool none_given = false;
        bool mods_given = false;
        // Modifiers come first, each followed by '+'. The first part that is not a modifier
        // starts the body, which may itself contain '+' ("Keypad +").
        for (auto plus = text.find('+'); plus != std::string_view::npos; plus = text.find('+')) {
            const auto token = modifier_token(input_detail::trim(text.substr(0, plus)));
            if (!token) {
                break;
            }
            if (*token == modifier::none) {
                none_given = true;
            } else {
                mods = mods | *token;
                mods_given = true;
            }
            strict = true;
            text = input_detail::trim(text.substr(plus + 1));
        }
        if (text.empty() || (none_given && mods_given)) {
            return std::nullopt;
        }
        if (const auto rest = after_word(text, mouse_prefix)) {
            const auto button = parse_mouse(*rest);
            if (!button) {
                return std::nullopt;
            }
            return strict ? binding{mouse_click{mods, *button}} : binding{*button};
        }
        if (const auto rest = after_word(text, pad_prefix)) {
            if (strict) {
                return std::nullopt;   // pad buttons take no modifiers
            }
            if (const auto pad = parse_pad(*rest)) {
                return binding{*pad};
            }
            return std::nullopt;
        }
        const auto k = parse_key(text);
        if (!k) {
            return std::nullopt;
        }
        return strict ? binding{hotkey{mods, *k}} : binding{*k};
    }

    std::optional <std::vector <binding>> bindings_from_string(std::string_view text) {
        text = input_detail::trim(text);
        std::vector <binding> bindings;
        if (text.empty()) {
            return bindings;
        }
        // Separated by ", ". No key name contains it; a ',' key followed by the separator is
        // ",, ", and the first ", " in it is the separator.
        for (;;) {
            const auto separator = text.find(", ");
            const auto b = binding_from_string(text.substr(0, separator));
            if (!b) {
                return std::nullopt;
            }
            bindings.push_back(*b);
            if (separator == std::string_view::npos) {
                return bindings;
            }
            text = text.substr(separator + 2);
        }
    }
}
