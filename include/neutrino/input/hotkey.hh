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

    // Ergonomic aliases, scoped to avoid collisions with user code and
    // platform headers (SHIFT/CTRL/... are common macro/constant names).
    namespace mods {
        inline constexpr modifier none   = modifier::none;
        inline constexpr modifier lshift = modifier::lshift;
        inline constexpr modifier rshift = modifier::rshift;
        inline constexpr modifier shift  = modifier::shift;
        inline constexpr modifier lctrl  = modifier::lctrl;
        inline constexpr modifier rctrl  = modifier::rctrl;
        inline constexpr modifier ctrl   = modifier::ctrl;
        inline constexpr modifier lalt   = modifier::lalt;
        inline constexpr modifier ralt   = modifier::ralt;
        inline constexpr modifier alt    = modifier::alt;
        inline constexpr modifier lgui   = modifier::lgui;
        inline constexpr modifier rgui   = modifier::rgui;
        inline constexpr modifier gui    = modifier::gui;
    }

    /// @brief Keyboard hotkey query (polled, not event-driven).
    ///
    /// Modifier matching is strict:
    /// - hotkey(scancode::a) fires only while NO modifier is held.
    /// - hotkey(mods::ctrl, key) accepts either ctrl (but no other groups).
    /// - hotkey(mods::lshift, key) requires specifically left shift; it does
    ///   not fire while right shift is also down.
    /// - A hotkey on a modifier key itself (e.g. scancode::lshift) ignores
    ///   that key's own modifier bit when matching.
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
