//
// Created by igor on 25/06/2026.
//

/**
 * @file triangle.hh
 * @brief Solid-triangle narrow-phase: static overlap, ray/segment parameterisation, and swept
 *        (continuous) collision for a filled triangle -- used for free-standing solid slopes/ramps.
 *
 * A triangle is a 2-D region whose interior is SOLID, so it blocks from every side -- unlike a
 * single @ref neutrino::physics::segment, which only blocks something crossing its line.
 *
 * Everything here is built by DECOMPOSITION: a triangle is its three edge segments plus a solid
 * interior. Each query reuses the already-tested segment machinery (@ref
 * neutrino::physics::intersects / @ref neutrino::physics::intersect_param / @ref
 * neutrino::physics::swept_intersection against a segment) over the three edges, and adds the
 * interior ("is this point inside?") cases the edges alone do not cover. No new SAT/CCD math is
 * introduced.
 *
 * @note The narrow-phase entry points mirror the other shapes': @ref
 *       neutrino::physics::intersects (static overlap), @ref neutrino::physics::intersect_param
 *       (raycast), and @ref neutrino::physics::swept_intersection (CCD), plus @ref
 *       neutrino::physics::contains, @ref neutrino::physics::enclose and @ref
 *       neutrino::physics::translate.
 */
#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <cmath>

#include <euler/vector/vector_ops.hh>
#include <neutrino/physics/geometry/types.hh>
#include <neutrino/physics/geometry/overlap.hh> // intersects(segment,*), contains(aabb/circle,vec)
#include <neutrino/physics/geometry/sweep.hh>   // intersect_param / swept_intersection / intersects(segment,*)
#include <neutrino/physics/geometry/enclose.hh>
#include <neutrino/physics/geometry/translate.hh>

namespace neutrino::physics {
    namespace detail {
        /**
         * @brief The triangle's three directed edges (a->b, b->c, c->a) as segments.
         * @param t The triangle.
         * @return The three edge segments, in winding order.
         */
        [[nodiscard]] inline std::array<segment, 3> tri_edges(const triangle& t) {
            return {segment{t.a, t.b}, segment{t.b, t.c}, segment{t.c, t.a}};
        }

        /**
         * @brief The centroid (average of the three vertices) of triangle @p t.
         * @param t The triangle.
         * @return A point guaranteed to lie strictly inside a non-degenerate triangle -- used as the
         *         "interior" reference when orienting edge normals.
         */
        [[nodiscard]] inline vec tri_centroid(const triangle& t) {
            return vec{(t.a.x() + t.b.x() + t.c.x()) / 3.0f, (t.a.y() + t.b.y() + t.c.y()) / 3.0f};
        }

        /**
         * @brief Unit OUTWARD normal of edge @p p -> @p q for a triangle whose interior lies toward
         *        @p inside.
         *
         * The raw perpendicular of the edge is flipped, if necessary, to point AWAY from the
         * interior reference point (typically the centroid; see @ref tri_centroid).
         *
         * @param p      Edge start vertex.
         * @param q      Edge end vertex.
         * @param inside A point on the interior side of the edge (e.g. the triangle centroid).
         * @return The unit outward normal, or @c {0,0} if the edge is shorter than
         *         @c constants::NORMALIZE_EPS (degenerate).
         */
        [[nodiscard]] inline vec edge_outward_normal(const vec& p, const vec& q, const vec& inside) {
            vec n{-(q.y() - p.y()), q.x() - p.x()};
            const vec mid{(p.x() + q.x()) * 0.5f, (p.y() + q.y()) * 0.5f};
            if (euler::dot(n, inside - mid) > 0.0f) {
                n = -n; // points toward the interior -> flip
            }
            const float len = euler::length(n);
            return len > constants::NORMALIZE_EPS ? n / len : vec{0.0f, 0.0f};
        }
    }

    // ---- enclose / translate (defined early: swept_tri uses translate for its end-overlap test) --

    /**
     * @brief The axis-aligned bounding box that tightly encloses triangle @p t.
     * @param t The triangle.
     * @return The AABB spanning the min/max of the three vertices.
     * @note Defined early because @ref detail::swept_tri uses @ref translate for its end-overlap test.
     */
    [[nodiscard]] inline aabb enclose(const triangle& t) noexcept {
        return aabb{
            vec{std::min({t.a.x(), t.b.x(), t.c.x()}), std::min({t.a.y(), t.b.y(), t.c.y()})},
            vec{std::max({t.a.x(), t.b.x(), t.c.x()}), std::max({t.a.y(), t.b.y(), t.c.y()})}
        };
    }

    /**
     * @brief Translate triangle @p t by displacement @p v (each vertex moved by @p v).
     * @param t The triangle.
     * @param v The displacement.
     * @return The translated triangle.
     */
    [[nodiscard]] constexpr triangle translate(const triangle& t, const vec& v) {
        return {translate(t.a, v), translate(t.b, v), translate(t.c, v)};
    }

    /**
     * @brief Test if a (solid) triangle contains a point, inclusive of the boundary.
     *
     * Combines a bounding-box guard with a same-sign cross-product (half-plane) test: a point is
     * inside iff it lies on the same side of all three directed edges (zero counts as "on an edge").
     *
     * @param t The (solid) triangle.
     * @param p The query point.
     * @return @c true iff @p p is inside @p t or on its boundary.
     * @note The bounding-box guard also gives a DEGENERATE (collinear) triangle the documented
     *       "behaves like its longest edge" semantics: the same-sign cross test alone is true for the
     *       whole infinite line when the area is zero, but the bbox of three collinear points is
     *       exactly the longest edge's extent, so the two together accept only points on that edge.
     */
    [[nodiscard]] inline bool contains(const triangle& t, const vec& p) noexcept {
        const float min_x = std::min({t.a.x(), t.b.x(), t.c.x()});
        const float max_x = std::max({t.a.x(), t.b.x(), t.c.x()});
        const float min_y = std::min({t.a.y(), t.b.y(), t.c.y()});
        const float max_y = std::max({t.a.y(), t.b.y(), t.c.y()});
        if (p.x() < min_x || p.x() > max_x || p.y() < min_y || p.y() > max_y) {
            return false;
        }
        const auto cross = [](const vec& u, const vec& v, const vec& w) {
            return (v.x() - u.x()) * (w.y() - u.y()) - (v.y() - u.y()) * (w.x() - u.x());
        };
        const float d1 = cross(t.a, t.b, p);
        const float d2 = cross(t.b, t.c, p);
        const float d3 = cross(t.c, t.a, p);
        const bool has_neg = d1 < 0.0f || d2 < 0.0f || d3 < 0.0f;
        const bool has_pos = d1 > 0.0f || d2 > 0.0f || d3 > 0.0f;
        return !(has_neg && has_pos); // all the same sign (or zero) -> inside / on an edge
    }

    // ---- static overlap (intersects) ---------------------------------------------------------

    /**
     * @brief Static overlap test between a solid triangle and a segment.
     * @param t The solid triangle.
     * @param s The segment.
     * @return @c true iff any triangle edge crosses @p s, or @p s lies entirely inside @p t.
     */
    [[nodiscard]] inline bool intersects(const triangle& t, const segment& s) noexcept {
        for (const auto& e : detail::tri_edges(t)) {
            if (intersects(e, s)) {
                return true;
            }
        }
        return contains(t, s.from); // s lies entirely inside the triangle
    }

    /**
     * @brief Static overlap test between a solid triangle and a circle.
     * @param t The solid triangle.
     * @param c The circle.
     * @return @c true iff any triangle edge intersects @p c, or @p c lies entirely inside @p t.
     */
    [[nodiscard]] inline bool intersects(const triangle& t, const circle& c) noexcept {
        for (const auto& e : detail::tri_edges(t)) {
            if (intersects(e, c)) {
                return true;
            }
        }
        return contains(t, c.center); // circle entirely inside the triangle
    }

    /**
     * @brief Static overlap test between a solid triangle and an AABB.
     * @param t The solid triangle.
     * @param b The axis-aligned bounding box.
     * @return @c true iff any triangle edge crosses or lies inside @p b (which also covers the
     *         triangle-inside-box case), or @p b is entirely inside @p t (a box corner is inside).
     */
    [[nodiscard]] inline bool intersects(const triangle& t, const aabb& b) noexcept {
        for (const auto& e : detail::tri_edges(t)) {
            if (intersects(e, b)) {
                return true; // an edge crosses or lies inside the box (covers triangle-inside-box too)
            }
        }
        // otherwise overlap only if the box is entirely inside the triangle -> a box corner is inside
        return contains(t, b.min) || contains(t, vec{b.max.x(), b.min.y()})
               || contains(t, vec{b.min.x(), b.max.y()}) || contains(t, b.max);
    }

    /**
     * @brief Static overlap test between two solid triangles.
     * @param t The first solid triangle.
     * @param u The second solid triangle.
     * @return @c true iff any edge of @p t crosses any edge of @p u, or one triangle is entirely
     *         inside the other.
     */
    [[nodiscard]] inline bool intersects(const triangle& t, const triangle& u) noexcept {
        for (const auto& e : detail::tri_edges(t)) {
            for (const auto& f : detail::tri_edges(u)) {
                if (intersects(e, f)) {
                    return true;
                }
            }
        }
        return contains(t, u.a) || contains(u, t.a); // one entirely inside the other
    }

    // Argument-order-independent overloads (a triangle may be either operand).

    /// @brief Argument-order-flipped overload of @ref intersects(const triangle&, const segment&).
    [[nodiscard]] inline bool intersects(const segment& s, const triangle& t) noexcept { return intersects(t, s); }
    /// @brief Argument-order-flipped overload of @ref intersects(const triangle&, const circle&).
    [[nodiscard]] inline bool intersects(const circle& c, const triangle& t) noexcept { return intersects(t, c); }
    /// @brief Argument-order-flipped overload of @ref intersects(const triangle&, const aabb&).
    [[nodiscard]] inline bool intersects(const aabb& b, const triangle& t) noexcept { return intersects(t, b); }

    // ---- ray / segment parameter (for raycast) -----------------------------------------------

    /**
     * @brief Where the line through segment `ray` enters/exits the solid triangle.
     *
     * Mirrors @ref intersect_param(const aabb&, const segment&): the returned params are along
     * the ray's infinite line (NOT clamped to [0,1]); a ray that starts inside the triangle has
     * `entry_param < 0 <= exit_param`. `entry_normal` is the triangle's OUTWARD normal at the
     * entry edge. Returns nullopt if the line misses the triangle.
     */
    [[nodiscard]] inline std::optional<line_hit> intersect_param(const triangle& t, const segment& ray) noexcept {
        const vec dir = ray.to - ray.from;
        const float dir_sq = euler::dot(dir, dir);
        if (dir_sq < constants::POINT_EPS * constants::POINT_EPS) {
            return std::nullopt; // degenerate (zero-length) ray
        }
        const vec ctr = detail::tri_centroid(t);

        float entry = constants::INF;
        float exit = constants::NEG_INF;
        vec entry_n{}, exit_n{};
        bool any = false;

        for (const auto& e : detail::tri_edges(t)) {
            const vec ed = e.to - e.from;
            const float H = euler::cross(dir, ed);
            const float ed_sq = euler::dot(ed, ed);
            // Parallel (or collinear) edge: its crossing is a grazing/degenerate touch -- the other
            // two edges carry the entry/exit. Skip, scaled like the segment-segment test.
            if (H * H <= constants::PARALLEL_REL_EPS * constants::PARALLEL_REL_EPS * dir_sq * ed_sq) {
                continue;
            }
            const vec q = e.from - ray.from;
            const float tt = euler::cross(q, ed) / H;  // param along the ray's infinite line
            const float uu = euler::cross(q, dir) / H; // param along the edge
            if (uu < 0.0f || uu > 1.0f) {
                continue; // the crossing is outside this edge
            }
            any = true;
            const vec on = detail::edge_outward_normal(e.from, e.to, ctr);
            if (tt < entry) { entry = tt; entry_n = on; }
            if (tt > exit) { exit = tt; exit_n = on; }
        }
        if (!any) {
            return std::nullopt;
        }
        return line_hit{entry, exit, entry_n, exit_n};
    }

    // ---- swept (continuous) collision: a moving aabb / circle vs a static-ish triangle --------

    namespace detail {
        /**
         * @brief Earliest contact of a swept mover with a (static-ish) triangle.
         *
         * Computes the time-of-impact interval against the triangle's boundary (its three edges) via
         * the per-edge segment sweeps. Because a convex mover vs a convex triangle overlaps over ONE
         * contiguous interval, the first/last boundary touches bound it. If the mover never touches an
         * edge but already overlaps the triangle's solid interior at @c t=0 (or still does at
         * @c t=time), the corresponding interval end is anchored to a toi-0 / toi-time contact with
         * the nearest edge's outward normal (the push-out direction) -- the edge sweeps only see
         * boundary CROSSINGS, so a mover that starts or ends deep inside the solid would otherwise be
         * mis-reported.
         *
         * @tparam Mover       The moving shape type (@ref aabb or @ref circle).
         * @param m            The mover shape at @c t=0.
         * @param mv           The mover's per-unit-time velocity.
         * @param t            The (static-ish) triangle.
         * @param tv           The triangle's per-unit-time velocity (~ @c {0,0} for static tiles).
         * @param time         The length of the sweep window.
         * @param mover_centre The mover's centre at @c t=0 (used to pick the push-out edge normal).
         * @return The @ref swept_hit interval, or @c std::nullopt if the mover never meets the triangle.
         */
        template<class Mover>
        [[nodiscard]] std::optional<swept_hit> swept_tri(const Mover& m, const vec& mv,
                                                         const triangle& t, const vec& tv, float time,
                                                         const vec& mover_centre) {
            // Boundary-touch interval from the per-edge sweeps: a convex mover vs a convex triangle
            // overlaps over ONE contiguous interval, so the first/last boundary touches bound it.
            float entry = constants::INF, exit = constants::NEG_INF;
            vec entry_n{}, exit_n{};
            bool any = false;
            for (const auto& e : tri_edges(t)) {
                if (const auto h = swept_intersection(m, mv, e, tv, time)) {
                    any = true;
                    if (h->entry_time < entry) { entry = h->entry_time; entry_n = h->entry_normal; }
                    if (h->exit_time > exit) { exit = h->exit_time; exit_n = h->exit_normal; }
                }
            }

            // Outward normal of the edge nearest to `p` (the push-out direction at a penetration).
            const auto nearest_outward = [&](const vec& p) {
                const vec ctr = tri_centroid(t);
                float best_d = constants::INF;
                vec n{};
                for (const auto& e : tri_edges(t)) {
                    const float d = squared_distance(p, e);
                    if (d < best_d) {
                        best_d = d;
                        n = edge_outward_normal(e.from, e.to, ctr);
                    }
                }
                return n;
            };

            // Anchor the interval ends against the actual solid overlap at t=0 and t=time -- the
            // edge sweeps only see boundary CROSSINGS, so a mover that starts (or ends) deep inside
            // the solid would otherwise mis-report that end of the interval. (tv ~ {0,0} for static
            // tiles, so the end normal uses the original triangle.)
            const vec mdisp{mv.x() * time, mv.y() * time};
            const vec tdisp{tv.x() * time, tv.y() * time};
            const bool start_overlap = intersects(t, m);
            const bool end_overlap = intersects(translate(t, tdisp), translate(m, mdisp));

            if (!start_overlap && !end_overlap && !any) {
                return std::nullopt; // never meets the triangle
            }

            swept_hit out;
            if (start_overlap) { // already penetrating at t=0 -> entry 0, push-out normal
                out.entry_time = 0.0f;
                out.entry_normal = nearest_outward(mover_centre);
            } else {
                out.entry_time = entry;
                out.entry_normal = entry_n;
            }
            if (end_overlap) { // still penetrating at t=time -> no separation within the window
                out.exit_time = time;
                // nearest_outward uses the ORIGINAL triangle, so feed the mover's end position
                // RELATIVE to the triangle's end (subtract the triangle's displacement). For a
                // static triangle (tdisp == 0) this is just the mover end; it matters when tv != 0.
                out.exit_normal = nearest_outward(mover_centre + mdisp - tdisp);
            } else {
                out.exit_time = exit;
                out.exit_normal = exit_n;
            }
            return out;
        }
    } // namespace detail

    /**
     * @brief Swept (continuous) collision of a moving AABB against a (static-ish) triangle.
     * @param m    The moving AABB at @c t=0.
     * @param mv   The AABB's per-unit-time velocity.
     * @param t    The triangle.
     * @param tv   The triangle's per-unit-time velocity (~ @c {0,0} for static tiles).
     * @param time The length of the sweep window.
     * @return The @ref swept_hit interval, or @c std::nullopt if no contact occurs. See @ref
     *         detail::swept_tri.
     */
    [[nodiscard]] inline std::optional<swept_hit> swept_intersection(const aabb& m, const vec& mv,
                                                                     const triangle& t, const vec& tv,
                                                                     float time) {
        return detail::swept_tri(m, mv, t, tv, time, m.center());
    }

    /**
     * @brief Swept (continuous) collision of a moving circle against a (static-ish) triangle.
     * @param m    The moving circle at @c t=0.
     * @param mv   The circle's per-unit-time velocity.
     * @param t    The triangle.
     * @param tv   The triangle's per-unit-time velocity (~ @c {0,0} for static tiles).
     * @param time The length of the sweep window.
     * @return The @ref swept_hit interval, or @c std::nullopt if no contact occurs. See @ref
     *         detail::swept_tri.
     */
    [[nodiscard]] inline std::optional<swept_hit> swept_intersection(const circle& m, const vec& mv,
                                                                     const triangle& t, const vec& tv,
                                                                     float time) {
        return detail::swept_tri(m, mv, t, tv, time, m.center);
    }
} // namespace neutrino::physics
