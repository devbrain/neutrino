#pragma once

#include <neutrino/neutrino_export.h>
#include <neutrino/input/hotkey.hh>
#include <sdlpp/events/mouse_codes.hh>
#include <sdlpp/utility/geometry_types.hh>

namespace neutrino {

    class NEUTRINO_EXPORT mouse_click {
    public:
        mouse_click(sdlpp::mouse_button button)
            : m_mods(modifier::none), m_button(button) {}

        mouse_click(modifier mods, sdlpp::mouse_button button)
            : m_mods(mods), m_button(button) {}

        // Checking methods
        [[nodiscard]] bool pressed() const noexcept;
        [[nodiscard]] bool held() const noexcept;
        [[nodiscard]] bool released() const noexcept;

        // Default operator bool maps to pressed()
        [[nodiscard]] explicit operator bool() const noexcept {
            return pressed();
        }

    private:
        modifier m_mods;
        sdlpp::mouse_button m_button;
    };

    // Global mouse state retrieval helper functions
    [[nodiscard]] NEUTRINO_EXPORT int mouse_x() noexcept;
    [[nodiscard]] NEUTRINO_EXPORT int mouse_y() noexcept;
    [[nodiscard]] NEUTRINO_EXPORT sdlpp::point_i mouse_pos() noexcept;
    [[nodiscard]] NEUTRINO_EXPORT int mouse_wheel() noexcept;

} // namespace neutrino
