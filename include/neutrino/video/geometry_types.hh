//
// Created by igor on 04/07/2026.
//

#pragma once

/**
 * @file geometry_types.hh
 * @brief The engine's geometry vocabulary: integer screen space and float world space.
 *
 * Short aliases for the sdlpp geometry primitives, so code shares one coordinate type set rather
 * than spelling out @c sdlpp::point<int> and friends everywhere. Two spaces live here:
 *   - **integer screen/pixel space** (@ref point, @ref rect, ...) -- what the renderer draws in;
 *   - **float world space** (@ref world_point, @ref world_rect) -- what gameplay and physics use.
 *
 * This header is deliberately LIGHT (two sdlpp geometry headers, nothing else), because the world
 * aliases are needed almost everywhere. They previously lived in @c world/world_common.hh, the
 * tile-world header, which drags in @c <filesystem>, @c <map>, @c <variant>, @c <vector> and the
 * colour header -- so a consumer needing one float point paid for the whole tile-world vocabulary.
 * @c world_common.hh includes this header, so the names remain visible through it.
 */

#include <sdlpp/utility/geometry.hh>
#include <sdlpp/utility/dimension.hh>

namespace neutrino {
    using point = sdlpp::point<int>;    ///< A 2D integer screen-space position (x, y) in pixels.
    using line = sdlpp::line<int>;      ///< A segment between two integer screen-space endpoints.
    using rect = sdlpp::rect<int>;      ///< An axis-aligned integer rectangle (x, y, w, h) in pixels.
    using circle = sdlpp::circle<int>;  ///< A circle with integer centre and radius in pixels.
    using dim = sdlpp::size<int>;       ///< An integer size (width, height) in pixels.

    /// @brief A 2D position in world pixels; the floating-point counterpart of @ref point.
    using world_point = sdlpp::point<float>;
    /// @brief An axis-aligned rectangle in world pixels; the floating-point counterpart of @ref rect.
    using world_rect = sdlpp::rect<float>;
}