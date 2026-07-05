//
// Created by igor on 25/06/2026.
//

/**
 * @file clip.hh
 * @brief Clip a directed line segment to the interior of an axis-aligned box
 *        (@ref neutrino::physics::clip).
 *
 * Computes the portion of a @ref neutrino::physics::segment that lies inside a
 * @ref neutrino::physics::aabb, returning the trimmed sub-segment (or nothing when the segment
 * misses the box entirely). This is the classic Liang-Barsky line-clipping operation, layered on
 * top of the parametric slab test @ref neutrino::physics::intersect_param.
 *
 * @see sweep.hh for the underlying @ref neutrino::physics::intersect_param primitive.
 */

#pragma once

#include <optional>
#include <algorithm>
#include <neutrino/physics/geometry/sweep.hh>

namespace neutrino::physics {
    /**
     * @brief Clip a directed segment to the interior of an AABB (Liang-Barsky).
     *
     * Intersects the segment's line with @p box via @ref intersect_param, then clamps the
     * resulting entry/exit line parameters to the finite segment range @c [0,1] and
     * reconstructs the in-box sub-segment via @ref segment::point_in_time. The returned
     * sub-segment preserves the original orientation (its @c from precedes its @c to along
     * @p s).
     *
     * @param box The axis-aligned clipping box. Assumed to satisfy the @ref aabb min<=max
     *            invariant.
     * @param s   The segment to clip.
     * @return The portion of @p s inside @p box, or @c std::nullopt when @p s does not
     *         overlap @p box (no intersection, or the overlap lies entirely outside the
     *         finite @c [0,1] range).
     * @note A segment that merely grazes a face/corner (tangent) may still be reported as a
     *       hit, yielding a degenerate zero-length sub-segment.
     */
    inline std::optional <segment> clip(const aabb& box, const segment& s) {
        // Liang-Barsky
        const auto hit = intersect_param(box, s);
        if (!hit || !hit->segment_overlaps()) {
            return std::nullopt;
        }

        const float t0 = std::clamp(hit->entry_param, 0.0f, 1.0f);
        const float t1 = std::clamp(hit->exit_param, 0.0f, 1.0f);

        return segment{
            s.point_in_time(t0),
            s.point_in_time(t1),
        };
    }
} // namespace neutrino::physics
