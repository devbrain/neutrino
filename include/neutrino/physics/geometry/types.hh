//
// Created by igor on 22/06/2026.
//

#pragma once

/**
 * @file types.hh
 * @brief Core collision value types plus the shared numeric tolerances -- the vocabulary
 *        every @ref neutrino::physics query is expressed in.
 *
 * Pure world-space types (float world units) with no display/dp/SDL dependency. Holds:
 *   - the @ref neutrino::physics::constants tolerances (epsilons) shared across the module;
 *   - the shape value types -- @ref neutrino::physics::aabb, @ref neutrino::physics::circle,
 *     @ref neutrino::physics::segment, @ref neutrino::physics::triangle;
 *   - the query result types -- @ref neutrino::physics::line_hit,
 *     @ref neutrino::physics::swept_hit, @ref neutrino::physics::penetration.
 *
 * @see <neutrino/physics/geometry/shapes.hh> for the module overview.
 */

#include <limits>
#include <euler/vector/vector.hh>
#include <euler/vector/vector_ops.hh>

namespace neutrino::physics {
    /**
     * @brief Shared numeric tolerances (epsilons) and float sentinels used across the
     *        collide module's predicates and parametric queries.
     *
     * Centralising these keeps the boolean, distance, clip and sweep queries consistent in
     * how they treat coincident points, parallel directions, tangency and degenerate normals.
     */
    namespace constants {
        /// Positive infinity sentinel (e.g. an empty/unbounded interval's upper bound).
        inline constexpr auto INF = std::numeric_limits <float>::infinity();
        /// Negative infinity sentinel (e.g. an empty/unbounded interval's lower bound).
        inline constexpr auto NEG_INF = -std::numeric_limits <float>::infinity();

        /// Length below which a vector/segment is treated as a zero-length point, and the
        /// position tolerance for treating two points as coincident (world units).
        inline constexpr auto POINT_EPS = 1e-6f;

        /// Relative tolerance for the parallel / collinear cross-product tests: two
        /// directions count as parallel when |cross(u, v)| <= PARALLEL_REL_EPS * |u| * |v|
        /// (the sine of the angle between them is within this bound).
        inline constexpr auto PARALLEL_REL_EPS = 1e-6f;

        /// Relative tolerance (in squared-distance units) separating circle/segment
        /// tangency from a miss: a hit requires dist^2 - r^2 <= TANGENT_REL_EPS * max(r^2, dist^2).
        inline constexpr auto TANGENT_REL_EPS = 1e-6f;

        /// Vector length below which a computed normal is considered undefined and
        /// returned as {0,0} rather than normalized (guards against divide-by-zero).
        inline constexpr auto NORMALIZE_EPS = 1e-6f;
    }

    /**
     * @brief Type alias for 2D float vector
     */
    using vec = euler::vec2f;

    /**
     * @brief Axis-aligned bounding box stored as opposite corners (min <= max component-wise).
     *
     * In standard coordinates, `min` and `max` correspond to the minimum and maximum
     * coordinate components of the box.
     *
     * @pre `min.x() <= max.x()` and `min.y() <= max.y()`. This invariant is the caller's
     *      responsibility; every query in this module (contains, intersects, the slab
     *      sweep, distance) assumes it and gives undefined results for an inverted box.
     */
    struct aabb {
        vec min{}; ///< Component-wise minimum corner of the box (e.g., bottom-left in y-up, top-left in y-down)
        vec max{}; ///< Component-wise maximum corner of the box (e.g., top-right in y-up, bottom-right in y-down)

        /**
         * @brief Get the size of the box (width, height).
         * @return A vector whose components are the box extents along x and y.
         */
        [[nodiscard]] constexpr vec size() const noexcept {
            return vec{max.x() - min.x(), max.y() - min.y()};
        }

        /**
         * @brief Get the center point of the box.
         * @return The midpoint of the min and max corners.
         */
        [[nodiscard]] constexpr vec center() const noexcept {
            return vec{(min.x() + max.x()) * 0.5f, (min.y() + max.y()) * 0.5f};
        }

        /**
         * @brief Area of the box (width * height).
         * @return The box area; the surface-area-heuristic cost metric used by the BVH broadphase.
         */
        [[nodiscard]] constexpr float area() const noexcept {
            return (max.x() - min.x()) * (max.y() - min.y());
        }

        /**
         * @brief The smallest AABB enclosing both @p a and @p b (their component-wise union).
         * @param a The first box.
         * @param b The second box.
         * @return The union box: component-wise min of the mins and max of the maxes.
         */
        [[nodiscard]] static constexpr aabb combine(const aabb& a, const aabb& b) {
            return {
                {std::min(a.min.x(), b.min.x()), std::min(a.min.y(), b.min.y())},
                {std::max(a.max.x(), b.max.x()), std::max(a.max.y(), b.max.y())}
            };
        }
    };

    /**
     * @brief Circle shape defined by a center point and a radius.
     *
     * @pre `radius >= 0`. A radius of 0 is supported (the circle degenerates to its
     *      centre point); queries assume a non-negative radius and are undefined otherwise.
     */
    struct circle {
        vec center{}; ///< Center position vector of the circle in world space
        float radius{0.0f}; ///< Radius of the circle in world units (>= 0)
    };

    /**
     * @brief Directed line segment from a start point to an end point.
     */
    struct segment {
        vec from; ///< Starting point of the segment in world space
        vec to; ///< Ending point of the segment in world space

        /**
         * @brief Interpolate a point along the segment at a given normalized fraction.
         * @param t Interpolation fraction. Typically in the range [0.0, 1.0], but values
         *          outside this range can be used to extrapolate along the infinite line.
         * @return The interpolated position vector.
         */
        [[nodiscard]] constexpr vec point_in_time(float t) const noexcept {
            // Component-wise (rather than `from + t * (to - from)`) so the result is a
            // constexpr scalar construction; euler's expression->vector conversion is not
            // constexpr. Numerically identical to the vector form.
            return vec{
                from.x() + t * (to.x() - from.x()),
                from.y() + t * (to.y() - from.y())
            };
        }
    };

    /**
     * @brief A solid (filled) triangle, defined by three vertices in any winding.
     *
     * Unlike @ref segment (a 1-D line), a triangle is a 2-D region: its interior counts as
     * solid for overlap/sweep queries. Used for free-standing solid slopes/ramps that must
     * block from every side (a single diagonal `segment` only blocks crossing the diagonal).
     * Assumed non-degenerate (the three vertices are not collinear); a degenerate triangle
     * behaves like its longest edge.
     */
    struct triangle {
        vec a{}; ///< First vertex
        vec b{}; ///< Second vertex
        vec c{}; ///< Third vertex
    };

    /**
     * @brief Result of a raw line/segment-vs-shape query (@ref intersect_param).
     *
     * `entry_param` / `exit_param` are parameters along the query segment's infinite
     * line P(t) = from + t * (to - from). They are NOT clamped to [0, 1]: a value < 0
     * or > 1 means the crossing lies outside the finite segment (e.g. the segment
     * starts inside the shape, or the line crosses beyond an endpoint). Feed a param
     * back to @ref segment::point_in_time to recover the world-space crossing point.
     */
    struct line_hit {
        float entry_param{}; ///< Line parameter where the line enters the shape
        float exit_param{}; ///< Line parameter where the line exits the shape
        vec entry_normal{}; ///< Unit normal pointing outwards at the entry point, or {0,0} when undefined
        vec exit_normal{}; ///< Unit normal pointing outwards at the exit point, or {0,0} when undefined

        /// @return true if the infinite line containing the segment overlaps the shape.
        [[nodiscard]] constexpr bool line_overlaps() const noexcept {
            return entry_param <= exit_param;
        }

        /// @return true if the finite segment [0, 1] overlaps the shape.
        [[nodiscard]] constexpr bool segment_overlaps() const noexcept {
            return line_overlaps() && entry_param <= 1.0f && exit_param >= 0.0f;
        }
    };

    /**
     * @brief Result of a swept continuous-collision query (@ref swept_intersection).
     *
     * `entry_time` / `exit_time` lie within [0, `time`], where `time` is the query-window
     * argument passed to the swept query -- and they carry the SAME unit as that argument.
     * Pass a duration in seconds and they come back in seconds; pass a normalized window of
     * 1.0 (as the collision @ref neutrino::physics::world does on its unit-step sweeps) and
     * they come back as a [0, 1] time-of-impact. This differs from @ref line_hit, whose
     * params are measured along the query segment's infinite line and can fall outside [0, 1].
     */
    struct swept_hit {
        float entry_time{}; ///< Time of first contact, within [0, time] (same unit as the query window)
        float exit_time{}; ///< Time of separation, within [0, time] (same unit as the query window)
        vec entry_normal{}; ///< Unit normal pointing outwards at first contact, or {0,0} when undefined
        vec exit_normal{}; ///< Unit normal pointing outwards at separation, or {0,0} when undefined
    };

    /**
     * @brief Minimum translation needed to separate two overlapping static shapes (@ref overlap).
     *
     * `normal` is the unit separation axis and `depth` the overlap distance along it, so the
     * minimum translation vector is `normal * depth`. By convention `normal` points the
     * direction the *first* argument must move to be pushed out of the second:
     *   a.center += overlap(a, b)->normal * overlap(a, b)->depth   // separates a from b
     */
    struct penetration {
        vec normal{}; ///< Unit separation axis (direction to move the first shape out of the second)
        float depth{}; ///< Overlap distance along `normal` (>= 0); MTV = normal * depth
    };
} // namespace neutrino::physics
