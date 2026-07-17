#pragma once

#include <neutrino/neutrino_export.h>
#include <sdlpp/input/gamepad.hh>

namespace neutrino {

    /// @brief Polled query for a single gamepad button on a specific player slot.
    ///
    /// Reports the current frame's state of one button on the pad bound to
    /// @p gamepad_index (the stable player slot from
    /// application::on_gamepad_connected). All queries read live state and
    /// return false when no gamepad service is available or the slot is empty
    /// (e.g. the pad is disconnected).
    class NEUTRINO_EXPORT gamepad_button {
    public:
        /// @brief Query @p button on gamepad index 0 (Player 1).
        gamepad_button(sdlpp::gamepad_button button)
            : m_gamepad_index(0), m_button(button) {}

        /// @brief Query @p button on the pad in slot @p gamepad_index.
        gamepad_button(int gamepad_index, sdlpp::gamepad_button button)
            : m_gamepad_index(gamepad_index), m_button(button) {}

        /// @brief True on the frame the button transitions to down (edge).
        [[nodiscard]] bool pressed() const noexcept;
        /// @brief True on every frame the button is down (level).
        [[nodiscard]] bool held() const noexcept;
        /// @brief True on the frame the button transitions to up (edge).
        [[nodiscard]] bool released() const noexcept;

        /// @brief Equivalent to pressed(); lets a gamepad_button be used directly in a condition.
        [[nodiscard]] explicit operator bool() const noexcept {
            return pressed();
        }

    private:
        int m_gamepad_index;
        sdlpp::gamepad_button m_button;
    };

    /// @brief Current value of @p axis on gamepad 0, normalized to [-1, 1]
    /// (triggers to [0, 1]); returns 0 if no gamepad service is available.
    [[nodiscard]] NEUTRINO_EXPORT float gamepad_axis(sdlpp::gamepad_axis axis) noexcept;
    /// @brief Current value of @p axis on the pad in slot @p gamepad_index,
    /// normalized to [-1, 1] (triggers to [0, 1]); returns 0 if the service is
    /// unavailable or the slot is empty.
    [[nodiscard]] NEUTRINO_EXPORT float gamepad_axis(int gamepad_index, sdlpp::gamepad_axis axis) noexcept;

} // namespace neutrino
