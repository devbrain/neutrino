#pragma once

/**
 * @file sweep.hh
 * @brief Swept / continuous-collision-detection (CCD) narrow phase: parametric intersection
 *        of static shapes and time-of-impact intersection of MOVING convex shapes.
 *
 * Two complementary families of queries live here, distinguished by their return type and units:
 *
 *   - @ref neutrino::physics::intersect_param -- the geometric primitive. It intersects the
 *     infinite line (or finite segment) carrying a swept path against a static shape (AABB,
 *     circle, segment) and returns a @ref neutrino::physics::line_hit: entry/exit parameters
 *     measured ALONG THE SEGMENT (typically in [0,1], but possibly outside it -- a negative
 *     entry means the path starts already inside) plus the outward contact normals at those
 *     two points. These are the per-shape-pair building blocks the swept queries dispatch to.
 *
 *   - @ref neutrino::physics::swept_intersection -- the CCD entry points. Each takes two moving
 *     shapes with their velocities and a time window, reduces the pair to a single moving point
 *     vs. a static Minkowski-sum shape (in the RELATIVE frame, only @c avel-bvel matters), runs
 *     the relevant @ref intersect_param primitive(s), and returns a @ref neutrino::physics::swept_hit:
 *     entry/exit times within [0, @c time] carrying the SAME unit as @c time (seconds if @c time is
 *     in seconds; a [0,1] time-of-impact when the caller passes a normalized window of 1.0, as the
 *     collision world does), plus entry/exit normals.
 *
 * The bridge between the two is @ref neutrino::physics::to_swept_hit, which clamps the raw
 * line-parameter interval to a finite [0,1] and scales it by @c time.
 *
 * Minkowski reductions used per shape pair:
 *   - AABB vs AABB     -- inflate one box by the other's half-size; sweep the first's center
 *                         against the resulting box (slab method).
 *   - circle vs circle -- inflate one circle by the other's radius; sweep a point (line vs circle).
 *   - circle vs AABB   -- the sum is a rounded rectangle (stadium): two inflated boxes + four
 *                         corner circles; sweep a point against the 6-shape union.
 *   - circle vs segment-- the sum is a capsule: a core box + two endpoint circles in the
 *                         segment's local frame; sweep a point against that union.
 *   - AABB vs segment  -- vertex/edge events: each segment endpoint swept vs the box and each
 *                         box corner swept vs the segment, anchored on actual overlap at the
 *                         window ends.
 * Because each Minkowski sum is convex, the swept point's crossing is a single interval whose
 * entry comes from the earliest-entering sub-shape and exit from the latest-exiting one.
 *
 * @note @ref intersect_param returns line PARAMETERS along the query segment (which may fall
 *       outside [0,1]); @ref swept_intersection returns TIMES clamped to [0, @c time], expressed in
 *       @c time's own unit. Do not confuse the two.
 */

// =============================================================================
// Parametric intersection (intersect_param) and continuous collision detection
// (swept_intersection). intersect_param returns line_hit (segment parameters);
// swept_intersection returns swept_hit (times in [0, time], in time's own unit).
// =============================================================================

#include <cmath>
#include <algorithm>
#include <optional>
#include <euler/vector/vector_ops.hh>
#include <neutrino/physics/geometry/types.hh>
#include <neutrino/physics/geometry/distance.hh>

namespace neutrino::physics {
    /**
     * @brief Convert a raw line_hit interval (segment parameters) into a swept_hit (times in @p time 's unit).
     *
     * Entry comes from @p entry_src, exit from @p exit_src (the same hit for a single convex
     * shape; distinct sub-shape hits for a composite/convex query). Each parameter is clamped
     * to a finite [0, 1] *before* scaling by @p time, so an already-overlapping start
     * (param == -inf) maps to 0 and a zero-duration query yields 0 rather than inf * 0 == NaN.
     *
     * @param entry_src Source hit supplying the entry parameter and entry normal.
     * @param exit_src  Source hit supplying the exit parameter and exit normal.
     * @param time      Window duration (seconds) the clamped [0,1] parameters are scaled by.
     * @return A @ref swept_hit with @c entry_time / @c exit_time in [0, @p time] and the
     *         carried-over entry/exit normals.
     */
    [[nodiscard]] constexpr swept_hit to_swept_hit(const line_hit& entry_src, const line_hit& exit_src,
                                                   float time) noexcept {
        return swept_hit{
            std::max(0.0f, entry_src.entry_param) * time,
            std::min(exit_src.exit_param, 1.0f) * time,
            entry_src.entry_normal,
            exit_src.exit_normal,
        };
    }

    /**
     * @brief Computes raw line-parameter intersection and normals for an AABB and a line segment.
     *
     * This function uses the slab method (Kay and Kajiya) to calculate where the line
     * containing the segment crosses the axis-aligned bounding box. It projects the
     * segment onto each axis and determines the entry/exit parameter intervals. The overlap
     * of the X-interval [t_min_x, t_max_x] and the Y-interval [t_min_y, t_max_y] yields the
     * entry and exit parameters of the AABB:
     *   - @c t_entry = max(t_min_x, t_min_y)
     *   - @c t_exit  = min(t_max_x, t_max_y)
     * An axis with zero displacement degenerates its slab to either the whole line (the origin
     * is inside that slab) or an empty interval (it is outside), so axis-aligned paths are
     * handled without division by zero.
     *
     * Slab method visual representation (for dx > 0, dy > 0):
     * @verbatim
     *                     - - - - - - - - - - - - - - - - - - - * ty_far (t_max_y)
     *                                                           /
     *                                                AABB (a)  /
     *                                                         /
     *                     +----------------------------------+
     *                     |                                 /|
     *                     |                                / |
     *                     |                               /  |
     *                     |                              /   |
     *                     |                             /    |
     *                     |                            /     |
     *                     |                           /      |
     *                     +--------------------------/-------+
     *                                               /|       x = a.max.x (tx_far / t_max_x)
     *                                              / |
     *                                             /  |
     *                     - - - - - - - - - - - -* - - - - - - - - - - -
     *                                           /    |
     *                                 ty_near  /     |
     *                                (t_min_y)       x = a.min.x (tx_near / t_min_x)
     *                                       /
     *                                      / segment (b)
     *                                     /
     * @endverbatim
     *
     * @note The returned `entry_param`/`exit_param` are parameters along the segment line
     *       and may lie outside [0, 1]:
     *       - @ref line_hit::line_overlaps is true iff the infinite line meets the AABB.
     *       - @ref line_hit::segment_overlaps is true iff the finite segment meets it.
     *       - `entry_param < 0 <= exit_param` means the segment starts inside the AABB.
     *
     * @param a The axis-aligned bounding box (target obstacle) to test against.
     * @param b The line segment representing the swept path of a point.
     * @return A @ref line_hit if the infinite line meets the AABB, or std::nullopt if it misses.
     */
    [[nodiscard]] constexpr std::optional <line_hit> intersect_param(const aabb& a, const segment& b) noexcept {
        const float dx = b.to.x() - b.from.x();
        const float dy = b.to.y() - b.from.y();

        float t_min_x, t_max_x;
        if (dx == 0.0f) {
            if (b.from.x() >= a.min.x() && b.from.x() <= a.max.x()) {
                t_min_x = constants::NEG_INF;
                t_max_x = constants::INF;
            } else {
                t_min_x = constants::INF;
                t_max_x = constants::NEG_INF;
            }
        } else {
            const float tx1 = (a.min.x() - b.from.x()) / dx;
            const float tx2 = (a.max.x() - b.from.x()) / dx;
            t_min_x = std::min(tx1, tx2);
            t_max_x = std::max(tx1, tx2);
        }

        float t_min_y, t_max_y;
        if (dy == 0.0f) {
            if (b.from.y() >= a.min.y() && b.from.y() <= a.max.y()) {
                t_min_y = constants::NEG_INF;
                t_max_y = constants::INF;
            } else {
                t_min_y = constants::INF;
                t_max_y = constants::NEG_INF;
            }
        } else {
            const float ty1 = (a.min.y() - b.from.y()) / dy;
            const float ty2 = (a.max.y() - b.from.y()) / dy;
            t_min_y = std::min(ty1, ty2);
            t_max_y = std::max(ty1, ty2);
        }

        // The governing axis is whichever slab produced the entry (max) / exit (min)
        // bound. Derive it from the comparison that selects the bound rather than
        // re-deriving by float equality on the result (ties go to x, matching std::max/min).
        const bool entry_on_x = t_min_x >= t_min_y;
        const bool exit_on_x = t_max_x <= t_max_y;
        const float t_entry = entry_on_x ? t_min_x : t_min_y;
        const float t_exit = exit_on_x ? t_max_x : t_max_y;

        if (t_entry > t_exit) {
            return std::nullopt; // infinite line misses the box
        }

        const vec entry_normal = entry_on_x
                                     ? vec{(dx >= 0.0f) ? -1.0f : 1.0f, 0.0f}
                                     : vec{0.0f, (dy >= 0.0f) ? -1.0f : 1.0f};
        const vec exit_normal = exit_on_x
                                    ? vec{(dx >= 0.0f) ? 1.0f : -1.0f, 0.0f}
                                     : vec{0.0f, (dy >= 0.0f) ? 1.0f : -1.0f};

        return line_hit{t_entry, t_exit, entry_normal, exit_normal};
    }

    /**
     * @brief Continuous collision detection (CCD) query for two moving AABBs over a time interval.
     *
     * This function checks if two moving axis-aligned bounding boxes (AABBs) collide
     * during a given time step. It uses a Minkowski sum formulation:
     * 1. The first box (`abox`) is shrunk to a single moving point (its center).
     * 2. The second box (`bbox`) is inflated by the half-dimensions of the first box to form a Minkowski "image" box.
     * 3. The relative velocity `avel - bvel` is used to trace a relative displacement segment over the time duration.
     * 4. The query is solved by checking if the moving point's path segment intersects the inflated "image" box.
     *
     * @note Time Clamping:
     *       If the two AABBs are already overlapping at the start of the time interval (t = 0), the raw query
     *       yields a negative entry parameter. Since the collision is already active, the returned `entry_time`
     *       is clamped to 0. The parameter is clamped to a finite [0, 1] *before* scaling by `time`, so a
     *       zero-duration query yields 0 rather than inf * 0 == NaN.
     *
     * @note Return Value Units:
     *       Unlike @ref intersect_param (which returns line parameters), this returns absolute collision
     *       times scaled by `time` (e.g. seconds). `entry_time`/`exit_time` lie within [0, time].
     *
     * @param abox First bounding box.
     * @param avel Velocity of the first bounding box (world units / s).
     * @param bbox Second bounding box.
     * @param bvel Velocity of the second bounding box (world units / s).
     * @param time Maximum time duration of the query (s).
     * @return A @ref swept_hit if a collision occurs within [0, time], or std::nullopt otherwise.
     */
    [[nodiscard]] constexpr std::optional <swept_hit> swept_intersection(const aabb& abox, const vec& avel,
                                                                         const aabb& bbox, const vec& bvel,
                                                                         float time) noexcept {
        const auto rel_speed = avel - bvel;
        const auto from = abox.center();
        const vec to = from + time * rel_speed;

        const vec abox_size = abox.size() / 2.0f;
        auto image = bbox;
        image.min = image.min - abox_size;
        image.max = image.max + abox_size;

        const auto it = intersect_param(image, {from, to});
        if (!it || !it->segment_overlaps()) {
            return std::nullopt;
        }
        return to_swept_hit(*it, *it, time);
    }

    /**
     * @brief Computes raw line-parameter intersection and normals between a circle and a segment.
     *
     * This function solves the quadratic equation representing the intersection of the line containing
     * segment `s` with the circle `circ`. It returns the entry and exit parameters (along the segment line)
     * and the outward-pointing unit normals at the intersection points.
     *
     * @note The returned `entry_param`/`exit_param` are parameters along the segment line.
     *       - std::nullopt means the infinite line does not intersect the circle.
     *       - Otherwise use @ref line_hit::segment_overlaps to test the finite segment.
     *
     * @param circ The circle shape.
     * @param s The line segment.
     * @return A @ref line_hit if the line meets the circle, or std::nullopt if it misses.
     */
    [[nodiscard]] inline std::optional <line_hit>
    intersect_param(const circle& circ, const segment& s) noexcept {
        /*
         * Closest-approach form (all vectors UPPERCASE). D = s.to - s.from,
         * X = s.from - circ.center. Rather than solving the quadratic via the
         * discriminant b^2 - 4ac — which catastrophically cancels for coordinates far
         * from the origin (both terms ~|coord|^4) and falsely reports tangents — work
         * from the geometry:
         *   - Closest parameter:        t_mid    = -dot(X, D) / dot(D, D)
         *   - Min squared distance:     dist_sq  = cross(X, D)^2 / dot(D, D)   (Lagrange)
         *   - Line meets circle where   (t - t_mid)^2 * dot(D, D) = r^2 - dist_sq,
         *     so the half-chord is      dt       = sqrt((r^2 - dist_sq) / dot(D, D)).
         * Intermediates are kept in double so the (small) geometric quantities survive
         * float rounding at large world coordinates.
         */
        const vec d = s.to - s.from;
        const vec x = s.from - circ.center;

        // Degenerate (zero-length) segment: the "sweep" is a static point. Handle this
        // first so the static case never reaches the chord math below.
        if (euler::approx_zero(d, constants::POINT_EPS)) {
            const float c = euler::length_squared(x) - circ.radius * circ.radius;
            if (c <= 0.0f) {
                // Point is inside or on the circle
                line_hit out;
                out.entry_param = constants::NEG_INF;
                out.exit_param = constants::INF;
                // Normals are undefined for a static interior point, leave as zero
                return out;
            }
            return std::nullopt; // Point is outside
        }

        const double dx = d.x(), dy = d.y();
        const double xx = x.x(), xy = x.y();
        const double dd = dx * dx + dy * dy;
        const double cross_xd = xx * dy - xy * dx;
        const double dist_sq = (cross_xd * cross_xd) / dd; // perpendicular distance^2 to the centre
        const double rr = static_cast<double>(circ.radius) * circ.radius;

        // Compare perpendicular distance to the radius in squared-distance units with a
        // scale-aware tolerance, so a true tangent (dist_sq == rr) survives float rounding
        // while a genuine far-field miss (dist_sq >> rr) is rejected.
        const double diff = rr - dist_sq;
        const double tol = static_cast<double>(constants::TANGENT_REL_EPS) * std::max(rr, dist_sq);
        if (diff < -tol) {
            return std::nullopt; // line misses the circle
        }

        const double t_mid = -(xx * dx + xy * dy) / dd;
        const double dt = (diff > 0.0) ? std::sqrt(diff / dd) : 0.0;

        line_hit out;
        out.entry_param = static_cast<float>(t_mid - dt);
        out.exit_param = static_cast<float>(t_mid + dt);

        // Outward-pointing unit normal at the circle boundary for a given segment parameter.
        auto outward_normal = [&](float t) {
            const vec v = s.point_in_time(t) - circ.center;
            const float len = euler::length(v);
            return (len > constants::NORMALIZE_EPS) ? (v / len) : vec{0.0f, 0.0f};
        };
        out.entry_normal = outward_normal(out.entry_param);
        out.exit_normal = outward_normal(out.exit_param);

        return out;
    }

    /**
     * @brief Computes the raw line-parameter intersection and normal between two segments.
     *
     * This function checks if the finite segment `a` intersects the finite segment `b`.
     * If they intersect, it returns the intersection parameter along segment `a` (as both
     * entry and exit parameters) and the unit normal of segment `b` pointing opposite to
     * `a`'s direction.
     *
     * @param a The first segment.
     * @param b The second segment.
     * @return A @ref line_hit if they intersect, or std::nullopt if they are parallel or do not overlap.
     */
    [[nodiscard]] inline std::optional <line_hit> intersect_param(const segment& a, const segment& b) noexcept {
        const vec r = a.to - a.from;
        const vec s_dir = b.to - b.from;
        const float len_sq_r = euler::dot(r, r);
        const float len_sq_s = euler::dot(s_dir, s_dir);

        // Degenerate (point) segments: the general cross-product path is vacuous when a
        // direction vector is ~0, so fall back to point/segment containment. The reported
        // parameter is along segment a, so a point b on a yields its parameter along a,
        // while a degenerate a is the single point at parameter 0.
        const bool a_point = euler::approx_zero(r, constants::POINT_EPS);
        const bool b_point = euler::approx_zero(s_dir, constants::POINT_EPS);
        if (a_point || b_point) {
            float param;
            if (a_point && b_point) {
                if (!euler::approx_equal(a.from, b.from, constants::POINT_EPS)) {
                    return std::nullopt;
                }
                param = 0.0f;
            } else if (a_point) {
                if (!euler::approx_equal(closest_point(a.from, b), a.from, constants::POINT_EPS)) {
                    return std::nullopt;
                }
                param = 0.0f;
            } else {
                // b is the point: locate it along segment a
                if (!euler::approx_equal(closest_point(b.from, a), b.from, constants::POINT_EPS)) {
                    return std::nullopt;
                }
                param = closest_parameter(b.from, a);
            }
            line_hit out;
            out.entry_param = param;
            out.exit_param = param;
            return out;
        }

        const float H = euler::cross(r, s_dir);

        const vec q = b.from - a.from;

        // Parallel test scaled by the segment magnitudes: |H| <= PARALLEL_REL_EPS*|r|*|s_dir|.
        // H has units of area, so an absolute epsilon would be scale-dependent.
        if (H * H <= constants::PARALLEL_REL_EPS * constants::PARALLEL_REL_EPS * len_sq_r * len_sq_s) {
            // Check if they are collinear, again scaled: |cross(q, r)| <= PARALLEL_REL_EPS*|q|*|r|.
            const float cross_q_r = euler::cross(q, r);
            const float len_sq_q = euler::dot(q, q);
            if (cross_q_r * cross_q_r <= constants::PARALLEL_REL_EPS * constants::PARALLEL_REL_EPS * len_sq_q * len_sq_r) {
                // Project onto r to check interval overlap
                const float t_from = euler::dot(q, r);
                const float t_to = euler::dot(b.to - a.from, r);
                const float t_min = std::min(t_from, t_to);
                const float t_max = std::max(t_from, t_to);

                if (t_min <= len_sq_r && t_max >= 0.0f) {
                    line_hit out;
                    out.entry_param = std::max(0.0f, t_min / len_sq_r);
                    out.exit_param = std::min(1.0f, t_max / len_sq_r);

                    vec normal{-r.y(), r.x()};
                    const float n_len = euler::length(normal);
                    if (n_len > constants::NORMALIZE_EPS) {
                        normal = normal / n_len;
                    }
                    // s_dir is parallel to r here, so dot(normal, s_dir) is ~0 and cannot
                    // orient the normal. Use the offset q between the segments instead so
                    // the normal points deterministically away from b toward a.
                    if (euler::dot(normal, q) > 0.0f) {
                        normal = -normal;
                    }
                    out.entry_normal = normal;
                    out.exit_normal = normal;
                    return out;
                }
            }
            return std::nullopt;
        }

        const float t = euler::cross(q, s_dir) / H;
        const float u = euler::cross(q, r) / H;

        // Check if the intersection point lies on both finite segments
        if (t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f) {
            line_hit out;
            out.entry_param = t;
            out.exit_param = t;

            // Compute the normal of segment b pointing opposite to segment a's direction
            vec normal{-s_dir.y(), s_dir.x()};
            const float len = euler::length(normal);
            if (len > constants::NORMALIZE_EPS) {
                normal = normal / len;
            }
            if (euler::dot(normal, r) > 0.0f) {
                normal = -normal;
            }
            out.entry_normal = normal;
            out.exit_normal = normal;
            return out;
        }

        return std::nullopt;
    }

    /**
     * @brief Test if two segments intersect.
     *
     * @param a The first segment.
     * @param b The second segment.
     * @return true if the finite segments intersect, false otherwise.
     */
    [[nodiscard]] inline bool intersects(const segment& a, const segment& b) noexcept {
        return intersect_param(a, b).has_value();
    }

    /**
     * @brief Test if a finite segment overlaps an AABB.
     *
     * True when the segment crosses the box or lies (partly) inside it.
     *
     * @param s The line segment.
     * @param b The axis-aligned bounding box.
     * @return true if the finite segment intersects/overlaps the AABB.
     */
    [[nodiscard]] constexpr bool intersects(const segment& s, const aabb& b) noexcept {
        const auto hit = intersect_param(b, s);
        return hit && hit->segment_overlaps();
    }

    /// @brief Argument-order-independent overload of @ref intersects(const segment&, const aabb&).
    [[nodiscard]] constexpr bool intersects(const aabb& b, const segment& s) noexcept {
        return intersects(s, b);
    }

    /**
     * @brief Squared distance between two finite segments (0 if they intersect).
     *
     * In 2D the closest pair of two non-intersecting segments always includes an
     * endpoint, so the distance reduces to the minimum of the four endpoint/segment
     * distances. (This is the planar simplification of the general clamped solve.)
     *
     * @param a The first segment.
     * @param b The second segment.
     * @return The squared Euclidean distance between the segments, or 0 if they intersect.
     */
    [[nodiscard]] inline float squared_distance(const segment& a, const segment& b) noexcept {
        if (intersects(a, b)) {
            return 0.0f;
        }
        return std::min({
            squared_distance(a.from, b),
            squared_distance(a.to, b),
            squared_distance(b.from, a),
            squared_distance(b.to, a),
        });
    }

    /**
     * @brief Squared distance between a finite segment and an AABB (0 if they overlap).
     *
     * When disjoint, the closest pair includes either a segment endpoint or a box corner,
     * so the distance is the minimum over the two endpoints (vs the box) and the four
     * corners (vs the segment).
     *
     * @param s The line segment.
     * @param b The axis-aligned bounding box.
     * @return The squared Euclidean distance between the segment and the box, or 0 if they overlap.
     */
    [[nodiscard]] constexpr float squared_distance(const segment& s, const aabb& b) noexcept {
        if (intersects(s, b)) {
            return 0.0f;
        }
        return std::min({
            squared_distance(s.from, b),
            squared_distance(s.to, b),
            squared_distance(vec{b.min.x(), b.min.y()}, s),
            squared_distance(vec{b.max.x(), b.min.y()}, s),
            squared_distance(vec{b.min.x(), b.max.y()}, s),
            squared_distance(vec{b.max.x(), b.max.y()}, s),
        });
    }

    /// @brief Argument-order-independent overload of @ref squared_distance(const segment&, const aabb&).
    [[nodiscard]] constexpr float squared_distance(const aabb& b, const segment& s) noexcept {
        return squared_distance(s, b);
    }

    /**
     * @brief Continuous collision detection (CCD) query for two moving circles over a time interval.
     *
     * This function checks if two moving circles collide during a given time step. It uses
     * a Minkowski sum formulation by inflating the second circle by the radius of the first
     * and sweeping the center point of the first circle along the relative velocity vector.
     *
     * @param acirc First circle.
     * @param avel Velocity of the first circle (world units / s).
     * @param bcirc Second circle.
     * @param bvel Velocity of the second circle (world units / s).
     * @param time Maximum time duration of the query (s).
     * @return A @ref swept_hit if a collision occurs within [0, time], or std::nullopt otherwise.
     */
    [[nodiscard]] inline std::optional <swept_hit> swept_intersection(
        const circle& acirc, const vec& avel,
        const circle& bcirc, const vec& bvel,
        float time) noexcept {
        const vec rel_vel = avel - bvel;
        const circle image{bcirc.center, bcirc.radius + acirc.radius};
        const segment s{acirc.center, acirc.center + rel_vel * time};
        const auto it = intersect_param(image, s);
        if (!it || !it->segment_overlaps()) {
            return std::nullopt;
        }
        return to_swept_hit(*it, *it, time);
    }

    /**
     * @brief Continuous collision detection (CCD) query for a moving circle and a moving AABB.
     *
     * This function checks if a moving circle and a moving AABB collide during a given time step.
     *
     * THEORY:
     * To solve the swept intersection between a circle and an AABB:
     * 1. Transform the query to relative coordinates: AABB is static, circle center moves along
     *    the relative displacement segment: S = acirc.center + t * (avel - bvel) * time.
     * 2. The Minkowski sum of an AABB and a circle of radius R yields a "rounded rectangle"
     *    (a stadium shape). This rounded rectangle is composed of:
     *    - A horizontally inflated box: aabb{min.x - R, min.y, max.x + R, max.y}
     *    - A vertically inflated box:   aabb{min.x, min.y - R, max.x, max.y + R}
     *    - Four circular caps of radius R centered at each corner of the original AABB.
     * 3. The moving point (circle center) intersects the rounded rectangle if and only if it
     *    intersects any of these 6 candidate shapes. We test all 6 and select the earliest
     *    entry time.
     *
     * ILLUSTRATION:
     *                  Rounded Rectangle Minkowski Sum
     *                     +-----------------+
     *                  .  |  Vertical Box   |  .
     *                .    |  (height + 2R)  |    .
     *               +-----+-----------------+-----+
     *               |     |                 |     |
     *   Horizontal  |     |  Original AABB  |     |
     *      Box      |     |                 |     |
     *  (width + 2R) +-----+-----------------+-----+
     *                .    |                 |    .
     *                  .  |                 |  .
     *                     +-----------------+
     *                   (Corner Circles of radius R
     *                    centered at AABB vertices)
     *
     * @param acirc Moving circle.
     * @param avel Velocity of the circle (world units / s).
     * @param bbox Moving axis-aligned bounding box.
     * @param bvel Velocity of the AABB (world units / s).
     * @param time Maximum time duration of the query (s).
     * @return A @ref swept_hit if a collision occurs within [0, time], or std::nullopt otherwise.
     */
    [[nodiscard]] inline std::optional <swept_hit> swept_intersection(
        const circle& acirc, const vec& avel,
        const aabb& bbox, const vec& bvel,
        float time) noexcept {
        const auto rel_speed = avel - bvel;
        const auto from = acirc.center;
        const vec to = from + time * rel_speed;
        const segment seg{from, to};
        const float R = acirc.radius;

        // The Minkowski sum (rounded rectangle) is convex, so the swept point's
        // intersection with it is a single interval [min entry, max exit] taken over the
        // sub-shapes the path actually crosses. Track the earliest-entering and
        // latest-exiting candidates separately rather than reading both off one shape.
        std::optional <line_hit> entry_hit = std::nullopt;
        std::optional <line_hit> exit_hit = std::nullopt;

        auto update_best = [&](const line_hit& hit) {
            if (!entry_hit || hit.entry_param < entry_hit->entry_param) {
                entry_hit = hit;
            }
            if (!exit_hit || hit.exit_param > exit_hit->exit_param) {
                exit_hit = hit;
            }
        };

        // 1. Check Horizontal Expanded Box
        const aabb box_h{{bbox.min.x() - R, bbox.min.y()}, {bbox.max.x() + R, bbox.max.y()}};
        if (const auto hit_h = intersect_param(box_h, seg); hit_h && hit_h->segment_overlaps()) {
            update_best(*hit_h);
        }

        // 2. Check Vertical Expanded Box
        const aabb box_v{{bbox.min.x(), bbox.min.y() - R}, {bbox.max.x(), bbox.max.y() + R}};
        if (const auto hit_v = intersect_param(box_v, seg); hit_v && hit_v->segment_overlaps()) {
            update_best(*hit_v);
        }

        // 3. Check the 4 Corner Circles
        const vec corners[4] = {
            {bbox.min.x(), bbox.min.y()}, // Bottom-Left
            {bbox.max.x(), bbox.min.y()}, // Bottom-Right
            {bbox.min.x(), bbox.max.y()}, // Top-Left
            {bbox.max.x(), bbox.max.y()} // Top-Right
        };

        for (const auto& corner : corners) {
            const circle corner_circ{corner, R};
            if (const auto hit_circ = intersect_param(corner_circ, seg); hit_circ && hit_circ->segment_overlaps()) {
                update_best(*hit_circ);
            }
        }

        if (!entry_hit) {
            return std::nullopt;
        }
        // Entry/normal come from the earliest-entering sub-shape; exit/normal from the
        // latest-exiting one (valid because the composite Minkowski shape is convex).
        return to_swept_hit(*entry_hit, *exit_hit, time);
    }

    /**
     * @brief Argument-order-independent overload of swept_intersection(circle, aabb).
     *
     * The physical query is symmetric (only the relative velocity matters), so this simply
     * forwards with the shapes/velocities reordered. Normals still point outward from the
     * AABB obstacle, exactly as in the canonical (circle, aabb) overload.
     */
    [[nodiscard]] inline std::optional <swept_hit> swept_intersection(
        const aabb& abox, const vec& avel,
        const circle& ccirc, const vec& cvel,
        float time) noexcept {
        return swept_intersection(ccirc, cvel, abox, avel, time);
    }

    /**
     * @brief CCD query for a moving circle vs a (possibly moving) segment over [0, time].
     *
     * The Minkowski sum of the circle and the segment is a capsule (the segment thickened
     * by the radius). Working in the segment's local frame (x along the segment, y across),
     * the capsule splits into an axis-aligned core box {{0,-R},{L,R}} plus the two endpoint
     * circles of radius R; their union is the capsule and it is convex, so the swept point's
     * crossing is the single interval [min entry, max exit] over those sub-shapes.
     *
     * @param acirc Moving circle.    @param avel Its velocity (world units / s).
     * @param seg   The segment.      @param svel Its velocity (world units / s).
     * @param time  Query duration (s).
     * @return A @ref swept_hit (normals point outward from the segment), or std::nullopt.
     */
    [[nodiscard]] inline std::optional <swept_hit> swept_intersection(
        const circle& acirc, const vec& avel,
        const segment& seg, const vec& svel,
        float time) noexcept {
        const vec rel = avel - svel;
        const vec a = acirc.center;
        const segment path{a, a + rel * time};
        const float R = acirc.radius;

        const vec seg_d = seg.to - seg.from;
        if (euler::approx_zero(seg_d, constants::POINT_EPS)) {
            // Degenerate segment: the capsule is a single circle of radius R at seg.from.
            const auto h = intersect_param(circle{seg.from, R}, path);
            if (!h || !h->segment_overlaps()) {
                return std::nullopt;
            }
            return to_swept_hit(*h, *h, time);
        }

        // Local frame: x along the segment, y across it.
        const float len = euler::length(seg_d);
        const vec ux = seg_d / len;
        const vec uy{-ux.y(), ux.x()};
        const auto to_local = [&](const vec& p) {
            const vec q = p - seg.from;
            return vec{euler::dot(q, ux), euler::dot(q, uy)};
        };
        const segment lpath{to_local(a), to_local(a + rel * time)};

        std::optional<line_hit> entry_hit, exit_hit;
        const auto update = [&](const line_hit& h) {
            if (!entry_hit || h.entry_param < entry_hit->entry_param) { entry_hit = h; }
            if (!exit_hit || h.exit_param > exit_hit->exit_param) { exit_hit = h; }
        };
        if (const auto h = intersect_param(aabb{{0.0f, -R}, {len, R}}, lpath); h && h->segment_overlaps()) { update(*h); }
        if (const auto h = intersect_param(circle{{0.0f, 0.0f}, R}, lpath); h && h->segment_overlaps()) { update(*h); }
        if (const auto h = intersect_param(circle{{len, 0.0f}, R}, lpath); h && h->segment_overlaps()) { update(*h); }
        if (!entry_hit) {
            return std::nullopt;
        }

        // Rotate the local-frame contact normals back to world.
        const auto to_world = [&](const vec& n) { return n.x() * ux + n.y() * uy; };
        entry_hit->entry_normal = to_world(entry_hit->entry_normal);
        exit_hit->exit_normal = to_world(exit_hit->exit_normal);
        return to_swept_hit(*entry_hit, *exit_hit, time);
    }

    /**
     * @brief CCD query for a moving AABB vs a (possibly moving) segment over [0, time].
     *
     * Transitions into/out of overlap between two translating convex shapes are vertex/edge
     * events, captured by two feature families: each segment endpoint swept against the box
     * (in the box's rest frame the endpoint moves by -rel), and each box corner swept against
     * the segment. Those give the entry/exit *times*. The interval is then anchored on actual
     * overlap at the window ends (a segment can already cross the box interior at t=0 with no
     * vertex event): if overlapping at t=0 the entry is 0, and if still overlapping at t=time
     * the overlap runs to the end of the window.
     *
     * @param abox Moving box.        @param avel Its velocity (world units / s).
     * @param seg  The segment.       @param svel Its velocity (world units / s).
     * @param time Query duration (s).
     * @return A @ref swept_hit (normals point outward from the segment), or std::nullopt.
     */
    [[nodiscard]] inline std::optional <swept_hit> swept_intersection(
        const aabb& abox, const vec& avel,
        const segment& seg, const vec& svel,
        float time) noexcept {
        const vec d = (avel - svel) * time; // relative displacement of the box

        std::optional<line_hit> entry_hit, exit_hit;
        const auto update = [&](const line_hit& h) {
            if (!entry_hit || h.entry_param < entry_hit->entry_param) { entry_hit = h; }
            if (!exit_hit || h.exit_param > exit_hit->exit_param) { exit_hit = h; }
        };

        // Each segment endpoint vs the box (box's rest frame: the endpoint moves by -d).
        // intersect_param yields the box-face outward normal (along the box's motion); negate
        // it so every contact reports the same convention: the way to push the box to separate.
        for (const vec& ep : {seg.from, seg.to}) {
            if (auto h = intersect_param(abox, segment{ep, ep - d}); h && h->segment_overlaps()) {
                h->entry_normal = -h->entry_normal;
                h->exit_normal = -h->exit_normal;
                update(*h);
            }
        }
        // Each box corner sweeping by +d, vs the segment.
        const vec corners[4] = {
            {abox.min.x(), abox.min.y()}, {abox.max.x(), abox.min.y()},
            {abox.min.x(), abox.max.y()}, {abox.max.x(), abox.max.y()}
        };
        for (const vec& c : corners) {
            if (const auto h = intersect_param(segment{c, c + d}, seg); h && h->segment_overlaps()) {
                update(*h);
            }
        }

        // Anchor the interval on real overlap at the window ends (covers the segment crossing
        // the box interior at t=0 with no vertex event, and overlap persisting through t=time).
        const bool start_overlap = intersects(seg, abox);
        const aabb end_box{abox.min + d, abox.max + d};
        const bool end_overlap = intersects(seg, end_box);

        if (!start_overlap && !end_overlap && !entry_hit) {
            return std::nullopt; // never overlap within the window
        }

        // Unit segment normal oriented toward `toward`, the push-out direction used for an
        // already-overlapping end of the interval (where there is no vertex/edge entry event).
        const auto seg_push_normal = [&](const vec& toward) -> vec {
            vec n{seg.from.y() - seg.to.y(), seg.to.x() - seg.from.x()};
            const float l = euler::length(n);
            if (l <= constants::NORMALIZE_EPS) {
                return vec{0.0f, 0.0f};
            }
            n = n / l;
            if (euler::dot(n, toward - seg.from) < 0.0f) {
                n = -n;
            }
            return n;
        };

        swept_hit out;
        if (start_overlap || !entry_hit) {
            out.entry_time = 0.0f;
            out.entry_normal = seg_push_normal(abox.center());
        } else {
            out.entry_time = std::max(0.0f, entry_hit->entry_param) * time;
            out.entry_normal = entry_hit->entry_normal;
        }
        if (end_overlap || !exit_hit) {
            out.exit_time = time;
            out.exit_normal = seg_push_normal(end_box.center());
        } else {
            out.exit_time = std::min(exit_hit->exit_param, 1.0f) * time;
            out.exit_normal = exit_hit->exit_normal;
        }
        return out;
    }
} // namespace neutrino::physics
