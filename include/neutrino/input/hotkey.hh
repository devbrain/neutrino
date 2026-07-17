#pragma once

#include <cstdint>
#include <variant>
#include <neutrino/neutrino_export.h>
#include <sdlpp/events/keyboard_codes.hh>

namespace neutrino {

    /// @brief Keyboard modifier flags, combinable with the bitwise operators below.
    ///
    /// Each physical key has its own bit (l*/r*); the side-agnostic values
    /// (@ref shift, @ref ctrl, @ref alt, @ref gui) are the OR of their left and
    /// right bits and match either side. Used to qualify @ref hotkey and
    /// @ref mouse_click queries.
    enum class modifier : std::uint32_t {
        none       = 0,        ///< No modifier held.
        lshift     = 1 << 0,   ///< Left Shift only.
        rshift     = 1 << 1,   ///< Right Shift only.
        shift      = lshift | rshift,  ///< Either Shift key.

        lctrl      = 1 << 2,   ///< Left Ctrl only.
        rctrl      = 1 << 3,   ///< Right Ctrl only.
        ctrl       = lctrl | rctrl,    ///< Either Ctrl key.

        lalt       = 1 << 4,   ///< Left Alt only.
        ralt       = 1 << 5,   ///< Right Alt (AltGr) only.
        alt        = lalt | ralt,      ///< Either Alt key.

        lgui       = 1 << 6,   ///< Left GUI key (Windows/Command/Super).
        rgui       = 1 << 7,   ///< Right GUI key (Windows/Command/Super).
        gui        = lgui | rgui       ///< Either GUI key.
    };

    /// @brief Bitwise OR: combine modifier flags (e.g. @c ctrl|shift).
    inline modifier operator|(modifier lhs, modifier rhs) {
        return static_cast<modifier>(static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
    }

    /// @brief Bitwise AND: mask/test modifier flags.
    inline modifier operator&(modifier lhs, modifier rhs) {
        return static_cast<modifier>(static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs));
    }

    /// @brief Bitwise NOT: complement of a modifier flag set (used to clear bits, e.g. @c x & ~shift).
    inline modifier operator~(modifier mod) {
        return static_cast<modifier>(~static_cast<std::uint32_t>(mod));
    }

    /// @brief Lower-case aliases for the @c modifier enumerators.
    ///
    /// Ergonomic aliases (e.g. @c mods::ctrl) scoped in their own namespace to
    /// avoid collisions with user code and platform headers, where SHIFT/CTRL/...
    /// are common macro and constant names.
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
        /// @brief Match a physical key by scancode (layout-independent position), no modifier required.
        hotkey(sdlpp::scancode key)
            : m_mods(modifier::none), m_key(key) {}

        /// @brief Match a scancode only while @p mods is held.
        hotkey(modifier mods, sdlpp::scancode key)
            : m_mods(mods), m_key(key) {}

        /// @brief Match a logical key by keycode (current layout); resolved to a scancode at query time.
        hotkey(sdlpp::keycode key)
            : m_mods(modifier::none), m_key(key) {}

        /// @brief Match a keycode only while @p mods is held.
        hotkey(modifier mods, sdlpp::keycode key)
            : m_mods(mods), m_key(key) {}

        /// @brief True on the frame the key transitions to down (edge) with the modifier set matching.
        [[nodiscard]] bool pressed() const noexcept;
        /// @brief True on every frame the key is down (level) with the modifier set matching.
        [[nodiscard]] bool held() const noexcept;
        /// @brief True on the frame the key transitions to up (edge) with the modifier set matching.
        [[nodiscard]] bool released() const noexcept;

        /// @brief Equivalent to pressed(); lets a hotkey be used directly in a condition.
        [[nodiscard]] explicit operator bool() const noexcept {
            return pressed();
        }

    private:
        [[nodiscard]] sdlpp::scancode resolve_scancode() const noexcept;

        modifier m_mods;
        std::variant<sdlpp::scancode, sdlpp::keycode> m_key;
    };

} // namespace neutrino
