#pragma once

#include <cstdint>
#include <variant>
#include <neutrino/neutrino_export.h>
#include <sdlpp/events/keyboard_codes.hh>

namespace neutrino {

    enum class modifier : std::uint32_t {
        none       = 0,
        lshift     = 1 << 0,
        rshift     = 1 << 1,
        shift      = lshift | rshift,

        lctrl      = 1 << 2,
        rctrl      = 1 << 3,
        ctrl       = lctrl | rctrl,

        lalt       = 1 << 4,
        ralt       = 1 << 5,
        alt        = lalt | ralt,

        lgui       = 1 << 6,
        rgui       = 1 << 7,
        gui        = lgui | rgui
    };

    // Bitwise operators for modifier flags
    inline modifier operator|(modifier lhs, modifier rhs) {
        return static_cast<modifier>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
    }

    inline modifier operator&(modifier lhs, modifier rhs) {
        return static_cast<modifier>(static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs));
    }

    inline modifier operator~(modifier mod) {
        return static_cast<modifier>(~static_cast<std::uint32_t>(mod));
    }

    // Ergonomic aliases
    constexpr modifier NONE   = modifier::none;
    constexpr modifier LSHIFT = modifier::lshift;
    constexpr modifier RSHIFT = modifier::rshift;
    constexpr modifier SHIFT  = modifier::shift;
    constexpr modifier LCTRL  = modifier::lctrl;
    constexpr modifier RCTRL  = modifier::rctrl;
    constexpr modifier CTRL   = modifier::ctrl;
    constexpr modifier LALT   = modifier::lalt;
    constexpr modifier RALT   = modifier::ralt;
    constexpr modifier ALT    = modifier::alt;
    constexpr modifier LGUI   = modifier::lgui;
    constexpr modifier RGUI   = modifier::rgui;
    constexpr modifier GUI    = modifier::gui;

    class NEUTRINO_EXPORT hotkey {
    public:
        // Scancode constructors
        hotkey(sdlpp::scancode key)
            : m_mods(modifier::none), m_key(key) {}

        hotkey(modifier mods, sdlpp::scancode key)
            : m_mods(mods), m_key(key) {}

        // Keycode constructors
        hotkey(sdlpp::keycode key)
            : m_mods(modifier::none), m_key(key) {}

        hotkey(modifier mods, sdlpp::keycode key)
            : m_mods(mods), m_key(key) {}

        // Checking methods
        [[nodiscard]] bool pressed() const noexcept;
        [[nodiscard]] bool held() const noexcept;
        [[nodiscard]] bool released() const noexcept;

        // Default operator bool maps to pressed()
        [[nodiscard]] explicit operator bool() const noexcept {
            return pressed();
        }

    private:
        [[nodiscard]] sdlpp::scancode resolve_scancode() const noexcept;

        modifier m_mods;
        std::variant<sdlpp::scancode, sdlpp::keycode> m_key;
    };

} // namespace neutrino
