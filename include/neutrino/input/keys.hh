//
// Created by igor on 26/10/2026.
//

#pragma once

/**
 * @file keys.hh
 * @brief The engine's input-code vocabulary: keys, mouse buttons, gamepad buttons and axes.
 *
 * Engine-owned NAMES for sdlpp's input enums (roadmap P1.2 / P1.7): aliases, not new types, so the
 * values pass to sdlpp unchanged. Client code writes @c neutrino::key::space and never names sdlpp.
 *
 * The gamepad enums are @ref pad_button / @ref pad_axis because @c gamepad_button is already the
 * name of the per-slot button matcher (see gamepad_button.hh) -- as @ref key feeds @c hotkey and
 * @ref mouse_button feeds @c mouse_click.
 */

#include <sdlpp/events/keyboard_codes.hh>
#include <sdlpp/events/mouse_codes.hh>
#include <sdlpp/input/gamepad.hh>

namespace neutrino {
    /// @brief A physical, layout-independent key (an SDL scancode): @c key::space, @c key::w, ...
    using key = sdlpp::scancode;               // sdlpp-ok
    /// @brief A layout-dependent virtual key code (an SDL keycode).
    using keycode = sdlpp::keycode;            // sdlpp-ok
    /// @brief A mouse button: @c mouse_button::left, @c ::middle, @c ::right, ...
    using mouse_button = sdlpp::mouse_button;  // sdlpp-ok
    /// @brief A gamepad button: @c pad_button::south, @c ::right_shoulder, ...
    using pad_button = sdlpp::gamepad_button;  // sdlpp-ok
    /// @brief A gamepad axis: @c pad_axis::leftx, @c ::right_trigger, ...
    using pad_axis = sdlpp::gamepad_axis;      // sdlpp-ok
}
