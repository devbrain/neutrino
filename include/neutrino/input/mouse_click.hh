#pragma once

#include <neutrino/neutrino_export.h>
#include <neutrino/input/hotkey.hh>
#include <sdlpp/events/mouse_codes.hh>
#include <sdlpp/utility/geometry_types.hh>

namespace neutrino {

    /// @brief Polled mouse-button query, optionally qualified by keyboard modifiers.
    ///
    /// Reports the current frame's state of one mouse button. When constructed
    /// with a @ref modifier, the query only matches while exactly those modifier
    /// groups are held (same strict matching as @ref hotkey). All queries read
    /// live application state and return false when no application is running.
    class NEUTRINO_EXPORT mouse_click {
    public:
        /// @brief Match @p button with no modifier requirement.
        mouse_click(sdlpp::mouse_button button)
            : m_mods(modifier::none), m_button(button) {}

        /// @brief Match @p button only while @p mods is held.
        mouse_click(modifier mods, sdlpp::mouse_button button)
            : m_mods(mods), m_button(button) {}

        /// @brief True on the frame the button transitions to down (edge), with modifiers matching.
        [[nodiscard]] bool pressed() const noexcept;
        /// @brief True on every frame the button is down (level), with modifiers matching.
        [[nodiscard]] bool held() const noexcept;
        /// @brief True on the frame the button transitions to up (edge), with modifiers matching.
        [[nodiscard]] bool released() const noexcept;

        /// @brief Equivalent to pressed(); lets a mouse_click be used directly in a condition.
        [[nodiscard]] explicit operator bool() const noexcept {
            return pressed();
        }

    private:
        modifier m_mods;
        sdlpp::mouse_button m_button;
    };

    /// @brief Current mouse X position, in window coordinates (0 if no application is running).
    [[nodiscard]] NEUTRINO_EXPORT int mouse_x() noexcept;
    /// @brief Current mouse Y position, in window coordinates (0 if no application is running).
    [[nodiscard]] NEUTRINO_EXPORT int mouse_y() noexcept;
    /// @brief Current mouse position as a point, in window coordinates ({0,0} if no application is running).
    [[nodiscard]] NEUTRINO_EXPORT sdlpp::point_i mouse_pos() noexcept;
    /// @brief Mouse wheel delta accumulated for the current frame (0 if no application is running).
    [[nodiscard]] NEUTRINO_EXPORT int mouse_wheel() noexcept;

} // namespace neutrino
