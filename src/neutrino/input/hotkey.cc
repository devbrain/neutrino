#include <neutrino/input/hotkey.hh>
#include <neutrino/application.hh>
#include <sdlpp/input/keyboard.hh>
#include <SDL3/SDL_keyboard.h>

namespace neutrino {

    extern application* g_app;

    sdlpp::scancode hotkey::resolve_scancode() const noexcept {
        if (std::holds_alternative<sdlpp::scancode>(m_key)) {
            return std::get<sdlpp::scancode>(m_key);
        }
        auto key = std::get<sdlpp::keycode>(m_key);
        SDL_Scancode scan = SDL_GetScancodeFromKey(static_cast<SDL_Keycode>(key), nullptr);
        return static_cast<sdlpp::scancode>(scan);
    }

    namespace {
        modifier get_modifier_for_scancode(sdlpp::scancode scan) noexcept {
            switch (scan) {
                case sdlpp::scancode::lshift: return modifier::lshift;
                case sdlpp::scancode::rshift: return modifier::rshift;
                case sdlpp::scancode::lctrl:  return modifier::lctrl;
                case sdlpp::scancode::rctrl:  return modifier::rctrl;
                case sdlpp::scancode::lalt:   return modifier::lalt;
                case sdlpp::scancode::ralt:   return modifier::ralt;
                case sdlpp::scancode::lgui:   return modifier::lgui;
                case sdlpp::scancode::rgui:   return modifier::rgui;
                default:                      return modifier::none;
            }
        }

        modifier translate_keymod(sdlpp::keymod sdl_mods) noexcept {
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

        bool match_modifier_group(modifier expected, modifier current, modifier left, modifier right) noexcept {
            bool expected_left = (expected & left) != modifier::none;
            bool expected_right = (expected & right) != modifier::none;
            bool current_left = (current & left) != modifier::none;
            bool current_right = (current & right) != modifier::none;

            // If both are expected (e.g. CTRL), match if at least one is pressed
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

        bool check_state(modifier mods, sdlpp::scancode scan, bool sdlpp::button_state::*member) noexcept {
            if (!g_app) {
                return false;
            }

            auto state = g_app->get_key_state(scan);
            if (!(state.*member)) {
                return false;
            }

            // Get current modifier states
            auto current_sdl_mods = sdlpp::get_mod_state();
            modifier current_mods = translate_keymod(current_sdl_mods);

            // If the primary key is itself a modifier, exclude it from matching
            modifier self_mask = get_modifier_for_scancode(scan);
            if (self_mask != modifier::none) {
                mods = mods & ~self_mask;
                current_mods = current_mods & ~self_mask;
            }

            // Compare each modifier group individually
            return match_modifier_group(mods, current_mods, modifier::lshift, modifier::rshift) &&
                   match_modifier_group(mods, current_mods, modifier::lctrl, modifier::rctrl) &&
                   match_modifier_group(mods, current_mods, modifier::lalt, modifier::ralt) &&
                   match_modifier_group(mods, current_mods, modifier::lgui, modifier::rgui);
        }
    }

    bool hotkey::pressed() const noexcept {
        return check_state(m_mods, resolve_scancode(), &sdlpp::button_state::pressed);
    }

    bool hotkey::held() const noexcept {
        return check_state(m_mods, resolve_scancode(), &sdlpp::button_state::held);
    }

    bool hotkey::released() const noexcept {
        return check_state(m_mods, resolve_scancode(), &sdlpp::button_state::released);
    }

} // namespace neutrino
