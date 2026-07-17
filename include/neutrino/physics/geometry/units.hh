//
// Created by igor on 23/06/2026.
//

/**
 * @file units.hh
 * @brief Minimal strongly-typed wrappers for the physical quantities the collision world's
 *        inner math juggles, so the compiler rejects unit confusions.
 *
 * These strong types catch the class of bug where a time-of-impact @ref
 * neutrino::physics::units::fraction is multiplied in where a @ref
 * neutrino::physics::units::duration belongs, or a @ref neutrino::physics::units::velocity is used
 * as a @ref neutrino::physics::units::displacement.
 *
 * The set is deliberately MINIMAL -- only the four quantities that actually interact in the
 * bug-prone arithmetic, with only the legal combinations defined:
 *   - @c velocity     @c * @c duration  @c = @c displacement     (v * dt)
 *   - @c displacement @c * @c fraction  @c = @c displacement     (delta * toi)
 *   - @c displacement @c +/- @c displacement @c = @c displacement
 *
 * Positions, directions/normals, and scalar distances/speeds are intentionally left as plain
 * @c vec / @c float -- typing them too would balloon into an ~8-type algebra for little extra
 * safety (those were never the bug sites). The public world API also stays @c vec / @c float;
 * these types are used inside the engine, converting at the boundary.
 *
 * @note The types live in a nested @c units namespace because @c velocity would otherwise clash
 *       with the @c vec @c velocity; member on the body records.
 * @note All results are materialised component-wise into @c vec (not via euler's expression
 *       templates), so a strong type never holds a lazy expression that could dangle.
 */

#pragma once

#include <neutrino/physics/geometry/types.hh>

namespace neutrino::physics::units {
    /**
     * @brief A time span: a frame's dt, or a swept-query window.
     *
     * Unit-agnostic on purpose -- the arithmetic in this header never assumes seconds; dt is
     * whatever unit the caller integrates in. (The world's public frame API happens to document
     * its dt as seconds, but nothing here depends on that choice.)
     */
    struct duration {
        float value{0.0f}; ///< The span magnitude, in the caller's time unit.
    };

    /// @brief A dimensionless parameter, typically in [0, 1]: a time-of-impact or interpolation t.
    struct fraction {
        float value{0.0f}; ///< The parameter value (typically in [0, 1]).
    };

    /// @brief A rate: world units per unit @ref duration.
    struct velocity {
        vec value{}; ///< The per-duration rate, component-wise in world units.
    };

    /// @brief A world-space offset (delta / remaining / leftover). A position stays a plain @c vec.
    struct displacement {
        vec value{}; ///< The offset, component-wise in world units.
    };

    // ---- dimensional algebra: only the legal combinations exist --------------------

    /**
     * @brief Integrate a rate over a time span: @c velocity @c * @c duration @c = @c displacement.
     * @param v The rate.
     * @param d The time span.
     * @return The world-space offset travelled (@p v scaled component-wise by @p d).
     */
    [[nodiscard]] inline displacement operator*(velocity v, duration d) {
        return displacement{vec{v.value.x() * d.value, v.value.y() * d.value}};
    }
    /**
     * @brief Commuted form of @ref operator*(velocity, duration).
     * @param d The time span.
     * @param v The rate.
     * @return The world-space offset travelled.
     */
    [[nodiscard]] inline displacement operator*(duration d, velocity v) { return v * d; }

    /**
     * @brief Take a sub-span of a move (e.g. up to the TOI):
     *        @c displacement @c * @c fraction @c = @c displacement.
     * @param s The full offset.
     * @param f The fraction of @p s to keep (typically in [0, 1]).
     * @return @p s scaled component-wise by @p f.
     */
    [[nodiscard]] inline displacement operator*(displacement s, fraction f) {
        return displacement{vec{s.value.x() * f.value, s.value.y() * f.value}};
    }
    /**
     * @brief Commuted form of @ref operator*(displacement, fraction).
     * @param f The fraction (typically in [0, 1]).
     * @param s The full offset.
     * @return @p s scaled component-wise by @p f.
     */
    [[nodiscard]] inline displacement operator*(fraction f, displacement s) { return s * f; }

    /**
     * @brief Add two offsets: @c displacement @c + @c displacement @c = @c displacement.
     * @param a,b The offsets to sum.
     * @return The component-wise sum.
     */
    [[nodiscard]] inline displacement operator+(displacement a, displacement b) {
        return displacement{vec{a.value.x() + b.value.x(), a.value.y() + b.value.y()}};
    }
    /**
     * @brief Subtract two offsets: @c displacement @c - @c displacement @c = @c displacement.
     * @param a The minuend offset.
     * @param b The subtrahend offset.
     * @return The component-wise difference @p a - @p b.
     */
    [[nodiscard]] inline displacement operator-(displacement a, displacement b) {
        return displacement{vec{a.value.x() - b.value.x(), a.value.y() - b.value.y()}};
    }
} // namespace neutrino::physics::units
