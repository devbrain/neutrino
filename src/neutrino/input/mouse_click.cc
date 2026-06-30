#include <neutrino/input/mouse_click.hh>
#include <neutrino/application.hh>
#include <sdlpp/input/keyboard.hh>

namespace neutrino {

    extern application* g_app;

    namespace {
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

            if (expected_left && expected_right) {
                return current_left || current_right;
            }
            if (expected_left) {
                return current_left && !current_right;
            }
            if (expected_right) {
                return current_right && !current_left;
            }
            return !current_left && !current_right;
        }

        bool check_state(modifier mods, sdlpp::mouse_button button, bool sdlpp::button_state::*member) noexcept {
            if (!g_app) {
                return false;
            }

            auto state = g_app->get_mouse_state(button);
            if (!(state.*member)) {
                return false;
            }

            auto current_sdl_mods = sdlpp::get_mod_state();
            modifier current_mods = translate_keymod(current_sdl_mods);

            return match_modifier_group(mods, current_mods, modifier::lshift, modifier::rshift) &&
                   match_modifier_group(mods, current_mods, modifier::lctrl, modifier::rctrl) &&
                   match_modifier_group(mods, current_mods, modifier::lalt, modifier::ralt) &&
                   match_modifier_group(mods, current_mods, modifier::lgui, modifier::rgui);
        }
    }

    bool mouse_click::pressed() const noexcept {
        return check_state(m_mods, m_button, &sdlpp::button_state::pressed);
    }

    bool mouse_click::held() const noexcept {
        return check_state(m_mods, m_button, &sdlpp::button_state::held);
    }

    bool mouse_click::released() const noexcept {
        return check_state(m_mods, m_button, &sdlpp::button_state::released);
    }

    // Global helper functions
    int mouse_x() noexcept {
        return g_app ? g_app->get_mouse_x() : 0;
    }

    int mouse_y() noexcept {
        return g_app ? g_app->get_mouse_y() : 0;
    }

    sdlpp::point_i mouse_pos() noexcept {
        return g_app ? g_app->get_mouse_pos() : sdlpp::point_i{0, 0};
    }

    int mouse_wheel() noexcept {
        return g_app ? g_app->get_mouse_wheel() : 0;
    }

} // namespace neutrino
