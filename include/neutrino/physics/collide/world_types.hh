//
// Public value types and handles that the `world` class is built on.
// Split out of world.hh for navigability.
//

/**
 * @file world_types.hh
 * @brief The public vocabulary of the @ref neutrino::physics::world API -- shapes, materials,
 *        filters, body DTOs, handles, and result/config types.
 *
 * This is the client-facing half of the world's type surface (game code includes this header
 * directly). It defines:
 *   - shape variants: @ref neutrino::physics::shape_t (any shape) and @ref
 *     neutrino::physics::moving_shape_t (movers only).
 *   - per-collider properties: @ref neutrino::physics::response_mode, @ref
 *     neutrino::physics::material_props, @ref neutrino::physics::filter_props.
 *   - the input DTOs handed to @ref neutrino::physics::world::add: @ref neutrino::physics::static_body,
 *     @ref neutrino::physics::kinematic_body, @ref neutrino::physics::carrier_body, @ref
 *     neutrino::physics::bullet, @ref neutrino::physics::tile_body.
 *   - the opaque handle @ref neutrino::physics::collider_id (plus its @c std::hash specialization).
 *   - query/event results: @ref neutrino::physics::contact, @ref neutrino::physics::footing, @ref
 *     neutrino::physics::event_kind, @ref neutrino::physics::world_event.
 *   - construction config: @ref neutrino::physics::world_config.
 *
 * @note The internal (@c detail) implementation types -- the grid tile payload, the resident/
 *       nonresident body records, the slot-map storage, @c eval_velocity_response, the shape
 *       narrow/widen helpers, and @c tight_box -- live in @ref world_internal.hh.
 */

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <type_traits>
#include <variant>

#include <neutrino/detail/hash.hh>
#include <vector>
#include <optional>

#include <neutrino/physics/geometry/shapes.hh>
#include <neutrino/physics/geometry/units.hh>
#include <neutrino/physics/collide/grid.hh>
#include <neutrino/physics/collide/dynamic_aabb_tree.hh>

namespace neutrino::physics {
    /// @brief Full shape set -- static bodies and broadphase targets may be any of these.
    using shape_t = std::variant <segment, aabb, circle, triangle>;
    /**
     * @brief Movers only (aabb | circle) -- the shape of a kinematic body or bullet.
     * @note A kinematic body / bullet is swept each frame, and a segment cannot be a swept mover
     *       (that is raycast's job). Restricting the type makes an invalid mover unconstructable and
     *       lets the swept dispatch cover every combination with no guard.
     */
    using moving_shape_t = std::variant <aabb, circle>;

    /// @brief How a surface responds to a contact -- the classifier read by the solver/trigger passes.
    enum class response_mode {
        BLOCK,   ///< Solid: stop the mover (the default).
        ONE_WAY, ///< Solid only when crossed on its @c block_normal face (jump-through floors).
        SENSOR,  ///< Never blocks; reported via TRIGGER_BEGIN/END overlap events.
        IGNORE   ///< Never blocks and never reported.
    };

    /// @brief A surface's physical response parameters (governs the velocity response on contact).
    struct material_props {
        // in [0, 1]. restitution > 1 injects energy
        //     relative velocity of separation after collision
        float restitution{0}; ///< Bounciness in [0,1] (e = v_separation / v_approach; >1 injects energy).
        float friction{0};    ///< Tangential damping in [0,1] (0 = ice, 1 = no slide; >1 reverses motion).

        response_mode response{response_mode::BLOCK}; ///< How this surface responds (BLOCK/ONE_WAY/SENSOR/IGNORE).
        vec block_normal{0, 1};                       ///< For ONE_WAY: the blocked face direction ("can't fall through the floor").
    };

    /// @brief Collision-layer filter: two colliders interact only when each side's category is in
    ///        the other's mask.
    struct filter_props {
        uint16_t category = 0xFFFF; ///< Bitmask of layers this collider belongs to.
        uint16_t mask = 0xFFFF;     ///< Bitmask of layers this collider collides with.
    };

    // Client DTOs (transient input to world::add). No inheritance: a kinematic/bullet shape is
    // a *narrower* type than a static one, so they are independent structs.

    /// @brief Input DTO for an immovable, free-form static body (any shape), stored in the BVH.
    struct static_body {
        shape_t shape;          ///< Any shape.
        material_props material; ///< Surface response.
        filter_props filter;     ///< Collision layers.
    };

    /// @brief Input DTO for a kinematic body (an "actor": player/enemy mover) resolved by move-and-slide.
    struct kinematic_body {
        moving_shape_t shape;    ///< Mover shape (aabb|circle).
        material_props material; ///< Surface response.
        filter_props filter;     ///< Collision layers.
        vec velocity;            ///< Initial velocity (world units / second).
    };

    /**
     * @brief Input DTO for a carrier (a "solid" in the actors-and-solids model): a kinematic body
     *        that moves RIGIDLY on a scripted path and transports the actors riding it.
     *
     * Moving platforms, elevators, crushers, conveyors. A distinct type from @ref kinematic_body
     * purely so @ref world::add tags it CARRIER. Each frame it translates by @c velocity*dt, and a
     * rider on top inherits @c (velocity+surface_velocity)*dt:
     *   - moving platform -> @c surface_velocity {0,0};
     *   - conveyor -> @c velocity {0,0}, @c surface_velocity = belt speed;
     *   - moving conveyor -> both set (the rider gets the sum).
     */
    struct carrier_body {
        moving_shape_t shape;    ///< Mover shape (aabb|circle).
        material_props material; ///< Surface response.
        filter_props filter;     ///< Collision layers.
        vec velocity{};          ///< The carrier's own rigid motion.
        vec surface_velocity{};  ///< Tangential drag imparted to riders without moving the carrier.
    };

    /// @brief Input DTO for a bullet -- a swept projectile resolved by one-way CCD (not in the tree).
    struct bullet {
        moving_shape_t shape;    ///< Mover shape (aabb|circle).
        material_props material; ///< Surface response.
        filter_props filter;     ///< Collision layers.
        vec velocity;            ///< Velocity (world units / second).
    };

    /**
     * @brief Input DTO for a static tile -- routed into the internal static grid instead of the BVH.
     *
     * Identical fields to @ref static_body, a distinct type purely so @ref world::add buckets it into
     * the grid cell containing its centre. The shape must fit within one cell -- larger or free-form
     * statics belong in @ref static_body (BVH). Slopes are just @c segment tiles, like resident walls.
     */
    struct tile_body {
        shape_t shape;           ///< Tile shape (must fit one grid cell).
        material_props material; ///< Surface response.
        filter_props filter;     ///< Collision layers.
        /**
         * @brief Opt this plain static solid tile into boundary-compilation.
         *
         * On the first @ref world::run the world merges adjacent mergeable cell-filling BLOCK aabb
         * tiles (same material+filter) into bigger AABB residents, removing the internal tile seams
         * that snag fast movers.
         * @note Merged tiles lose their per-cell TILE handle (it goes invalid), so leave this @c false
         *       for tiles you address individually (destructibles, sensors, slopes -- which also
         *       can't merge anyway).
         */
        bool mergeable = false;
    };

    /**
     * @brief An opaque, generation-checked handle to a collider (body, bullet, or tile).
     *
     * Returned by @ref world::add and carried in events/queries. A value-initialised handle
     * (@c collider_id{}) is the null sentinel. Identity is (value, generation, type_id); the
     * generation lets a stale handle to a recycled slot/cell read as invalid rather than aliasing a
     * new collider.
     */
    struct collider_id {
        /// @brief Which storage the handle addresses.
        enum type {
            BODY,   ///< A resident body slot.
            BULLET, ///< A bullet slot.
            TILE    ///< A static grid cell (@c value is the linear cell handle; eid lives in the payload).
        };

        static constexpr uint32_t INVALID = 0xFFFFFFFFu; ///< Null-handle sentinel for @c value.

        // Defaults make a value-initialised handle (collider_id{}) the null sentinel rather than
        // aliasing slot 0 generation 0. world::is_valid() also rejects it (INVALID is out of
        // range for any pool). Aggregate init collider_id{idx, gen, type} is unaffected.
        uint32_t value = INVALID;   ///< Slot index / linear cell handle (@ref INVALID = null).
        uint32_t generation = 0;    ///< Recycle counter, matched against the storage to detect staleness.
        type type_id = BODY;        ///< Which storage @c value refers to.

        /**
         * @brief Is this not the null sentinel?
         * @return @c true unless @c value == @ref INVALID.
         * @note Distinct from @ref world::is_valid, which also checks liveness + generation.
         */
        [[nodiscard]] bool valid() const { return value != INVALID; }

        // Full-identity comparison (value, generation, type_id in that order). Defaulting these
        // keeps collider_id an aggregate (no user-provided ctor), so collider_id{idx, gen, type}
        // still works. == / != are usable as map keys; <=> gives ordering for std::set / sorting.
        [[nodiscard]] friend bool operator==(const collider_id&, const collider_id&) = default;
        [[nodiscard]] friend std::strong_ordering operator<=>(const collider_id&, const collider_id&) = default;
    };

    /// @brief A single resolved contact: who was hit, the outward surface normal, and the time of impact.
    struct contact {
        collider_id who; ///< The collider hit.
        vec normal;      ///< Outward surface normal at impact (for slide / ricochet).
        float toi;       ///< Time of impact, normalized [0,1] along the query delta.
    };

    /**
     * @brief The result of @ref world::ground_support -- the WALKABLE ground under three
     *        footprint probes -- left edge, centre, right edge.
     *
     * The left/right pair is exactly Sonic's twin floor sensors. Each member is the probe's contact,
     * or @c std::nullopt when that foot hangs over nothing walkable within reach (a void, a sensor,
     * or a too-steep face). A STATE the game reads to drive teeter/balance, edge-stop, coyote-time
     * and ledge-grab -- not a solver behaviour.
     */
    struct footing {
        std::optional <contact> left;   ///< Ground under the left footprint edge.
        std::optional <contact> centre; ///< Ground under the footprint centre.
        std::optional <contact> right;  ///< Ground under the right footprint edge.

        /// @brief Is any foot supported?
        [[nodiscard]] bool grounded() const noexcept { return left || centre || right; }
        /// @brief Are all three feet supported?
        [[nodiscard]] bool fully_supported() const noexcept { return left && centre && right; }
        /// @brief At a ledge: standing on something, but part of the footprint hangs off (teeter cue).
        [[nodiscard]] bool at_ledge() const noexcept { return grounded() && !fully_supported(); }
        /// @brief The drop is off the left side: true iff the left foot hangs (centre or right hold).
        [[nodiscard]] bool ledge_left() const noexcept { return !left && (centre || right); }
        /// @brief The drop is off the right side: true iff the right foot hangs (centre or left hold).
        [[nodiscard]] bool ledge_right() const noexcept { return !right && (centre || left); }
    };

    /// @brief The kind of a @ref world_event produced by @ref world::run.
    enum class event_kind {
        COLLISION,      ///< A kinematic mover hit a solid surface.
        BULLET_HIT,     ///< A bullet hit a solid surface.
        BULLET_EXPIRED, ///< A bullet left the world bounds; the game should despawn it.
        TRIGGER_BEGIN,  ///< A sensor overlap began this frame.
        TRIGGER_END,    ///< A sensor overlap ended this frame.
        CRUSH           ///< A carrier pinned an actor against other solid geometry (couldn't move clear).
    };

    /// @brief One event emitted by @ref world::run, naming the two colliders and the contact data.
    struct world_event {
        /**
         * @brief Construct an event.
         * @param kind_   The event kind.
         * @param mover_  The acting collider (mover / bullet / sensor / crushed actor).
         * @param target_ The other collider (the surface hit / the other sensor / the carrier).
         * @param normal_ Contact normal where meaningful (else zero).
         * @param toi_    Time of impact where meaningful (else @c -1).
         */
        world_event(event_kind kind_, const collider_id& mover_, const collider_id& target_, const vec& normal_,
                    float toi_)
            : kind(kind_),
              mover(mover_),
              target(target_),
              normal(normal_),
              toi(toi_) {
        }

        event_kind kind{event_kind::COLLISION}; ///< What happened.
        collider_id mover{};                    ///< The acting collider.
        collider_id target{};                   ///< The other collider.
        vec normal{};                           ///< Contact normal (zero when not applicable).
        float toi{-1.0f};                        ///< Time of impact (@c -1 when not applicable).
    };

    /// @brief Construction-time configuration for a @ref world (passed to its constructor).
    struct world_config {
        float fatten_margin = 0.1f; ///< Broadphase fat-AABB margin for moving bodies.
        float skin = 0.01f;         ///< Back-off so the mover never quite touches (anti-jitter).
        int max_slide_iter = 4;     ///< Move-and-slide corner passes (a floor+wall corner needs 2).
        vec up = {0, 1};            ///< Up-axis for grounded detection (matches @c block_normal default).
        /**
         * @brief Play-field extent; unset = unbounded.
         * @note When set, a bullet that leaves it gets a @c BULLET_EXPIRED event (bullets only --
         *       kinematic bodies are never auto-expired). This is the LEVEL extent, NOT the camera
         *       (@c active_region): a bullet scrolling off-camera is culled, not expired.
         */
        std::optional <aabb> bounds{};

        /**
         * @brief Static tile grid configuration; unset = pure-dynamic world (no grid;
         *        @c add(tile_body) is rejected).
         * @note When set, the grid's extent IS @c bounds (one shared world box -- no separate,
         *       mismatched grid box), divided into cells of @c tile_size. @c bounds must therefore be
         *       present and an integer multiple of @c tile_size (enforced at construction).
         */
        struct grid_config {
            vec tile_size{1, 1}; ///< Per-cell size.
        };
        std::optional <grid_config> grid{}; ///< The tile grid, or unset for a pure-dynamic world.
    };

    /// @brief Test-only access to the world's internal (non-public) query helpers.
    /// @note Defined only in the test TU; lets the brute-force suite reach @c cast / @c overlap
    ///       without widening the public API.
    struct world_test_access;
}

/**
 * @brief @c std::hash specialization so @ref neutrino::physics::collider_id keys an
 *        @c unordered_map / @c unordered_set.
 *
 * Lives beside @c collider_id so a consumer of the lightweight @ref world_types.hh alone can use
 * unordered containers of handles. Mixes all three identity fields so handles to different
 * slots/generations/types hash apart.
 */
template <>
struct std::hash<neutrino::physics::collider_id> {
    [[nodiscard]] std::size_t operator()(const neutrino::physics::collider_id& id) const noexcept {
        std::size_t h = neutrino::details::hash_value(id.value);
        neutrino::details::hash_combine(h, neutrino::details::hash_value(id.generation));
        neutrino::details::hash_combine(h, neutrino::details::hash_value(static_cast<std::uint32_t>(id.type_id)));
        return h;
    }
};
