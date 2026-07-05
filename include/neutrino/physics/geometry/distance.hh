/**
// Created by igor on 22/06/2026.
//
 * @file distance.hh
 * @brief Distance and closest-point queries between the collision shapes -- point-vs-shape
 *        projections plus the shape-to-shape squared-distance table.
 *
 * Two layers live here:
 *   - Point queries: @ref neutrino::physics::closest_parameter / @ref
 *     neutrino::physics::closest_point (project a point onto a segment / AABB / circle) and the
 *     @ref neutrino::physics::squared_distance point-vs-shape overloads.
 *   - Shape-to-shape distance: @ref neutrino::physics::squared_distance between two FILLED regions
 *     -- 0 when the shapes overlap or touch, otherwise the squared shortest gap. A filled circle
 *     is its centre inflated by its radius, so every circle pair reduces to a core
 *     point/segment/aabb distance minus the radii, clamped at 0.
 *
 * @note All distances are returned SQUARED (no @c sqrt) to keep the queries branch-cheap and
 *       comparable without a root; take @c std::sqrt at the call site only if a true distance is
 *       needed.
 * @note The segment/segment and segment/aabb pairs (which need an "intersecting => 0" test) live
 *       in @c <neutrino/physics/geometry/sweep.hh>, not here.
 */
#pragma once

#include <cmath>
#include <algorithm>
#include <euler/vector/vector_ops.hh>
#include <neutrino/physics/geometry/types.hh>

namespace neutrino::physics {
    /**
     * @brief Calculates the projection parameter of a point onto a line segment, clamped to [0.0, 1.0].
     *
     * This function projects the target point onto the line containing the segment and returns the
     * normalized interpolation fraction t. If the segment has zero length (degenerate), it returns 0.0f.
     * The returned value is clamped to the range [0.0f, 1.0f], representing the closest point along the
     * finite segment boundaries.
     *
     * @param dst The query point vector.
     * @param s The line segment.
     * @return The normalized projection parameter in the range [0.0f, 1.0f].
     */
    [[nodiscard]] constexpr float closest_parameter(const vec& dst, const segment& s) noexcept {
        const vec d = s.to - s.from;
        if (euler::approx_zero(d, constants::POINT_EPS)) {
            return 0.0f;
        }
        const float len_sq = euler::dot(d, d);
        return std::clamp(euler::dot(dst - s.from, d) / len_sq, 0.0f, 1.0f);
    }

    /**
     * @brief Finds the closest point on a line segment to a query point.
     *
     * This function calculates the coordinates of the point on the finite segment `s` that is nearest
     * to the query point `dst`. It evaluates the closest clamped parameter along the segment.
     *
     * @param dst The query point vector.
     * @param s The line segment.
     * @return The coordinate vector of the closest point on the segment.
     */
    [[nodiscard]] constexpr vec closest_point(const vec& dst, const segment& s) noexcept {
        return s.point_in_time(closest_parameter(dst, s));
    }

    /**
     * @brief Finds the closest point on an AABB to a query point.
     *
     * This function projects the target point `dst` onto the boundary or interior
     * of the axis-aligned bounding box `s` by clamping its coordinates.
     *
     * @param dst The query point vector.
     * @param s The axis-aligned bounding box.
     * @return The coordinate vector of the closest point on or inside the AABB.
     */
    [[nodiscard]] constexpr vec closest_point(const vec& dst, const aabb& s) noexcept {
        return {
            std::clamp(dst.x(), s.min.x(), s.max.x()),
            std::clamp(dst.y(), s.min.y(), s.max.y())
        };
    }

    /**
     * @brief Calculates the squared distance between a point and a line segment.
     *
     * This function computes the shortest squared Euclidean distance from the query
     * point `p` to any point on the finite segment `s`.
     *
     * @param p The query point vector.
     * @param s The line segment.
     * @return The squared distance as a float.
     */
    [[nodiscard]] constexpr float squared_distance(const vec& p, const segment& s) noexcept {
        const vec ab = s.to - s.from;
        const vec ac = p - s.from;
        const vec bc = p - s.to;

        const float e = euler::dot(ac, ab);
        if (e <= 0.0f) {
            return euler::dot(ac, ac);
        }
        const float f = euler::dot(ab, ab);
        if (e >= f) {
            return euler::dot(bc, bc);
        }
        // Interior case: perpendicular distance squared. Use the cross-product form
        // (|ab x ac|^2 / |ab|^2) rather than |ac|^2 - e^2/f, which loses all precision to
        // catastrophic cancellation when |ac| is large and the perpendicular offset small.
        const float cr = euler::cross(ab, ac);
        return cr * cr / f;
    }

    /**
     * @brief Calculates the squared distance between a point and an AABB.
     *
     * This function computes the shortest squared Euclidean distance from the query
     * point `p` to the boundaries of the axis-aligned bounding box `s`. Returns 0.0f
     * if the point is inside the box.
     *
     * @param p The query point vector.
     * @param s The axis-aligned bounding box.
     * @return The squared distance as a float.
     */
    [[nodiscard]] constexpr float squared_distance(const vec& p, const aabb& s) noexcept {
        float sq_dist = 0.0f;
        auto eval = [&sq_dist](float v, float min, float max) {
            if (v < min) {
                const float d = min - v;
                sq_dist += d * d;
            }
            if (v > max) {
                const float d = max - v;
                sq_dist += d * d;
            }
        };

        eval(p.x(), s.min.x(), s.max.x());
        eval(p.y(), s.min.y(), s.max.y());

        return sq_dist;
    }

    /**
     * @brief Calculates the squared distance between a point and a circle.
     *
     * This function computes the shortest squared Euclidean distance from the query
     * point `p` to the boundaries of the circle `s`. Returns 0.0f if the point is
     * inside or on the boundary of the circle.
     *
     * @param p The query point vector.
     * @param s The circle shape.
     * @return The squared distance as a float.
     */
    [[nodiscard]] inline float squared_distance(const vec& p, const circle& s) noexcept {
        const vec dir = s.center - p;
        const float dist_sq = euler::dot(dir, dir);
        if (dist_sq <= s.radius * s.radius) {
            return 0.0f;
        }
        const float d = euler::length(dir) - s.radius;
        return d * d;
    }

    /**
     * @brief Finds the closest point on a circle to a query point.
     *
     * This function calculates the coordinates of the point on the circle `s` that
     * is nearest to the query point `p`. If the point is inside the circle, it returns
     * the point itself.
     *
     * @param p The query point vector.
     * @param s The circle shape.
     * @return The coordinate vector of the closest point on or inside the circle.
     */
    [[nodiscard]] inline vec closest_point(const vec& p, const circle& s) noexcept {
        const vec dir = p - s.center;
        if (euler::dot(dir, dir) <= s.radius * s.radius) {
            return p;
        }
        const float len = euler::length(dir);
        return s.center + dir * (s.radius / len);
    }

    // -------------------------------------------------------------------------
    // Shape-to-shape squared distance.
    //
    // Distance between the *filled* regions: 0 when the shapes overlap or touch,
    // otherwise the squared shortest gap. A filled circle is its centre inflated by
    // its radius, so every circle pair reduces to a core point/segment/aabb distance
    // minus the radii, clamped at 0. The segment/segment and segment/aabb pairs (which
    // need an "intersecting => 0" test) live in <neutrino/physics/geometry/sweep.hh>.
    // -------------------------------------------------------------------------

    /// @brief Squared distance between a point and a segment, argument-order flipped.
    [[nodiscard]] constexpr float squared_distance(const segment& s, const vec& p) noexcept {
        return squared_distance(p, s);
    }

    /// @brief Squared distance between a point and an AABB, argument-order flipped.
    [[nodiscard]] constexpr float squared_distance(const aabb& s, const vec& p) noexcept {
        return squared_distance(p, s);
    }

    /// @brief Squared distance between a point and a circle, argument-order flipped.
    [[nodiscard]] inline float squared_distance(const circle& s, const vec& p) noexcept {
        return squared_distance(p, s);
    }

    /**
     * @brief Squared distance between two AABBs (0 if they overlap or touch).
     */
    [[nodiscard]] constexpr float squared_distance(const aabb& a, const aabb& b) noexcept {
        const float gx = std::max({0.0f, a.min.x() - b.max.x(), b.min.x() - a.max.x()});
        const float gy = std::max({0.0f, a.min.y() - b.max.y(), b.min.y() - a.max.y()});
        return gx * gx + gy * gy;
    }

    /**
     * @brief Squared distance between two circles (0 if they overlap or touch).
     */
    [[nodiscard]] inline float squared_distance(const circle& a, const circle& b) noexcept {
        const float gap = euler::length(a.center - b.center) - a.radius - b.radius;
        return gap > 0.0f ? gap * gap : 0.0f;
    }

    /**
     * @brief Squared distance between a circle and an AABB (0 if they overlap or touch).
     */
    [[nodiscard]] inline float squared_distance(const circle& c, const aabb& b) noexcept {
        const float gap = std::sqrt(squared_distance(c.center, b)) - c.radius;
        return gap > 0.0f ? gap * gap : 0.0f;
    }

    /// @brief Argument-order-independent overload of @ref squared_distance(const circle&, const aabb&).
    [[nodiscard]] inline float squared_distance(const aabb& b, const circle& c) noexcept {
        return squared_distance(c, b);
    }

    /**
     * @brief Squared distance between a circle and a segment (0 if they overlap or touch).
     */
    [[nodiscard]] inline float squared_distance(const circle& c, const segment& s) noexcept {
        const float gap = std::sqrt(squared_distance(c.center, s)) - c.radius;
        return gap > 0.0f ? gap * gap : 0.0f;
    }

    /// @brief Argument-order-independent overload of @ref squared_distance(const circle&, const segment&).
    [[nodiscard]] inline float squared_distance(const segment& s, const circle& c) noexcept {
        return squared_distance(c, s);
    }
} // namespace neutrino::physics
