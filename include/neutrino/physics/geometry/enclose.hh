//
// Created by igor on 22/06/2026.
//

/**
 * @file enclose.hh
 * @brief Tight axis-aligned bounding box (@ref neutrino::physics::aabb) enclosure of the
 *        collision shape value types.
 *
 * Provides a small set of overloaded @c enclose() free functions, one per shape, each returning
 * the smallest @ref neutrino::physics::aabb that fully contains the shape. These compute the
 * broadphase bound of a primitive -- the box used to seed spatial-index lookups (see
 * @ref grid.hh and @ref dynamic_aabb_tree.hh) before a precise narrow-phase test.
 *
 * All overloads are @c constexpr and free of rounding/padding: the returned box hugs the shape
 * exactly (modulo floating-point representation). The @ref neutrino::physics::aabb overload is the
 * identity, present so generic code can call @c enclose() uniformly regardless of shape type.
 *
 * @note No @c enclose() overload is provided for @ref neutrino::physics::triangle here.
 * @see translate.hh for moving these same shapes by a vector.
 */

#pragma once

#include <algorithm>
#include <neutrino/physics/geometry/types.hh>

namespace neutrino::physics {
    /**
     * @brief Compute the tight axis-aligned bounding box of a directed line segment.
     * @param s The segment to enclose.
     * @return The smallest @ref aabb containing both endpoints @c s.from and @c s.to.
     */
    constexpr
    aabb enclose(const segment& s) {
        auto min_x = std::min(s.from.x(), s.to.x());
        auto min_y = std::min(s.from.y(), s.to.y());

        auto max_x = std::max(s.from.x(), s.to.x());
        auto max_y = std::max(s.from.y(), s.to.y());

        return {{min_x, min_y}, {max_x, max_y}};
    }

    /**
     * @brief Identity enclosure: an AABB encloses itself.
     *
     * Provided so generic code can call @c enclose() uniformly across shape types.
     *
     * @param b The box.
     * @return @p b unchanged.
     */
    constexpr
    aabb enclose(const aabb& b) {
        return b;
    }

    /**
     * @brief Compute the tight axis-aligned bounding box of a circle.
     * @param c The circle to enclose.
     * @return The smallest @ref aabb containing the circle: the @c 2*radius square
     *         centred on @c c.center.
     */
    constexpr
    aabb enclose(const circle& c) {
        auto min_x = c.center.x() - c.radius;
        auto min_y = c.center.y() - c.radius;
        auto max_x = c.center.x() + c.radius;
        auto max_y = c.center.y() + c.radius;

        return {{min_x, min_y}, {max_x, max_y}};
    }
} // namespace neutrino::physics
