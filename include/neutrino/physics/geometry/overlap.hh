#pragma once

/**
 * @file overlap.hh
 * @brief Static overlap predicates and penetration (MTV) queries between two stationary
 *        2D shapes -- the boolean/penetration narrow-phase of @ref neutrino::physics.
 *
 * Provides three families of @c constexpr / @c inline queries over the shape vocabulary
 * defined in @ref types.hh (@ref neutrino::physics::aabb, @ref neutrino::physics::circle,
 * @ref neutrino::physics::segment):
 *   - @ref neutrino::physics::contains -- does a shape contain a point?
 *   - @ref neutrino::physics::intersects -- do two stationary shapes overlap or touch
 *     (a pure boolean test)?
 *   - @ref neutrino::physics::overlap -- the minimum translation vector
 *     (@ref neutrino::physics::penetration) that separates two strictly overlapping area
 *     shapes, or @c std::nullopt when they do not overlap.
 *
 * All predicates count touching (boundary contact) as overlap, whereas @ref
 * neutrino::physics::overlap treats touching as NO overlap (a zero MTV is not reported).
 *
 * @note The segment/segment boolean lives in <neutrino/physics/geometry/sweep.hh> next to its
 *       parametric implementation. Segments are 1-D and have no well-defined static MTV,
 *       so @ref neutrino::physics::overlap is defined only for the area shapes (aabb,
 *       circle); use the swept/parametric contact normal for segments instead.
 */

#include <cmath>
#include <optional>
#include <euler/vector/vector_ops.hh>
#include <neutrino/physics/geometry/types.hh>
#include <neutrino/physics/geometry/distance.hh>

namespace neutrino::physics {
    /**
     * @brief Test if an AABB contains a point
     * @param b Bounding box
     * @param p Point vector to test
     * @return true if point is inside or on the boundary of the box
     */
    [[nodiscard]] constexpr bool contains(const aabb& b, const vec& p) noexcept {
        return p.x() >= b.min.x() && p.x() <= b.max.x() &&
               p.y() >= b.min.y() && p.y() <= b.max.y();
    }

    /**
     * @brief Test if a circle contains a point
     * @param c Circle shape
     * @param p Point vector to test
     * @return true if point is inside or on the boundary of the circle
     */
    [[nodiscard]] constexpr bool contains(const circle& c, const vec& p) noexcept {
        const float dx = p.x() - c.center.x();
        const float dy = p.y() - c.center.y();
        return dx * dx + dy * dy <= c.radius * c.radius;
    }

    /**
     * @brief Test if a point lies on a (zero-width) segment, within POINT_EPS.
     * @param s Line segment
     * @param p Point vector to test
     * @return true if the point is within constants::POINT_EPS of the segment
     */
    [[nodiscard]] constexpr bool contains(const segment& s, const vec& p) noexcept {
        return squared_distance(p, s) <= constants::POINT_EPS * constants::POINT_EPS;
    }

    /**
     * @brief Test if two static AABBs intersect
     * @param a First bounding box
     * @param b Second bounding box
     * @return true if the boxes overlap or touch
     */
    [[nodiscard]] constexpr bool intersects(const aabb& a, const aabb& b) noexcept {
        return a.min.x() <= b.max.x() && a.max.x() >= b.min.x() &&
               a.min.y() <= b.max.y() && a.max.y() >= b.min.y();
    }

    /**
     * @brief Test if two static circles intersect
     * @param a First circle
     * @param b Second circle
     * @return true if the circles overlap or touch
     */
    [[nodiscard]] constexpr bool intersects(const circle& a, const circle& b) noexcept {
        const float dx = a.center.x() - b.center.x();
        const float dy = a.center.y() - b.center.y();
        const float r = a.radius + b.radius;
        return dx * dx + dy * dy <= r * r;
    }

    /**
     * @brief Test if a static circle and a static AABB intersect.
     *
     * This function determines if the circle `a` overlaps or touches the axis-aligned
     * bounding box `b` by checking if the squared distance from the circle center to
     * the AABB is within the squared radius of the circle.
     *
     * @param a The circle shape.
     * @param b The axis-aligned bounding box.
     * @return true if the shapes overlap or touch, false otherwise.
     */
    [[nodiscard]] constexpr bool intersects(const circle& a, const aabb& b) noexcept {
        return squared_distance(a.center, b) <= a.radius * a.radius;
    }

    /// @brief Argument-order-independent overload of @ref intersects(const circle&, const aabb&).
    [[nodiscard]] constexpr bool intersects(const aabb& a, const circle& b) noexcept {
        return intersects(b, a);
    }

    /**
     * @brief Test if a finite segment and a circle overlap or touch.
     *
     * True when the shortest distance from the circle centre to the segment is within
     * the circle radius.
     *
     * @param s The line segment.
     * @param c The circle shape.
     * @return true if the shapes overlap or touch, false otherwise.
     */
    [[nodiscard]] constexpr bool intersects(const segment& s, const circle& c) noexcept {
        return squared_distance(c.center, s) <= c.radius * c.radius;
    }

    /// @brief Argument-order-independent overload of @ref intersects(const segment&, const circle&).
    [[nodiscard]] constexpr bool intersects(const circle& c, const segment& s) noexcept {
        return intersects(s, c);
    }

    // -------------------------------------------------------------------------
    // Penetration / minimum translation vector (MTV) for overlapping shapes.
    //
    // overlap(a, b) returns the smallest translation that separates `a` from `b`
    // (see @ref penetration), or std::nullopt when they do not strictly overlap.
    // Defined for the area shapes (aabb, circle); segments are 1-D and have no
    // well-defined static MTV (use the swept/parametric contact normal instead).
    // -------------------------------------------------------------------------

    /**
     * @brief Minimum translation vector separating two overlapping AABBs (axis of least overlap).
     * @param a The first bounding box (the one the returned normal pushes out of @p b).
     * @param b The second bounding box.
     * @return A @ref penetration (normal points the way to move `a` out of `b`), or
     *         std::nullopt if the boxes do not strictly overlap (touching counts as no overlap).
     */
    [[nodiscard]] constexpr std::optional<penetration> overlap(const aabb& a, const aabb& b) noexcept {
        const float pen_neg_x = a.max.x() - b.min.x(); // push a in -x to clear b
        const float pen_pos_x = b.max.x() - a.min.x(); // push a in +x to clear b
        if (pen_neg_x <= 0.0f || pen_pos_x <= 0.0f) {
            return std::nullopt;
        }
        const float pen_neg_y = a.max.y() - b.min.y();
        const float pen_pos_y = b.max.y() - a.min.y();
        if (pen_neg_y <= 0.0f || pen_pos_y <= 0.0f) {
            return std::nullopt;
        }

        const bool x_neg = pen_neg_x < pen_pos_x;
        const float depth_x = x_neg ? pen_neg_x : pen_pos_x;
        const bool y_neg = pen_neg_y < pen_pos_y;
        const float depth_y = y_neg ? pen_neg_y : pen_pos_y;

        if (depth_x <= depth_y) {
            return penetration{vec{x_neg ? -1.0f : 1.0f, 0.0f}, depth_x};
        }
        return penetration{vec{0.0f, y_neg ? -1.0f : 1.0f}, depth_y};
    }

    /**
     * @brief Minimum translation vector separating two overlapping circles.
     * @param a The first circle (the one the returned normal pushes out of @p b).
     * @param b The second circle.
     * @return A @ref penetration (normal points from `b` toward `a`), or std::nullopt
     *         if the circles do not strictly overlap. Concentric circles get an arbitrary axis.
     */
    [[nodiscard]] inline std::optional<penetration> overlap(const circle& a, const circle& b) noexcept {
        const vec d = a.center - b.center; // from b toward a
        const float dist_sq = euler::dot(d, d);
        const float r = a.radius + b.radius;
        if (dist_sq >= r * r) {
            return std::nullopt;
        }
        const float dist = euler::length(d);
        if (dist > constants::NORMALIZE_EPS) {
            return penetration{d / dist, r - dist};
        }
        return penetration{vec{1.0f, 0.0f}, r}; // concentric: arbitrary axis
    }

    /**
     * @brief Minimum translation vector separating an overlapping circle and AABB.
     * @param c The circle (the one the returned normal pushes out of @p b).
     * @param b The axis-aligned bounding box.
     * @return A @ref penetration (normal points the way to move the circle `c` out of `b`),
     *         or std::nullopt if they do not strictly overlap. When the centre is inside the
     *         box the circle is pushed out through the nearest face.
     */
    [[nodiscard]] inline std::optional<penetration> overlap(const circle& c, const aabb& b) noexcept {
        const vec q = closest_point(c.center, b); // closest point on/in the box
        const vec d = c.center - q;
        const float dist_sq = euler::dot(d, d);

        // closest_point clamps componentwise, so q == c.center exactly iff the centre is
        // inside/on the box; therefore dist_sq > 0 means the centre is strictly outside.
        if (dist_sq > 0.0f) {
            // Centre outside the box: overlap only when within the radius (touching/miss -> none).
            if (dist_sq >= c.radius * c.radius) {
                return std::nullopt;
            }
            const float dist = euler::length(d); // > 0, so the normalize is safe
            return penetration{d / dist, c.radius - dist};
        }

        // Centre inside/on the box: exit through the nearest face.
        float min_d = c.center.x() - b.min.x();
        vec n{-1.0f, 0.0f};
        if (const float dr = b.max.x() - c.center.x(); dr < min_d) { min_d = dr; n = vec{1.0f, 0.0f}; }
        if (const float dy0 = c.center.y() - b.min.y(); dy0 < min_d) { min_d = dy0; n = vec{0.0f, -1.0f}; }
        if (const float dy1 = b.max.y() - c.center.y(); dy1 < min_d) { min_d = dy1; n = vec{0.0f, 1.0f}; }
        const float depth = c.radius + min_d;
        if (depth <= 0.0f) {
            return std::nullopt; // e.g. a zero-radius centre exactly on a face: no penetration
        }
        return penetration{n, depth};
    }

    /// @brief Argument-order-independent overload of @ref overlap(const circle&, const aabb&)
    ///        (normal is negated to point the way to move the AABB out of the circle).
    [[nodiscard]] inline std::optional<penetration> overlap(const aabb& b, const circle& c) noexcept {
        auto p = overlap(c, b);
        if (p) {
            p->normal = -p->normal;
        }
        return p;
    }
} // namespace neutrino::physics
