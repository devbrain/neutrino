/// @file modifier_match.hh
/// @brief Shared modifier translation and matching for hotkey / mouse_click.

#pragma once

#include <cstdint>
#include <neutrino/input/hotkey.hh> // modifier
#include <sdlpp/events/keyboard_codes.hh>
#include <sdlpp/input/keyboard.hh>

namespace neutrino::input_detail {

    inline modifier translate_keymod(sdlpp::keymod sdl_mods) noexcept {
        const auto held = [sdl_mods](sdlpp::keymod k) noexcept {
            return (sdl_mods & k) != sdlpp::keymod::none;
        };
        modifier m = modifier::none;
        if (held(sdlpp::keymod::lshift)) m = m | modifier::lshift;
        if (held(sdlpp::keymod::rshift)) m = m | modifier::rshift;
        if (held(sdlpp::keymod::lctrl))  m = m | modifier::lctrl;
        if (held(sdlpp::keymod::rctrl))  m = m | modifier::rctrl;
        if (held(sdlpp::keymod::lalt))   m = m | modifier::lalt;
        if (held(sdlpp::keymod::ralt))   m = m | modifier::ralt;
        if (held(sdlpp::keymod::lgui))   m = m | modifier::lgui;
        if (held(sdlpp::keymod::rgui))   m = m | modifier::rgui;
        return m;
    }

    inline modifier current_modifiers() noexcept {
        return translate_keymod(sdlpp::get_mod_state());
    }

    inline bool match_modifier_group(modifier expected, modifier current, modifier left, modifier right) noexcept {
        bool expected_left = (expected & left) != modifier::none;
        bool expected_right = (expected & right) != modifier::none;
        bool current_left = (current & left) != modifier::none;
        bool current_right = (current & right) != modifier::none;

        // If both are expected (e.g. ctrl), match if at least one is pressed
        if (expected_left && expected_right) {
            return current_left || current_right;
        }
        // If only specific left is expected, match if left is pressed and right is not
        if (expected_left) {
            return current_left && !current_right;
        }
        // If only specific right is expected, match if right is pressed and left is not
        if (expected_right) {
            return current_right && !current_left;
        }
        // If none expected, match if neither is pressed
        return !current_left && !current_right;
    }

    // Compare each modifier group individually (strict matching: groups not
    // named in `expected` must be idle).
    inline bool match_modifiers(modifier expected, modifier current) noexcept {
        return match_modifier_group(expected, current, modifier::lshift, modifier::rshift) &&
               match_modifier_group(expected, current, modifier::lctrl, modifier::rctrl) &&
               match_modifier_group(expected, current, modifier::lalt, modifier::ralt) &&
               match_modifier_group(expected, current, modifier::lgui, modifier::rgui);
    }

} // namespace neutrino::input_detail
