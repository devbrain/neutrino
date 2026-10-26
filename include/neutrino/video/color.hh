//
// Created by igor on 26/10/2026.
//

#pragma once

/**
 * @file color.hh
 * @brief The engine's colour vocabulary: @ref color, @ref fcolor and the @ref colors palette.
 *
 * Engine-owned NAMES for sdlpp's colour types (roadmap P1.1 / P1.7): aliases, not new types, so a
 * neutrino::color IS the sdlpp colour type and passes to sdlpp with no conversion. Client code spells
 * colours through neutrino and never needs to include or name sdlpp.
 */

#include <sdlpp/video/color.hh>

namespace neutrino {
    /// @brief An 8-bit-per-channel RGBA colour.
    using color = sdlpp::color;   // sdlpp-ok
    /// @brief A float-per-channel RGBA colour (0..1).
    using fcolor = sdlpp::fcolor; // sdlpp-ok

    /// @brief The named colour palette (X11 names in lower_snake_case: @c colors::white,
    ///        @c colors::forest_green, ...).
    namespace colors = sdlpp::colors; // sdlpp-ok
}
