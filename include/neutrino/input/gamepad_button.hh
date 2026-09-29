#pragma once

#include <neutrino/neutrino_export.h>
#include <sdlpp/input/gamepad.hh>

namespace neutrino {

    class input_snapshot;

    /// @brief Query for a single gamepad button on a specific player slot.
    ///
    /// Evaluated against a frame's @ref input_snapshot. Slot 0 is Player 1.
    class NEUTRINO_EXPORT gamepad_button {
    public:
        /// @brief Query @p button on gamepad index 0 (Player 1).
        gamepad_button(sdlpp::gamepad_button button)
            : m_gamepad_index(0), m_button(button) {}

        /// @brief Query @p button on the pad in slot @p gamepad_index.
        gamepad_button(int gamepad_index, sdlpp::gamepad_button button)
            : m_gamepad_index(gamepad_index), m_button(button) {}

        /// @brief True on the frame the button transitions to down (edge).
        [[nodiscard]] bool pressed(const input_snapshot& in) const noexcept;
        /// @brief True on every frame the button is down (level).
        [[nodiscard]] bool held(const input_snapshot& in) const noexcept;
        /// @brief True on the frame the button transitions to up (edge).
        [[nodiscard]] bool released(const input_snapshot& in) const noexcept;

        /// @brief The player slot index (0..3).
        [[nodiscard]] int gamepad_index() const noexcept { return m_gamepad_index; }
        /// @brief The gamepad button queried.
        [[nodiscard]] sdlpp::gamepad_button button() const noexcept { return m_button; }

    private:
        int m_gamepad_index;
        sdlpp::gamepad_button m_button;
    };

} // namespace neutrino
