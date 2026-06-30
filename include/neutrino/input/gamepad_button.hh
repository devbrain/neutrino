#pragma once

#include <neutrino/neutrino_export.h>
#include <sdlpp/input/gamepad.hh>

namespace neutrino {

    class NEUTRINO_EXPORT gamepad_button {
    public:
        // Defaults to gamepad index 0 (Player 1)
        gamepad_button(sdlpp::gamepad_button button)
            : m_gamepad_index(0), m_button(button) {}

        gamepad_button(int gamepad_index, sdlpp::gamepad_button button)
            : m_gamepad_index(gamepad_index), m_button(button) {}

        // Checking methods
        [[nodiscard]] bool pressed() const noexcept;
        [[nodiscard]] bool held() const noexcept;
        [[nodiscard]] bool released() const noexcept;

        // Default operator bool maps to pressed()
        [[nodiscard]] explicit operator bool() const noexcept {
            return pressed();
        }

    private:
        int m_gamepad_index;
        sdlpp::gamepad_button m_button;
    };

    // Global gamepad analog axis retrieval helpers
    [[nodiscard]] NEUTRINO_EXPORT float gamepad_axis(sdlpp::gamepad_axis axis) noexcept;
    [[nodiscard]] NEUTRINO_EXPORT float gamepad_axis(int gamepad_index, sdlpp::gamepad_axis axis) noexcept;

} // namespace neutrino
