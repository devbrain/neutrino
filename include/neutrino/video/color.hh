//
// Created by igor on 26/10/2026.
//

#pragma once

/**
 * @file color.hh
 * @brief The engine's colour vocabulary: @ref color, @ref fcolor, the @ref colors palette and
 *        the free colour helpers.
 *
 * Engine-owned NAMES for sdlpp's colour types: aliases, not new types, so a neutrino::color IS
 * the sdlpp colour type and passes to sdlpp with no conversion. Client code spells colours
 * through neutrino and never needs to include or name sdlpp.
 *
 * @c color holds 8-bit channels (0..255); @c fcolor holds float channels (0..1). Converting
 * between them is explicit (@c color{fc}, @c fcolor{c}); float-to-byte conversion clamps and
 * truncates, so 0.5f becomes 127. Members: @c mix(other, t), @c premultiply(), @c luminance()
 * (BT.709), @c to_grayscale(), @c adjust_brightness(f), saturating @c + (alpha included) and
 * @c * (brightness; alpha untouched).
 *
 * The palette uses X11 colour names AND X11 values, which differ from CSS for a few names:
 * @c colors::gray is 190 (CSS gray is 128), @c colors::dark_gray is 169 -- lighter than CSS
 * gray -- and @c colors::green is (0, 255, 0) (CSS calls that @c lime). @c colors::f holds a
 * small @c fcolor subset (black, white, the primaries and secondaries, gray = 0.5, transparent).
 */

#include <sdlpp/video/color.hh>

namespace neutrino {
    /// @brief An 8-bit-per-channel RGBA colour.
    using color = sdlpp::color;   // sdlpp-ok
    /// @brief A float-per-channel RGBA colour (0..1).
    using fcolor = sdlpp::fcolor; // sdlpp-ok

    /// @brief The named colour palette (X11 names and values in lower_snake_case:
    ///        @c colors::white, @c colors::forest_green, ...). See the file comment for the
    ///        names whose X11 values differ from CSS.
    namespace colors = sdlpp::colors; // sdlpp-ok

    /// @brief Linear interpolation, @c a.mix(b, t); @p t in [0, 1].
    using sdlpp::lerp;        // sdlpp-ok
    /// @brief Composite @c src over @c dst with straight (non-premultiplied) alpha.
    using sdlpp::alpha_blend; // sdlpp-ok
    /// @brief Pack as 0xRRGGBBAA -- the layout of an RGBA8888 texture pixel
    ///        (texture::update(), texture::lock()).
    using sdlpp::to_rgba32;   // sdlpp-ok
    /// @brief Unpack 0xRRGGBBAA.
    using sdlpp::from_rgba32; // sdlpp-ok
}
