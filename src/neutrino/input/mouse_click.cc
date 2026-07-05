#include <neutrino/input/mouse_click.hh>
#include <neutrino/application.hh>
#include "modifier_match.hh"
#include "services/service_locator.hh"

namespace neutrino {

    namespace {
        application* app_ptr() noexcept {
            return service_locator::instance().get_application();
        }

        bool check_state(modifier mods, sdlpp::mouse_button button, bool sdlpp::button_state::*member) noexcept {
            auto* app = app_ptr();
            if (!app) {
                return false;
            }

            auto state = app->get_mouse(button);
            if (!(state.*member)) {
                return false;
            }

            return input_detail::match_modifiers(mods, input_detail::current_modifiers());
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
        auto* app = app_ptr();
        return app ? app->get_mouse_x() : 0;
    }

    int mouse_y() noexcept {
        auto* app = app_ptr();
        return app ? app->get_mouse_y() : 0;
    }

    sdlpp::point_i mouse_pos() noexcept {
        auto* app = app_ptr();
        return app ? app->get_mouse_pos() : sdlpp::point_i{0, 0};
    }

    int mouse_wheel() noexcept {
        auto* app = app_ptr();
        return app ? app->get_mouse_wheel() : 0;
    }

} // namespace neutrino
