//
// Created by igor on 22/06/2026.
//

#pragma once

/**
 * @file shapes.hh
 * @brief Umbrella header for the collision-shapes module -- pure world-space geometry that
 *        re-exports the whole collide narrow-phase API.
 *
 * This module knows nothing about display, dp, scale, or SDL. Everything here operates on a plain
 * numeric field (float world units) so the collision math can use products of lengths freely (dot,
 * cross, squared distance, SAT projections) and can be unit-tested with no window or global scale
 * state.
 *
 * Conversions to/from display-space (@c simplex::point / @c rect / @c circle, which are dp-based)
 * are performed by a dedicated display<->world bridge layer outside this module, which is the
 * @e only place that crosses between the two coordinate systems. Nothing in this header does.
 *
 * Include this header for the full API, or include a part directly:
 *   - @c <neutrino/physics/geometry/types.hh>     value types + result types
 *   - @c <neutrino/physics/geometry/distance.hh>  closest-point / squared-distance queries
 *   - @c <neutrino/physics/geometry/overlap.hh>   static boolean overlap predicates
 *   - @c <neutrino/physics/geometry/sweep.hh>     parametric + continuous collision queries
 *   - @c <neutrino/physics/geometry/triangle.hh>  solid-triangle narrow-phase (slopes/ramps)
 *
 * Query categories (kept distinct in both naming and result type):
 *   - @c intersects(A,B) -> @c bool : static overlap of two shapes.
 *   - @c intersect_param(shape,seg) -> @c line_hit : raw geometry; entry/exit are PARAMETERS along
 *     the segment line.
 *   - @c swept_intersection(A,av,B,bv,time) -> @c swept_hit : continuous collision; entry/exit are
 *     absolute TIMES in seconds.
 *   - @c overlap(A,B) -> @c penetration : static MTV to separate A from B.
 *
 * Supported shape pairs (argument order is interchangeable where both forms exist):
 *   - @b contains: @c (aabb|circle|segment|triangle, point) -- full point row of the matrix
 *   - @b intersects (static): every pair of @c {point,segment,circle,aabb,triangle} (point via
 *     contains)
 *   - @b overlap (MTV): @c (aabb,aabb) @c (circle,circle) @c (circle,aabb)|(aabb,circle) ->
 *     penetration
 *   - @b intersect_param: @c (aabb,segment) @c (circle,segment) @c (segment,segment)
 *     @c (triangle,segment) -> line_hit
 *   - @b swept_intersection: @c (aabb,aabb) @c (circle,circle) @c (circle,aabb)|(aabb,circle)
 *     @c (circle,segment) @c (aabb,segment) @c (aabb,triangle) @c (circle,triangle) -> swept_hit
 *   - @b closest_point: @c (point, segment|aabb|circle)
 *   - @b squared_distance: every pair of @c {point,segment,circle,aabb} (filled regions; 0 if
 *     overlapping)
 *
 * The solid @c triangle (slopes/ramps) lives in @c <neutrino/physics/geometry/triangle.hh>; its narrow-phase
 * is built by decomposing into the 3 edge segments + interior tests, reusing the segment machinery.
 *
 * @note @b constexpr: a query is @c constexpr exactly when it avoids @c std::sqrt, which is not
 *       constexpr in C++20 (euler's vector/matrix expression evaluation is otherwise constexpr).
 *       That covers the value-type accessors, @c contains, every static overlap predicate,
 *       @c intersect_param(aabb,segment), @c closest_point / @c closest_parameter /
 *       @c squared_distance for point-vs-segment and point-vs-aabb,
 *       @c swept_intersection(aabb,aabb), and @c to_swept_hit. Runtime-only are the paths that take
 *       a square root: anything against a circle radius (@c squared_distance / @c closest_point vs
 *       circle, @c intersect_param(circle,segment), the circle CCD overloads) and segment/segment
 *       (its normals are normalized).
 */

#include <neutrino/physics/geometry/types.hh>
#include <neutrino/physics/geometry/enclose.hh>
#include <neutrino/physics/geometry/translate.hh>
#include <neutrino/physics/geometry/distance.hh>
#include <neutrino/physics/geometry/overlap.hh>
#include <neutrino/physics/geometry/sweep.hh>
#include <neutrino/physics/geometry/triangle.hh>
#include <neutrino/physics/geometry/clip.hh>
