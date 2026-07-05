#include <neutrino/input/hotkey.hh>
#include <neutrino/application.hh>
#include "modifier_match.hh"
#include "services/service_locator.hh"
#include <SDL3/SDL_keyboard.h>

namespace neutrino {

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

        bool check_state(modifier mods, sdlpp::scancode scan, bool sdlpp::button_state::*member) noexcept {
            auto* app = service_locator::instance().get_application();
            if (!app) {
                return false;
            }

            auto state = app->get_key(scan);
            if (!(state.*member)) {
                return false;
            }

            modifier current_mods = input_detail::current_modifiers();

            // If the primary key is itself a modifier, exclude it from matching
            modifier self_mask = get_modifier_for_scancode(scan);
            if (self_mask != modifier::none) {
                mods = mods & ~self_mask;
                current_mods = current_mods & ~self_mask;
            }

            return input_detail::match_modifiers(mods, current_mods);
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
