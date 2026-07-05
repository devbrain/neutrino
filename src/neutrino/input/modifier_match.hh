/// @file modifier_match.hh
/// @brief Shared modifier translation and matching for hotkey / mouse_click.

#pragma once

#include <cstdint>
#include <neutrino/input/hotkey.hh> // modifier
#include <sdlpp/input/keyboard.hh>
#include <SDL3/SDL_keyboard.h>

namespace neutrino::input_detail {

    inline modifier translate_keymod(sdlpp::keymod sdl_mods) noexcept {
        modifier m = modifier::none;
        auto val = static_cast<std::uint16_t>(sdl_mods);
        if (val & SDL_KMOD_LSHIFT) m = m | modifier::lshift;
        if (val & SDL_KMOD_RSHIFT) m = m | modifier::rshift;
        if (val & SDL_KMOD_LCTRL)  m = m | modifier::lctrl;
        if (val & SDL_KMOD_RCTRL)  m = m | modifier::rctrl;
        if (val & SDL_KMOD_LALT)   m = m | modifier::lalt;
        if (val & SDL_KMOD_RALT)   m = m | modifier::ralt;
        if (val & SDL_KMOD_LGUI)   m = m | modifier::lgui;
        if (val & SDL_KMOD_RGUI)   m = m | modifier::rgui;
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
