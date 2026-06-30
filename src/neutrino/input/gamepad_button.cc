#include <neutrino/input/gamepad_button.hh>
#include <neutrino/application.hh>

namespace neutrino {

    extern application* g_app;

    namespace {
        bool check_state(int gamepad_index, sdlpp::gamepad_button button, bool sdlpp::button_state::*member) noexcept {
            if (!g_app) {
                return false;
            }
            auto state = g_app->get_gamepad_button_state(gamepad_index, button);
            return state.*member;
        }
    }

    bool gamepad_button::pressed() const noexcept {
        return check_state(m_gamepad_index, m_button, &sdlpp::button_state::pressed);
    }

    bool gamepad_button::held() const noexcept {
        return check_state(m_gamepad_index, m_button, &sdlpp::button_state::held);
    }

    bool gamepad_button::released() const noexcept {
        return check_state(m_gamepad_index, m_button, &sdlpp::button_state::released);
    }

    // Global gamepad axis helpers
    float gamepad_axis(sdlpp::gamepad_axis axis) noexcept {
        return gamepad_axis(0, axis);
    }

    float gamepad_axis(int gamepad_index, sdlpp::gamepad_axis axis) noexcept {
        return g_app ? g_app->get_gamepad_axis(gamepad_index, axis) : 0.0f;
    }

} // namespace neutrino
