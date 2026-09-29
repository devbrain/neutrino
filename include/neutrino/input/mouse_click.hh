#pragma once

#include <neutrino/neutrino_export.h>
#include <neutrino/input/hotkey.hh>
#include <sdlpp/events/mouse_codes.hh>
#include <sdlpp/utility/geometry_types.hh>

namespace neutrino {

    class input_snapshot;

    /// @brief Mouse-button query, optionally qualified by keyboard modifiers.
    ///
    /// Evaluated against a frame's @ref input_snapshot. When constructed
    /// with a @ref modifier, the query only matches while exactly those modifier
    /// groups are held (same strict matching as @ref hotkey).
    class NEUTRINO_EXPORT mouse_click {
    public:
        /// @brief Match @p button with no modifier requirement.
        mouse_click(sdlpp::mouse_button button)
            : m_mods(modifier::none), m_button(button) {}

        /// @brief Match @p button only while @p mods is held.
        mouse_click(modifier mods, sdlpp::mouse_button button)
            : m_mods(mods), m_button(button) {}

        /// @brief True on the frame the button transitions to down (edge), with modifiers matching.
        [[nodiscard]] bool pressed(const input_snapshot& in) const noexcept;
        /// @brief True on every frame the button is down (level), with modifiers matching.
        [[nodiscard]] bool held(const input_snapshot& in) const noexcept;
        /// @brief True on the frame the button transitions to up (edge), with modifiers matching.
        [[nodiscard]] bool released(const input_snapshot& in) const noexcept;

        [[nodiscard]] modifier modifiers() const noexcept { return m_mods; }
        [[nodiscard]] sdlpp::mouse_button button() const noexcept { return m_button; }

    private:
        modifier m_mods;
        sdlpp::mouse_button m_button;
    };

} // namespace neutrino
