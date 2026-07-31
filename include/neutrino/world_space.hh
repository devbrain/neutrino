//
// Created by igor on 31/07/2026.
//

#pragma once

/**
 * @file world_space.hh
 * @brief Strong coordinate/quantity types for gameplay space -- roadmap Tier 3.
 *
 * The engine's public surface has long spoken one shape for many meanings: a position, an offset
 * and a velocity are all a pair of floats (@c vec / @c world_point), so any of them fits wherever
 * another is expected. These types separate them, defining only the operations that mean something:
 *
 *   world_pos      - world_pos     = world_delta      // two places -> the gap between them
 *   world_pos      + world_delta   = world_pos        // a place, displaced
 *   world_delta    +/- world_delta = world_delta
 *   world_velocity * seconds       = world_delta      // a rate, integrated
 *   world_delta    / seconds       = world_velocity   // a move, differentiated
 *   world_pos      + world_pos     = does not compile // adding two places is meaningless
 *
 * The bug class this exists to make unwritable is the one that actually cost us: KE held a
 * *position* target but the physics API only spoke *velocity*, so it fabricated
 * `(target - current)/dt` -- which exploded into a wall-pinning freeze when the target was
 * unreachable. With these types that mismatch is a compile error at the call, not a runtime
 * mystery. The second class is space confusion at the input edge: @ref render_pos and
 * @ref window_pos are separate types, so handing raw window pixels to something expecting
 * presentation-space coordinates cannot silently succeed under HiDPI or letterboxing.
 *
 * @note These are DISTINCT types, deliberately not aliases. `using world_pos = vec;` would compile
 *       everywhere and catch nothing -- the whole value is in the conversions being rejected.
 * @note Neutrino already applies this idea internally (@c physics::units::velocity / @c duration /
 *       @c displacement) but strips it at the public boundary. This is that discipline, made
 *       public. Conversion to and from the untyped representations is explicit and one-directional
 *       per call (@ref to_xy / the explicit constructors), so every boundary crossing is visible.
 *
 * @warning Introduced ahead of the call-site migration: nothing in the engine takes these yet.
 *          The migration converts the physics public boundary first and lets the compiler
 *          enumerate the rest. See games/ke/roadmap.md, Tier 3.
 */

#include <chrono>
#include <cmath>

namespace neutrino {
    /// @brief Seconds. The time base these quantities integrate over (same type as @c sim_duration).
    using world_seconds = std::chrono::duration <float>;

    /// @brief An OFFSET in world space: the gap between two positions, or a distance moved.
    /// Adding offsets, scaling them, and negating them are all meaningful.
    struct world_delta {
        float x{};
        float y{};

        [[nodiscard]] constexpr world_delta operator-() const noexcept { return {-x, -y}; }

        [[nodiscard]] float length() const noexcept { return std::sqrt(x * x + y * y); }
        [[nodiscard]] constexpr float length_sq() const noexcept { return x * x + y * y; }

        /// @brief The same direction with unit length; a zero delta stays zero (no NaN).
        [[nodiscard]] world_delta normalized() const noexcept {
            const float len = length();
            return len > 0.0f ? world_delta{x / len, y / len} : world_delta{};
        }

        [[nodiscard]] friend constexpr bool operator==(world_delta, world_delta) = default;
    };

    /// @brief A POSITION in world space. Deliberately NOT addable to another position: the sum of
    /// two places has no meaning, whereas the difference (a @ref world_delta) does.
    struct world_pos {
        float x{};
        float y{};

        [[nodiscard]] friend constexpr bool operator==(world_pos, world_pos) = default;
    };

    /// @brief A RATE in world space: world units per second. Integrate with @c * seconds.
    struct world_velocity {
        float x{};
        float y{};

        [[nodiscard]] constexpr world_velocity operator-() const noexcept { return {-x, -y}; }

        [[nodiscard]] float speed() const noexcept { return std::sqrt(x * x + y * y); }
        [[nodiscard]] constexpr float speed_sq() const noexcept { return x * x + y * y; }

        [[nodiscard]] friend constexpr bool operator==(world_velocity, world_velocity) = default;
    };

    // ---- the algebra: only the combinations that mean something --------------------------------

    /// @brief Two places -> the offset between them (@p to relative to @p from).
    [[nodiscard]] constexpr world_delta operator-(world_pos to, world_pos from) noexcept {
        return {to.x - from.x, to.y - from.y};
    }

    /// @brief A place, displaced.
    [[nodiscard]] constexpr world_pos operator+(world_pos p, world_delta d) noexcept {
        return {p.x + d.x, p.y + d.y};
    }
    /// @copydoc operator+(world_pos, world_delta)
    [[nodiscard]] constexpr world_pos operator+(world_delta d, world_pos p) noexcept { return p + d; }
    /// @brief A place, displaced backwards.
    [[nodiscard]] constexpr world_pos operator-(world_pos p, world_delta d) noexcept {
        return {p.x - d.x, p.y - d.y};
    }

    constexpr world_pos& operator+=(world_pos& p, world_delta d) noexcept { return p = p + d; }
    constexpr world_pos& operator-=(world_pos& p, world_delta d) noexcept { return p = p - d; }

    [[nodiscard]] constexpr world_delta operator+(world_delta a, world_delta b) noexcept {
        return {a.x + b.x, a.y + b.y};
    }
    [[nodiscard]] constexpr world_delta operator-(world_delta a, world_delta b) noexcept {
        return {a.x - b.x, a.y - b.y};
    }
    [[nodiscard]] constexpr world_delta operator*(world_delta d, float s) noexcept {
        return {d.x * s, d.y * s};
    }
    [[nodiscard]] constexpr world_delta operator*(float s, world_delta d) noexcept { return d * s; }

    constexpr world_delta& operator+=(world_delta& a, world_delta b) noexcept { return a = a + b; }
    constexpr world_delta& operator-=(world_delta& a, world_delta b) noexcept { return a = a - b; }

    [[nodiscard]] constexpr world_velocity operator+(world_velocity a, world_velocity b) noexcept {
        return {a.x + b.x, a.y + b.y};
    }
    [[nodiscard]] constexpr world_velocity operator-(world_velocity a, world_velocity b) noexcept {
        return {a.x - b.x, a.y - b.y};
    }
    [[nodiscard]] constexpr world_velocity operator*(world_velocity v, float s) noexcept {
        return {v.x * s, v.y * s};
    }
    [[nodiscard]] constexpr world_velocity operator*(float s, world_velocity v) noexcept { return v * s; }

    /// @brief Integrate a rate over a span: @c velocity * seconds = the offset travelled.
    [[nodiscard]] constexpr world_delta operator*(world_velocity v, world_seconds t) noexcept {
        return {v.x * t.count(), v.y * t.count()};
    }
    /// @copydoc operator*(world_velocity, world_seconds)
    [[nodiscard]] constexpr world_delta operator*(world_seconds t, world_velocity v) noexcept {
        return v * t;
    }

    /// @brief Differentiate a move over a span: @c delta / seconds = the rate it happened at.
    /// This is the EFFECTIVE velocity of a resolved move -- the quantity a blocked body should
    /// report ("how far did it actually get"), which is easy to confuse with a requested rate when
    /// both are bare float pairs.
    /// @pre @p t > 0 (a zero span has no rate); returns zero rather than infinity if violated.
    [[nodiscard]] constexpr world_velocity operator/(world_delta d, world_seconds t) noexcept {
        return t.count() > 0.0f ? world_velocity{d.x / t.count(), d.y / t.count()} : world_velocity{};
    }

    /// @brief An axis-aligned box in world space, as two corners.
    struct world_bounds {
        world_pos min{};
        world_pos max{};

        [[nodiscard]] constexpr world_delta size() const noexcept { return max - min; }
        [[nodiscard]] constexpr world_pos centre() const noexcept {
            return {(min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f};
        }
        [[nodiscard]] constexpr bool contains(world_pos p) const noexcept {
            return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y;
        }
        [[nodiscard]] constexpr bool intersects(world_bounds o) const noexcept {
            return !(o.min.x > max.x || o.max.x < min.x || o.min.y > max.y || o.max.y < min.y);
        }

        [[nodiscard]] friend constexpr bool operator==(world_bounds, world_bounds) = default;
    };

    // ---- input-edge spaces ---------------------------------------------------------------------
    // Separate types because the mapping between them is NOT the identity: logical presentation
    // scales and letterboxes, and HiDPI adds a further factor. Passing raw window pixels where
    // presentation-space coordinates belong is the KE mouse bug class -- a compile error here.

    /// @brief A position in RENDER space (logical / presentation pixels) -- what scenes draw in.
    struct render_pos {
        float x{};
        float y{};
        [[nodiscard]] friend constexpr bool operator==(render_pos, render_pos) = default;
    };

    /// @brief A position in WINDOW space (raw window pixels) -- what SDL delivers with input.
    /// Map to @ref render_pos with neutrino::to_render_coords(); never use one for the other.
    struct window_pos {
        float x{};
        float y{};
        [[nodiscard]] friend constexpr bool operator==(window_pos, window_pos) = default;
    };
}
