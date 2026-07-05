//
// Created by igor on 23/06/2026.
//

/**
 * @file translate.hh
 * @brief Rigid translation of the collision shape value types by a displacement vector
 *        (@ref neutrino::physics::translate).
 *
 * Provides a set of overloaded @c translate() free functions, one per type, each returning a copy
 * of the input shifted by a @ref neutrino::physics::vec offset. The translation is a pure rigid
 * shift: every point of the shape moves by the same vector, so shape, size and orientation are
 * preserved (a circle keeps its radius, an AABB keeps its extents, a segment keeps its direction
 * and length). All overloads are @c constexpr and build on the primitive @c vec overload.
 *
 * @see enclose.hh for the bounding boxes of these same shapes.
 */

#pragma once

#include <neutrino/physics/geometry/types.hh>

namespace neutrino::physics {
    /**
     * @brief Translate a point/vector by a displacement.
     * @param x The point to translate.
     * @param v The displacement vector.
     * @return @c x + v computed component-wise.
     */
    [[nodiscard]] constexpr vec translate(const vec& x, const vec& v) {
        return vec{x.x() + v.x(), x.y() + v.y()};
    }

    /**
     * @brief Translate an axis-aligned box by a displacement.
     * @param x The box to translate.
     * @param v The displacement vector.
     * @return A box with both corners shifted by @p v (extents unchanged; the
     *         @ref aabb min<=max invariant is preserved).
     */
    [[nodiscard]] constexpr aabb translate(const aabb& x, const vec& v) {
        return {translate(x.min, v), translate(x.max, v) };
    }

    /**
     * @brief Translate a circle by a displacement.
     * @param x The circle to translate.
     * @param v The displacement vector.
     * @return A circle with its centre shifted by @p v and the same radius.
     */
    [[nodiscard]] constexpr circle translate(const circle& x, const vec& v) {
        return {translate(x.center, v), x.radius };
    }

    /**
     * @brief Translate a directed segment by a displacement.
     * @param x The segment to translate.
     * @param v The displacement vector.
     * @return A segment with both endpoints shifted by @p v (direction and length
     *         unchanged).
     */
    [[nodiscard]] constexpr segment translate(const segment& x, const vec& v) {
        return {translate(x.from, v), translate(x.to, v) };
    }

} // namespace neutrino::physics