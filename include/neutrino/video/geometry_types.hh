//
// Created by igor on 04/07/2026.
//

#pragma once

/**
 * @file geometry_types.hh
 * @brief The engine's integer screen-space geometry vocabulary.
 *
 * Short aliases for the sdlpp geometry primitives, all specialized on @c int,
 * so render/video code shares one pixel-space coordinate type set (rather than
 * spelling out @c sdlpp::point<int> and friends everywhere).
 */

#include <sdlpp/utility/geometry.hh>
#include <sdlpp/utility/dimension.hh>

namespace neutrino {
    using point = sdlpp::point<int>;    ///< A 2D integer screen-space position (x, y) in pixels.
    using line = sdlpp::line<int>;      ///< A segment between two integer screen-space endpoints.
    using rect = sdlpp::rect<int>;      ///< An axis-aligned integer rectangle (x, y, w, h) in pixels.
    using circle = sdlpp::circle<int>;  ///< A circle with integer centre and radius in pixels.
    using dim = sdlpp::size<int>;       ///< An integer size (width, height) in pixels.
}