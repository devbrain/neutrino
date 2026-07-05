//
// Created by igor on 21/06/2026.
//

/**
 * @file world.hh
 * @brief The 2D collision @ref neutrino::physics::world -- the central runtime that owns every
 *        collider, advances them one frame at a time, and answers spatial queries.
 *
 * The @ref neutrino::physics::world is an "actors-and-solids" collision world for a 2D action/
 * platformer game. It stores four flavours of collider, each added through @ref
 * neutrino::physics::world::add :
 *   - **static bodies** -- immovable free-form geometry (any shape), held in a dynamic AABB tree
 *     (the broadphase BVH).
 *   - **kinematic bodies** ("actors") -- player/enemy movers (aabb|circle) resolved by
 *     move-and-slide each frame.
 *   - **carrier bodies** ("solids") -- kinematic platforms/elevators/crushers/conveyors that move
 *     rigidly on a scripted path and carry, push, and crush the actors they touch.
 *   - **bullets** -- swept point/small movers that are NOT in the tree (one-way CCD against it).
 *   - **tiles** -- axis-aligned static cells in an optional uniform @ref neutrino::physics::grid,
 *     the level's tilemap; mergeable runs are baked into big residents on the first frame.
 *
 * Every collider is addressed by an opaque, generation-checked @ref neutrino::physics::collider_id
 * handle (so a stale handle to a recycled slot reads as invalid rather than aliasing a new body).
 *
 * A frame is driven by @ref neutrino::physics::world::run , which executes four ordered passes
 * (carriers -> movement -> bullets -> triggers) and returns the @ref neutrino::physics::world_event
 * list produced this step. Outside the step the world answers const spatial queries
 * (@ref neutrino::physics::world::cast , @ref neutrino::physics::world::raycast and their multi-hit
 * variants) and offers a small set of character-controller helpers
 * (@ref neutrino::physics::world::snap_to_ground , @ref neutrino::physics::world::step_up ,
 * @ref neutrino::physics::world::ground_support ).
 *
 * @note The supporting value types (collider_id, contact, footing, world_event, world_config, the
 *       body DTOs) live in @ref world_types.hh .
 */

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <type_traits>
#include <variant>
#include <vector>
#include <optional>

#include <neutrino/physics/geometry/shapes.hh>
#include <neutrino/physics/geometry/units.hh>
#include <neutrino/physics/collide/grid.hh>
#include <neutrino/physics/collide/dynamic_aabb_tree.hh>
#include <neutrino/physics/collide/world_internal.hh> // detail:: internals + (transitively) world_types.hh

namespace neutrino::physics {
    /**
     * @brief An actors-and-solids 2D collision world: owns every collider, steps them a frame at a
     *        time, and answers spatial queries.
     *
     * See the @ref world.hh file overview for the collider taxonomy and the per-frame pass order.
     * Construct with a @ref world_config (extent, skin, up-axis, optional tile grid), populate via
     * the @ref add overloads, advance with @ref run, and read results back through the @ref get_shape
     * / @ref get_velocity getters or the const query API.
     *
     * @note Not copyable in spirit (it owns broadphase nodes and slot-map storage); pass by
     *       reference. Handles returned by @ref add stay valid until the collider is removed,
     *       overwritten, or @ref clear is called -- a generation counter makes stale handles read
     *       as invalid rather than aliasing a recycled slot.
     */
    class world {
        public:
            // ====================================================================================
            // Construction & lifecycle: ctor, add/remove/clear, set_* intent, get_* read-back, run().
            // ====================================================================================

            /// @brief Construct an empty, unconfigured world (no bounds, no grid, default skin/up).
            world() = default;

            /**
             * @brief Construct a world from a configuration.
             * @param cfg Extent (@c bounds), collision skin, up-axis, slide-iteration cap, and an
             *            optional static tile @c grid.
             * @note When @c cfg.grid is set, @c cfg.bounds MUST also be set and be an exact integer
             *       multiple of the grid's @c tile_size (the grid shares the world's coordinate
             *       frame); both are enforced here and abort loudly on violation.
             */
            explicit world(const world_config& cfg);

            /// @brief Grants the test harness access to the private query helpers (see @ref world_types.hh).
            friend struct world_test_access;

            /**
             * @brief Add an immovable, free-form static body to the broadphase.
             * @param eid  Game entity id carried as payload (echoed back by @ref get_eid and in events).
             * @param body Shape (any @ref shape_t), material, and collision filter.
             * @return A live @c BODY handle.
             */
            collider_id add(entity_id_t eid, const static_body& body);

            /**
             * @brief Add a kinematic body (an "actor": player/enemy mover) resolved by move-and-slide.
             * @param eid  Game entity id carried as payload.
             * @param body Mover shape (aabb|circle), material, filter, and initial velocity.
             * @return A live @c BODY handle.
             */
            collider_id add(entity_id_t eid, const kinematic_body& body);

            /**
             * @brief Add a carrier body (a "solid": moving platform / elevator / crusher / conveyor).
             * @param eid  Game entity id carried as payload.
             * @param body Mover shape (aabb|circle), material, filter, rigid @c velocity, and the
             *             conveyor @c surface_velocity imparted to riders.
             * @return A live @c BODY handle. Carriers are stepped before actors so actors see them at
             *         their resolved positions.
             */
            collider_id add(entity_id_t eid, const carrier_body& body);

            /**
             * @brief Add a bullet -- a swept projectile resolved by one-way CCD against the world.
             * @param eid  Game entity id carried as payload.
             * @param body Mover shape (aabb|circle), material, filter, and velocity.
             * @return A live @c BULLET handle. Bullets are not inserted into the tree (nothing
             *         collides against them); they sweep against everything else and emit
             *         @c BULLET_HIT / @c BULLET_EXPIRED events.
             */
            collider_id add(entity_id_t eid, const bullet& body);

            /**
             * @brief Add a static tile into the grid.
             *
             * The tile is bucketed into the single cell containing its shape's centre (so the shape
             * must fit within one cell, enforced). Overwrites any tile already in that cell
             * (loader-friendly); the overwrite bumps the cell's generation, so a handle to the
             * previous tile goes invalid (no silent aliasing).
             *
             * @param eid  Game entity id; recovered from the cell payload by @ref get_eid.
             * @param body Tile shape, material, filter, and the @c mergeable opt-in flag.
             * @return A @c TILE handle whose @c value is the linear cell index and @c generation is
             *         the cell's current generation.
             * @warning Requires a configured grid (@c world_config.grid). A @c mergeable tile must be
             *          added before the first @ref run (static geometry is baked once); adding one
             *          afterwards aborts.
             */
            collider_id add(entity_id_t eid, const tile_body& body);

            /**
             * @brief Remove a collider (body, bullet, or tile) by handle.
             * @param cid Handle to remove; a stale/invalid handle is a no-op (safe to double-remove).
             *            The freed slot/cell bumps its generation so @c cid reads invalid afterwards.
             */
            void remove(collider_id cid);

            /**
             * @brief Empty the world for a level reload.
             *
             * Drops every body, bullet and tile, the broadphase tree, and the trigger/event buffers,
             * while keeping the configuration (bounds, grid, skin, ...) intact and retaining capacity
             * for reuse. The one-shot tile bake is reset so a fresh level may bake again.
             *
             * @note All storages keep their generation counters monotonic, so any handle held across
             *       @ref clear reads as invalid afterwards rather than aliasing a reused slot/cell.
             */
            void clear();

            /**
             * @brief Resize or teleport a collider in place (replace its shape).
             * @param cid   Handle to a live body, bullet, or tile.
             * @param shape New shape as the wide @ref shape_t.
             * @note A moving body/bullet must stay an aabb|circle (a segment/triangle would later
             *       trip the swept narrow-phase); a tile's new shape must still fit its current cell;
             *       a mergeable tile cannot be reshaped after the first @ref run. Violations abort.
             */
            void set_shape(collider_id cid, const shape_t& shape);

            /**
             * @brief Set a mover's velocity -- the kinematic intent applied on the next @ref run.
             * @param cid Handle to a kinematic actor, carrier, or bullet (NOT a static body or tile).
             * @param v   New velocity (world units / second).
             */
            void set_velocity(collider_id cid, const vec& v);

            /**
             * @brief Set a carrier's conveyor belt speed -- the tangential drag imparted to riders.
             * @param cid Handle to a @c CARRIER body (carrier-only; aborts otherwise).
             * @param v   Belt velocity added to a rider's motion; @c {0,0} switches the belt off.
             *            The carrier's own rigid motion is set separately via @ref set_velocity.
             */
            void set_surface_velocity(collider_id cid, const vec& v);

            /**
             * @brief Is this handle still live -- alive AND matching the current generation?
             * @param cid Handle to test (the null sentinel @c collider_id{} reads false).
             * @return @c true iff the referenced body/bullet/tile exists and has not been
             *         removed/overwritten/cleared since @c cid was issued.
             */
            [[nodiscard]] bool is_valid(collider_id cid) const;

            // ---- read-back getters (state after run()/move) -------------------------
            // A character controller reads these each frame: the resolved shape/position and the
            // post-move velocity. All ENFORCE a live handle. Returned by value -- the underlying
            // record may be recycled later. Shapes come back as the wide shape_t (a bullet's
            // moving_shape_t is widened) so the caller has one type to visit.

            /**
             * @brief The collider's current (post-@ref run) shape, by value.
             * @param cid Handle to a live collider (aborts if invalid).
             * @return The shape as the wide @ref shape_t (a bullet's moving shape is widened) so the
             *         caller has one type to @c std::visit.
             */
            [[nodiscard]] shape_t get_shape(collider_id cid) const;

            /**
             * @brief The collider's current velocity (post-slide for actors).
             * @param cid Handle to a live collider (aborts if invalid).
             * @return The stored velocity; @c {0,0} for a static body or tile.
             */
            [[nodiscard]] vec get_velocity(collider_id cid) const;

            /**
             * @brief The game entity id carried as payload (the reverse of @ref add 's @c eid argument).
             * @param cid Handle to a live collider (aborts if invalid).
             * @return The @c entity_id_t supplied when the collider was added.
             */
            [[nodiscard]] entity_id_t get_eid(collider_id cid) const;

            /**
             * @brief Advance the world by one frame and return the events produced this step.
             *
             * Runs four ordered passes: carriers (carry/push/crush) -> kinematic movement
             * (move-and-slide) -> bullets -> sensor triggers. On the first call it also bakes the
             * mergeable static tiles once.
             *
             * @param active_region The camera/simulation window; movers and bullets whose swept bound
             *                       does not touch it are left dormant (sensors are never region-culled).
             * @param dt            Frame duration in seconds.
             * @return A reference to the world's internal event buffer, valid until the next @ref run
             *         or @ref clear. Contains @c COLLISION, @c BULLET_HIT, @c BULLET_EXPIRED,
             *         @c TRIGGER_BEGIN / @c TRIGGER_END, and @c CRUSH events.
             */
            [[nodiscard]] const std::vector <world_event>& run(const aabb& active_region, float dt);

        private:
            // ====================================================================================
            // Internals, part 1: run()'s per-phase passes, then the broadphase glue and query core
            // (fatten / swept_bound / material_of / tile_handle, the fan-out + narrow-phase plumbing,
            // and overlap_core / cast_core that the public queries below are built on).
            // ====================================================================================
            // ---- run() passes (each owns one phase of a step; orchestrated by run() above) -------

            /**
             * @brief Frame pass 1 -- carriers: move each carrier rigidly on its scripted path and
             *        carry (MP1), push (MP2), and crush (MP3) the actors it touches.
             * @param active_region Off-region carriers stay dormant (like off-region movers).
             * @param dt            Frame duration. Runs BEFORE the movement pass so actors then see
             *                      carriers at their resolved positions.
             */
            void carrier_pass(const aabb& active_region, float dt) {
                // Carrier pass (actors-and-solids): each carrier moves RIGIDLY on its scripted path
                // and transports the actors riding it. Runs BEFORE the actor movement pass, so actors
                // then see carriers at their resolved positions. A rider inherits
                // (velocity + surface_velocity)*dt -- the platform's own delta plus any conveyor drag
                // -- collision-aware against everything but its carrier. (MP1: carrying only; pushing
                // and crushing are follow-ups.) Carriers are body_kind::CARRIER, so the movement pass
                // below (which only processes KINEMATIC) leaves them alone.
                for (auto it = m_bodies_storage.begin(); it != m_bodies_storage.end(); ++it) {
                    if (it->kind != detail::body_kind::CARRIER) {
                        continue;
                    }
                    const vec body_delta{it->velocity.x() * dt, it->velocity.y() * dt};
                    const vec rider_delta{
                        (it->velocity.x() + it->surface_velocity.x()) * dt,
                        (it->velocity.y() + it->surface_velocity.y()) * dt
                    };
                    if (near_zero(body_delta) && near_zero(rider_delta)) {
                        continue; // a stationary carrier with no belt: nothing to do
                    }
                    if (!intersects(swept_bound(detail::narrow(it->shape), units::displacement{body_delta}),
                                    active_region)) {
                        continue; // off-region carrier: dormant (like off-region movers)
                    }
                    const uint32_t carrier_idx = it.index();
                    // Collect riders at the carrier's CURRENT position, before it moves. (Linear scan
                    // -- carriers are few; a tree query is a perf follow-up.)
                    m_rider_scratch.clear();
                    for (auto jt = m_bodies_storage.begin(); jt != m_bodies_storage.end(); ++jt) {
                        if (jt->kind == detail::body_kind::KINEMATIC && is_riding(jt.index(), carrier_idx)) {
                            m_rider_scratch.push_back(jt.index());
                        }
                    }
                    move_carrier_rigid(carrier_idx, body_delta);
                    for (const uint32_t r : m_rider_scratch) {
                        carry_translate(r, carrier_idx, rider_delta);
                        emit_crush_if_pinned(r, carrier_idx, rider_delta); // MP3: carried into a wall
                    }

                    // MP2 -- pushing: a carrier that MOVED (body_delta != 0) shoves any non-rider
                    // actor its sweep RAN INTO clear along its motion. (A conveyor with body_delta 0
                    // does not push -- it only drags riders.) The candidate gate is a swept test of
                    // the carrier (from its start, by body_delta) vs the actor: this is anti-tunnel
                    // (a fast carrier still catches a thin actor anywhere in the band) AND directional
                    // (a carrier moving AWAY from a trailing actor never hits it). Each hit is shoved
                    // to the carrier's FINAL leading edge via the collision-aware carry_translate
                    // (blocked at a wall it stops, leaving a residual overlap MP3 reads as a crush).
                    // Collect-then-push -- pushing mutates the tree and must not run during the query.
                    if (!near_zero(body_delta)) {
                        const vec back{-body_delta.x(), -body_delta.y()};
                        const aabb fbox = detail::tight_box(m_bodies_storage[carrier_idx].shape);
                        const aabb band = aabb::combine(fbox, physics::translate(fbox, back)); // start .. final sweep
                        const moving_shape_t cstart = detail::narrow(std::visit([&](const auto& s) {
                            return shape_t{physics::translate(s, back)};
                        }, m_bodies_storage[carrier_idx].shape));

                        m_push_scratch.clear();
                        query(m_space_partition, band, [&](entity_id_t actor_idx, const aabb&) {
                            if (actor_idx == carrier_idx) {
                                return;
                            }
                            const auto& act = m_bodies_storage[actor_idx];
                            if (act.kind != detail::body_kind::KINEMATIC
                                || !should_collide(act.filter, m_bodies_storage[carrier_idx].filter)) {
                                return; // only actors that interact with the carrier are pushed
                            }
                            for (const uint32_t r : m_rider_scratch) {
                                if (r == actor_idx) {
                                    return; // already handled as a rider (carried, not pushed)
                                }
                            }
                            // Directional gate: ignore an actor entirely BEHIND the carrier's start
                            // along the motion (the carrier moves away from it). This drops a body
                            // merely touching the carrier's trailing edge, which the swept test below
                            // would otherwise report as a toi-0 initial-overlap hit regardless of direction.
                            const aabb sbox = physics::translate(fbox, back);
                            const aabb abox0 = detail::tight_box(act.shape);
                            const float e = constants::POINT_EPS;
                            if ((body_delta.x() > e && abox0.max.x() <= sbox.min.x() + e)
                                || (body_delta.x() < -e && abox0.min.x() >= sbox.max.x() - e)
                                || (body_delta.y() > e && abox0.max.y() <= sbox.min.y() + e)
                                || (body_delta.y() < -e && abox0.min.y() >= sbox.max.y() - e)) {
                                return;
                            }
                            const auto swept = swept_vs_shape(cstart, body_delta, act.shape);
                            if (!swept) {
                                return; // the carrier's sweep doesn't reach the actor
                            }
                            // The carrier pushes only if it is SOLID for this contact -- same rule as
                            // riding (a SENSOR/IGNORE carrier pushes nothing; a one-way carrier only
                            // from its blocked face). Use the ACTUAL swept contact normal, oriented to
                            // point in the carrier's motion hemisphere (its push side): this normalizes
                            // the per-shape-pair convention difference (swept's outward side is the aabb
                            // for aabb-vs-circle but the target for aabb-vs-aabb) while keying one-way on
                            // the real contact face -- a motion-direction guess mis-evaluates diagonal
                            // carriers (a face hit can differ from the velocity direction).
                            vec n = swept->entry_normal;
                            if (euler::dot(n, body_delta) < 0.0f) {
                                n = vec{-n.x(), -n.y()};
                            }
                            if (solid_pred{}(m_bodies_storage[carrier_idx].material, n)) {
                                m_push_scratch.push_back(actor_idx);
                            }
                        });
                        for (const uint32_t aidx : m_push_scratch) {
                            const aabb abox = detail::tight_box(m_bodies_storage[aidx].shape);
                            const vec push = clear_push(abox, fbox, body_delta);
                            carry_translate(aidx, carrier_idx, push);
                            emit_crush_if_pinned(aidx, carrier_idx, push); // MP3: pushed into a wall
                        }
                    }
                }
            }

            /**
             * @brief Frame pass 2 -- movement: resolve each kinematic actor via move-and-slide and
             *        record its contacts as @c COLLISION events.
             * @param active_region Off-region movers are culled (left dormant off-screen).
             * @param dt            Frame duration. Runs BEFORE the bullet pass so bullets sweep
             *                      against movers at their resolved positions.
             */
            void movement_pass(const aabb& active_region, float dt) {
                // Movement pass: resolve each kinematic mover via move-and-slide against the
                // solid residents, in fixed slot order (deterministic). Off-region movers are
                // culled (skipped, so they stay dormant off-screen); statics and zero-velocity
                // bodies never move. Each recorded contact becomes a COLLISION event. Runs BEFORE
                // the bullet pass so bullets sweep against movers at their resolved positions.
                for (auto it = m_bodies_storage.begin(); it != m_bodies_storage.end(); ++it) {
                    if (it->kind != detail::body_kind::KINEMATIC) {
                        continue;
                    }
                    const units::displacement delta = units::velocity{it->velocity} * units::duration{dt};
                    if (near_zero(delta.value)) {
                        continue; // not moving this frame
                    }
                    if (!intersects(swept_bound(detail::narrow(it->shape), delta), active_region)) {
                        continue; // off-region: dormant
                    }
                    const uint32_t mover_idx = it.index();
                    const slide_result res = move_and_slide(mover_idx, units::duration{dt}, solid_acceptor());
                    for (int i = 0; i < res.count; ++i) {
                        m_events.emplace_back(
                            event_kind::COLLISION,
                            collider_id{mover_idx, m_bodies_storage.generation(mover_idx), collider_id::BODY},
                            res.contacts[i].who,
                            res.contacts[i].normal,
                            res.contacts[i].toi);
                    }
                }
            }

            /**
             * @brief Frame pass 3 -- bullets: integrate every live bullet, cast it against the solids,
             *        and emit @c BULLET_HIT / @c BULLET_EXPIRED events.
             * @param active_region Off-region bullets keep flying (so they can re-enter) but skip the
             *                      expensive tree query.
             * @param dt            Frame duration. A bullet leaving @c world_config.bounds expires.
             */
            void bullet_pass(const aabb& active_region, float dt) {
                // Bullet pass: integrate every live bullet, but only pay for the cast when its
                // swept bound touches the active region (off-region bullets keep flying so they
                // can re-enter, they just skip the expensive tree query). The game despawns
                // out-of-bounds bullets so they do not accumulate. toi is reported normalized.
                for (auto bullet_itr = m_bullets_storage.begin(); bullet_itr != m_bullets_storage.end(); ++bullet_itr) {
                    units::displacement delta_s = units::velocity{bullet_itr->velocity} * units::duration{dt};
                    if (intersects(swept_bound(bullet_itr->shape, delta_s), active_region)) {
                        // Bullets hit only solids -- a sensor/ignored body must not stop them
                        // (sensors detect via the trigger pass; bullets are not in the tree anyway).
                        if (auto hit = cast(bullet_itr.index(), collider_id::BULLET, delta_s, solid_acceptor())) {
                            m_events.emplace_back(
                                event_kind::BULLET_HIT,
                                collider_id{bullet_itr.index(), bullet_itr->generation, collider_id::BULLET},
                                hit->who,
                                hit->normal,
                                hit->toi);
                            delta_s = delta_s * units::fraction{hit->toi}; // toi is the fraction ALONG delta_s
                        }
                    }
                    translate(*bullet_itr, delta_s.value);

                    // Out-of-bounds: a bullet that no longer overlaps the world bounds has left
                    // the level -> report it (the game despawns in reaction; the handle is still
                    // live this frame). Only checked when bounds are configured.
                    if (m_cfg.bounds
                        && !intersects(swept_bound(bullet_itr->shape, units::displacement{}), *m_cfg.bounds)) {
                        m_events.emplace_back(
                            event_kind::BULLET_EXPIRED,
                            collider_id{bullet_itr.index(), bullet_itr->generation, collider_id::BULLET},
                            collider_id{}, vec{}, -1.0f);
                    }
                }
            }

            /**
             * @brief Frame pass 4 -- triggers: detect SENSOR overlaps and diff against last frame to
             *        emit only the @c TRIGGER_BEGIN / @c TRIGGER_END edges.
             * @note Scans both sensor bodies and the side-list of sensor tiles. Sensors are NOT
             *       region-culled (culling would spuriously fire end/begin as zones scroll). There is
             *       no per-frame "still touching" event -- continuous effects are the game's job
             *       between edges.
             */
            void trigger_pass() {
                // Trigger pass: detect SENSOR overlaps and diff against last frame to emit only
                // the begin/end EDGES. (response_mode is the classifier: solids were handled by
                // the movement pass as COLLISION; sensors are reported here -- there is no generic
                // per-frame "touch" event, continuous effects are the game's job between edges.)
                // Sensors are NOT region-culled: culling would spuriously fire end/begin as zones
                // scroll. The sensor's own filter decides what it senses.
                m_triggers_curr.clear();
                for (auto it = m_bodies_storage.begin(); it != m_bodies_storage.end(); ++it) {
                    if (it->material.response != response_mode::SENSOR) {
                        continue;
                    }
                    const uint32_t sidx = it.index();
                    const collider_id sensor_id{sidx, m_bodies_storage.generation(sidx), collider_id::BODY};
                    overlap(sidx, [&](collider_id other) {
                        m_triggers_curr.push_back({pair_key(sensor_id, other), sensor_id, other});
                    });
                }
                // Sensor TILES: the bodies loop above can't see them. Scan the side-list, pruning
                // stale entries (cell overwritten/cleared since -> generation mismatch) by swap-pop.
                // A sensor tile senses bodies only (query_tiles=false): tiles are static, so
                // tile-vs-tile overlaps never change frame to frame and carry no trigger meaning.
                for (std::size_t i = 0; i < m_sensor_tiles.size();) {
                    const collider_id st = m_sensor_tiles[i];
                    if (!is_valid(st)) {
                        m_sensor_tiles[i] = m_sensor_tiles.back();
                        m_sensor_tiles.pop_back();
                        continue;
                    }
                    const detail::tile& t = *m_static_grid->at(st.value);
                    overlap_core(t.shape, t.filter, collider_id::INVALID, /*query_tiles=*/false,
                                 [&](collider_id other) {
                                     m_triggers_curr.push_back({pair_key(st, other), st, other});
                                 });
                    ++i;
                }
                std::sort(m_triggers_curr.begin(), m_triggers_curr.end(),
                          [](const sensor_pair& a, const sensor_pair& b) { return a.key < b.key; });
                // Dedup: a sensor-vs-sensor overlap is produced once from each side (same key).
                m_triggers_curr.erase(
                    std::unique(m_triggers_curr.begin(), m_triggers_curr.end(),
                                [](const sensor_pair& a, const sensor_pair& b) { return a.key == b.key; }),
                    m_triggers_curr.end());

                // Linear merge of the two key-sorted sets: curr-only -> BEGIN, prev-only -> END,
                // in-both -> still inside (no event). END uses the PREVIOUS frame's stored ids, so
                // a pair that vanished because a body was removed still reports a clean end.
                std::size_t i = 0, j = 0;
                while (i < m_triggers_curr.size() && j < m_triggers_prev.size()) {
                    if (m_triggers_curr[i].key < m_triggers_prev[j].key) {
                        emit_trigger(event_kind::TRIGGER_BEGIN, m_triggers_curr[i++]);
                    } else if (m_triggers_prev[j].key < m_triggers_curr[i].key) {
                        emit_trigger(event_kind::TRIGGER_END, m_triggers_prev[j++]);
                    } else {
                        ++i;
                        ++j;
                    }
                }
                for (; i < m_triggers_curr.size(); ++i) {
                    emit_trigger(event_kind::TRIGGER_BEGIN, m_triggers_curr[i]);
                }
                for (; j < m_triggers_prev.size(); ++j) {
                    emit_trigger(event_kind::TRIGGER_END, m_triggers_prev[j]);
                }
                std::swap(m_triggers_curr, m_triggers_prev); // this frame becomes "previous"
            }

            /**
             * @brief The fattened broadphase box for a resident: its tight box expanded by
             *        @c fatten_margin, biased along the velocity so a moving body's proxy needs
             *        re-grafting less often.
             * @param body The resident whose stored shape/velocity/kind drive the margin.
             * @return The tight box for a STATIC body; otherwise a velocity-biased fat box.
             */
            [[nodiscard]] aabb fatten(const detail::resident_body& body) const {
                auto box = std::visit([](const auto& shape) {
                    return enclose(shape);
                }, body.shape);

                if (body.kind == detail::body_kind::STATIC) {
                    return box;
                }

                const auto vx = body.velocity.x();
                const auto vy = body.velocity.y();

                auto min_x = box.min.x();
                auto min_y = box.min.y();
                auto max_x = box.max.x();
                auto max_y = box.max.y();

                if (vx < 0) {
                    min_x -= m_cfg.fatten_margin;
                } else if (vx > 0) {
                    max_x += m_cfg.fatten_margin;
                } else {
                    const auto epsilon = m_cfg.fatten_margin / 2.0f;
                    min_x -= epsilon;
                    max_x += epsilon;
                }

                if (vy < 0) {
                    min_y -= m_cfg.fatten_margin;
                } else if (vy > 0) {
                    max_y += m_cfg.fatten_margin;
                } else {
                    const auto epsilon = m_cfg.fatten_margin / 2.0f;
                    min_y -= epsilon;
                    max_y += epsilon;
                }

                return {{min_x, min_y}, {max_x, max_y}};
            }

            /**
             * @brief Category/mask layer test -- do two filters mutually agree to interact?
             * @param a,b The two collision filters.
             * @return @c true iff each side's category is in the other's mask.
             */
            static bool should_collide(const filter_props& a, const filter_props& b) {
                return ((a.category & b.mask) && (b.category & a.mask));
            }

            /**
             * @brief AABB enclosing a mover's shape over the whole sweep (start box ∪ end box).
             * @param shape The mover (aabb|circle).
             * @param delta The frame displacement.
             * @return The swept envelope, used as the broadphase query box for @ref cast and as the
             *         active-region cull test.
             */
            static aabb swept_bound(const moving_shape_t& shape, units::displacement delta) {
                const vec d = delta.value;
                return std::visit([&d](const auto& s) {
                    const auto start = enclose(s);
                    return aabb{
                        {
                            std::min(start.min.x() + d.x(), start.min.x()),
                            std::min(start.min.y() + d.y(), start.min.y())
                        },
                        {
                            std::max(start.max.x() + d.x(), start.max.x()),
                            std::max(start.max.y() + d.y(), start.max.y())
                        }
                    };
                }, shape);
            }

            // Cast acceptors run per candidate AFTER its swept contact is computed and receive the
            // CONTACT NORMAL, so a direction-dependent rule (ONE_WAY) decides on the face actually
            // crossed -- not on velocity guessed before the geometry is known.
            //
            // "Is this surface solid for the mover, given it was hit on `hit_normal`?"
            //   BLOCK   -> always solid.
            //   ONE_WAY -> solid only when the mover crossed the blocked face, i.e. the contact
            //              normal aligns with the surface's block_normal (dot > threshold). A
            //              jump-through floor (block_normal = up): landing on top -> normal up ->
            //              blocked; entering from below or the side -> normal != up -> passes.
            //   SENSOR / IGNORE -> never solid.
            // Functor (concrete type, not a lambda) so it is usable in run() above its definition.
            // Acceptors take the target's MATERIAL (not the whole resident_body) so they apply
            // uniformly to a resident or a grid tile -- both carry material_props.
            /**
             * @brief Cast acceptor predicate: "is this surface solid for the mover, given it was hit
             *        on @c hit_normal?"
             *
             * @c BLOCK is always solid; @c ONE_WAY is solid only when the contact normal aligns with
             * the surface's @c block_normal (the mover crossed the blocked face); @c SENSOR / @c IGNORE
             * are never solid. Evaluated per candidate AFTER the swept contact is known, so a
             * direction-dependent rule decides on the face actually crossed.
             */
            struct solid_pred {
                bool operator()(const material_props& m, const vec& hit_normal) const {
                    switch (m.response) {
                        case response_mode::BLOCK:
                            return true;
                        case response_mode::ONE_WAY:
                            return euler::dot(hit_normal, m.block_normal) > ONE_WAY_DOT;
                        case response_mode::SENSOR:
                        case response_mode::IGNORE:
                        default:
                            return false;
                    }
                }
            };

            /// @brief Factory for the @ref solid_pred acceptor (solids only: BLOCK / one-way blocked face).
            static solid_pred solid_acceptor() { return solid_pred{}; }

            /// @brief Accept-all acceptor for unrestricted casts (two-arg to match the post-hit shape).
            static bool accept_any(const material_props&, const vec&) { return true; }

            /**
             * @brief The target material behind a handle -- dispatches BODY / BULLET / TILE.
             * @param id Handle to a live collider.
             * @return Its @ref material_props, so move-and-slide's velocity response reads a tile
             *         surface exactly like a resident.
             */
            [[nodiscard]] const material_props& material_of(const collider_id& id) const {
                if (id.type_id == collider_id::BODY) {
                    return m_bodies_storage[id.value].material;
                }
                if (id.type_id == collider_id::BULLET) {
                    return m_bullets_storage[id.value].material;
                }
                return m_static_grid->at(id.value)->material; // TILE
            }

            /**
             * @brief Recover the @c TILE handle for a grid candidate from its cell box.
             *
             * The grid enumerators hand back the cell box (geometry) but hide the cell index
             * (identity); it is recovered from the box centre, which maps back to the same cell.
             *
             * @param cell_box The candidate cell's box.
             * @return A @c TILE handle (@c value = linear cell index, @c generation = the cell's
             *         current generation, valid until that cell is next mutated).
             */
            [[nodiscard]] collider_id tile_handle(const aabb& cell_box) const {
                const uint32_t cell = m_static_grid->to_cell(cell_box.center());
                return {cell, m_static_grid->cell_generation(cell), collider_id::TILE};
            }

            // ---- query plumbing: narrow-phase wrappers + candidate fan-out -----------------------
            // These collapse the six near-identical "fan out over residents + tiles, narrow-phase each
            // candidate, accumulate" copies into one primitive per traversal kind. Each public query
            // is then a thin visitor: the fan-out owns exclusion / tile_handle / ray-clipping (one
            // place to get right), the visitor owns filtering + narrow-phase + accumulation.

            /**
             * @brief Swept narrow-phase: the mover (aabb|circle) swept by @c dv against any target shape.
             *
             * Hides the nested @c std::visit double-dispatch.
             *
             * @param mover  The swept shape (aabb|circle).
             * @param dv     The sweep displacement (the mover's velocity over a unit step).
             * @param target Any target @ref shape_t.
             * @return The earliest @ref swept_hit (@c entry_time is the [0,1] toi; time 1 is the
             *         anti-tunnel window), or @c std::nullopt if they never touch.
             */
            [[nodiscard]] static std::optional <swept_hit> swept_vs_shape(const moving_shape_t& mover,
                                                                          const vec& dv, const shape_t& target) {
                return std::visit([&](const auto& mv) {
                    return std::visit([&](const auto& tgt) -> std::optional <swept_hit> {
                        return swept_intersection(mv, dv, tgt, vec{0, 0}, 1.0f);
                    }, target);
                }, mover);
            }

            /**
             * @brief Ray narrow-phase: the finite segment @c s against any target shape, restricted
             *        to the segment span.
             *
             * Hides the segment-first / aabb-first @c intersect_param dispatch (params run along the
             * ray either way).
             *
             * @param s      The finite query segment.
             * @param target Any target @ref shape_t (incl. segment walls/slopes).
             * @return The crossing's @ref line_hit, or @c std::nullopt if it does not cross within the
             *         segment. The caller clamps @c entry_param >= 0 (origin-inside reads as 0).
             */
            [[nodiscard]] static std::optional <line_hit> ray_vs_shape(const segment& s, const shape_t& target) {
                return std::visit([&]<typename T>(const T& shape) -> std::optional <line_hit> {
                    const std::optional <line_hit> hit = [&] {
                        if constexpr (std::is_same_v <std::decay_t <T>, segment>) {
                            return intersect_param(s, shape); // ray first
                        } else {
                            return intersect_param(shape, s); // aabb/circle first
                        }
                    }();
                    if (hit && hit->segment_overlaps()) {
                        return hit;
                    }
                    return std::nullopt;
                }, target);
            }

            /**
             * @brief Broadphase fan-out: visit every candidate overlapping @c env (BVH residents and,
             *        when @c query_tiles, static-grid tiles), skipping up to two excluded body slots.
             * @tparam Visit Callable @c visit(shape, filter, material, id).
             * @param env          The broadphase query box.
             * @param exclude_idx  A body slot to skip (e.g. the querying body); @c INVALID = none.
             * @param exclude_idx2 A second body slot to skip; @c INVALID = none.
             * @param query_tiles  Whether to also visit static-grid tiles.
             * @param visit        Per-candidate visitor.
             * @note Visits ALL candidates with no clipping -- the visitor/accumulator decides what to
             *       keep, so nearest-hit, all-hits, and sensor variants share this one fan-out.
             */
            template<class Visit>
            void for_each_in_envelope(const aabb& env, uint32_t exclude_idx, uint32_t exclude_idx2,
                                      bool query_tiles, Visit&& visit) const {
                query(m_space_partition, env, [&](entity_id_t other_idx, [[maybe_unused]] const aabb& box) {
                    if (other_idx == exclude_idx || other_idx == exclude_idx2) {
                        return;
                    }
                    const auto& other = m_bodies_storage[other_idx];
                    visit(other.shape, other.filter, other.material,
                          collider_id{other_idx, other.generation, collider_id::BODY});
                });
                if (query_tiles && m_static_grid) {
                    m_static_grid->query(env, [&](const detail::tile& t, const aabb& cb) {
                        visit(t.shape, t.filter, t.material, tile_handle(cb));
                    });
                }
            }

            /**
             * @brief Ray fan-out: visit every candidate the segment @c s crosses (BVH residents and
             *        static-grid tiles), with progressive ray-clipping.
             * @tparam Clip  Callable @c clip() returning the current clip toi.
             * @tparam Visit Callable @c visit(shape, filter, id) that narrow-phases and accumulates.
             * @param s     The finite query segment.
             * @param clip  Returns the current ray-clip toi -- the nearest-hit best-so-far (so the
             *              tree prunes farther boxes and the grid DDA stops once a cell's entry
             *              exceeds it), or a constant @c 1.0 for an all-hits query that never prunes.
             * @param visit Per-candidate visitor.
             */
            template<class Clip, class Visit>
            void for_each_along_ray(const segment& s, Clip&& clip, Visit&& visit) const {
                physics::raycast(m_space_partition, s, // qualified: member `raycast` would otherwise hide it
                                 [&](entity_id_t other_idx, [[maybe_unused]] const aabb& box,
                                     [[maybe_unused]] const line_hit& box_hit) -> float {
                                     const auto& other = m_bodies_storage[other_idx];
                                     visit(other.shape, other.filter,
                                           collider_id{other_idx, other.generation, collider_id::BODY});
                                     return clip();
                                 });
                if (m_static_grid) {
                    m_static_grid->raycast(s.from, s.to,
                                           [&](const detail::tile& t, const aabb& cb, float t_entry) -> bool {
                                               if (t_entry > clip()) {
                                                   return false; // beyond the current best -> stop
                                               }
                                               visit(t.shape, t.filter, tile_handle(cb));
                                               return true;
                                           });
                }
            }

            /**
             * @brief Overlap-query core: report every collider whose shape overlaps @c self_shape
             *        and passes @c self_filter.
             * @tparam Fn Callable @c on_hit(collider_id) invoked once per overlapping candidate.
             * @param self_shape  The querying shape.
             * @param self_filter The querying filter.
             * @param exclude_body A body slot to skip (the querying body); @c INVALID = none.
             * @param query_tiles  Whether to also test static-grid tiles.
             * @param on_hit       Per-overlap callback.
             * @note Source-agnostic: serves both a body sensor (@c query_tiles = true, exclude self)
             *       and a tile sensor (@c query_tiles = false -- tiles do not sense other tiles).
             */
            template<class Fn>
            void overlap_core(const shape_t& self_shape, const filter_props& self_filter,
                              uint32_t exclude_body, bool query_tiles, Fn&& on_hit) const {
                for_each_in_envelope(
                    detail::tight_box(self_shape), exclude_body, collider_id::INVALID, query_tiles,
                    [&](const shape_t& target_shape, const filter_props& target_filter,
                        const material_props&, const collider_id& id) {
                        if (!should_collide(self_filter, target_filter)) {
                            return;
                        }
                        const bool rc = std::visit([&](const auto& my_shape) {
                            return std::visit([&](const auto& other_shape) {
                                return physics::intersects(my_shape, other_shape);
                            }, target_shape);
                        }, self_shape);
                        if (rc) {
                            on_hit(id);
                        }
                    });
            }

            /**
             * @brief A resident body's overlaps -- it senses bodies AND tiles, excluding itself.
             * @tparam Fn Callable @c on_hit(collider_id).
             * @param idx    The querying body's slot index.
             * @param on_hit Per-overlap callback.
             */
            template<class Fn>
            void overlap(uint32_t idx, Fn&& on_hit) const {
                const auto& self = m_bodies_storage[idx];
                overlap_core(self.shape, self.filter, idx, /*query_tiles=*/true, std::forward <Fn>(on_hit));
            }

            /**
             * @brief Swept-query core: sweep @c mover by @c delta and return the earliest accepted
             *        hit across residents AND tiles, or @c std::nullopt.
             * @tparam Accept Callable @c accept(material, hit_normal) -> bool, run per candidate
             *                AFTER its swept contact is computed (so @c ONE_WAY sees the real contact
             *                normal). Per-candidate by necessity: a cast keeps only the earliest hit,
             *                so a post-filter could not recover a farther accepted target behind a
             *                rejected near one.
             * @param mover         The swept shape (aabb|circle) -- covers every target with no guard.
             * @param delta         The sweep displacement.
             * @param self_filter   The mover's collision filter.
             * @param accept        The per-candidate acceptor.
             * @param exclude_idx   A body slot to skip (the mover itself, when in the tree); @c INVALID = none.
             * @param exclude_idx2  A second body slot to skip; defaults to @c INVALID.
             * @return The nearest accepted @ref contact (normalized toi in [0,1]), or @c std::nullopt.
             *         Residents are treated as stationary (per-mover CCD, §8c).
             */
            template<class Accept>
            [[nodiscard]] std::optional <contact> cast_core(const moving_shape_t& mover,
                                                            units::displacement delta,
                                                            filter_props self_filter, Accept&& accept,
                                                            uint32_t exclude_idx,
                                                            uint32_t exclude_idx2 = collider_id::INVALID) const {
                const vec dv = delta.value;
                std::optional <contact> out;
                // Keep the earliest accepted hit across residents AND tiles (so move_and_slide resolves
                // tile floors/walls/slopes for free). accept() sees the real contact normal, so it can
                // reject per-candidate on the actual face (one-way side) -- and the fan-out visits all,
                // so a farther accepted target behind a rejected near one is still found.
                for_each_in_envelope(
                    swept_bound(mover, delta), exclude_idx, exclude_idx2, /*query_tiles=*/true,
                    [&](const shape_t& target_shape, const filter_props& target_filter,
                        const material_props& target_material, const collider_id& id) {
                        if (!should_collide(self_filter, target_filter)) {
                            return;
                        }
                        const auto hit = swept_vs_shape(mover, dv, target_shape);
                        if (!hit || !accept(target_material, hit->entry_normal)) {
                            return;
                        }
                        if (!out || hit->entry_time < out->toi) {
                            out = contact{id, hit->entry_normal, hit->entry_time};
                        }
                    });
                return out;
            }

            /**
             * @brief Internal: cast a @e stored mover (kinematic resident or bullet) by @c delta.
             * @tparam Accept Per-candidate acceptor (see @ref cast_core).
             * @param idx     The mover's slot index.
             * @param type_id @c BODY (excludes itself) or @c BULLET (not in the tree, excludes nothing).
             * @param delta   The sweep displacement.
             * @param accept  The acceptor.
             * @return The nearest accepted @ref contact, or @c std::nullopt.
             */
            template<class Accept>
            [[nodiscard]] std::optional <contact> cast(uint32_t idx, collider_id::type type_id,
                                                       units::displacement delta, Accept&& accept) const {
                if (type_id == collider_id::BODY) {
                    const auto& self = m_bodies_storage[idx];
                    return cast_core(detail::narrow(self.shape), delta, self.filter,
                                     std::forward <Accept>(accept), idx);
                }
                const auto& self = m_bullets_storage[idx];
                return cast_core(self.shape, delta, self.filter,
                                 std::forward <Accept>(accept), collider_id::INVALID);
            }

            /**
             * @brief Internal convenience: cast a stored mover against every filtered candidate (no
             *        acceptor restriction).
             * @param idx     The mover's slot index.
             * @param type_id @c BODY or @c BULLET.
             * @param delta   The sweep displacement.
             * @return The nearest @ref contact, or @c std::nullopt.
             */
            [[nodiscard]] std::optional <contact> cast(uint32_t idx, collider_id::type type_id,
                                                       units::displacement delta) const {
                return cast(idx, type_id, delta, &world::accept_any);
            }

        public:
            // ====================================================================================
            // Public queries & §19 controller helpers: cast / cast_all / raycast / raycast_all /
            // swept_triggers / dedup_by_entity / line_of_sight / snap_to_ground / step_up /
            // ground_support. All const except the snap/step helpers, which move the actor.
            // ====================================================================================
            /**
             * @brief Game-facing aiming cast: sweep a transient @c mover shape by @c delta and return
             *        the earliest hit passing @c filter, or @c std::nullopt.
             *
             * Across BOTH dynamic residents and static tiles (the result's @c collider_id may be
             * @c BODY or @c TILE). The shape is transient (not in the world), so nothing is excluded.
             * The swept counterpart to @ref raycast -- use it for aim previews, lobbed-shot
             * prediction, and "is this move clear?" probes.
             *
             * @param mover  The shape to sweep (aabb|circle).
             * @param delta  The sweep displacement.
             * @param filter The query's collision filter.
             * @return The nearest @ref contact, or @c std::nullopt.
             */
            [[nodiscard]] std::optional <contact> cast(const moving_shape_t& mover, vec delta,
                                                       filter_props filter = {}) const {
                return cast_core(mover, units::displacement{delta}, filter, &world::accept_any,
                                 collider_id::INVALID);
            }

            /**
             * @brief Multi-hit swept shape cast: EVERY collider that @c mover swept by @c delta would
             *        touch, ordered nearest-first by toi -- the swept counterpart to @ref raycast_all.
             *
             * Across residents AND tiles; the transient shape excludes nothing. Reports ALL filtered
             * colliders regardless of material response (a beam passes through, so the caller decides
             * what stops it via @c filter).
             *
             * @param mover    The shape to sweep (aabb|circle).
             * @param delta    The sweep displacement.
             * @param filter   The query's collision filter.
             * @param max_hits Keep only the nearest N hits; @c 0 = all.
             * @return Contacts nearest-first.
             * @note A body with several colliders reports once PER collider -- dedup by eid is the
             *       caller's job via @ref dedup_by_entity.
             */
            [[nodiscard]] std::vector <contact> cast_all(const moving_shape_t& mover, vec delta,
                                                         filter_props filter = {},
                                                         std::size_t max_hits = 0) const {
                const units::displacement d{delta};
                const vec dv = d.value;
                std::vector <contact> hits;
                for_each_in_envelope(
                    swept_bound(mover, d), collider_id::INVALID, collider_id::INVALID, /*query_tiles=*/true,
                    [&](const shape_t& ts, const filter_props& tf,
                        const material_props&, const collider_id& id) {
                        if (!should_collide(filter, tf)) {
                            return;
                        }
                        if (const auto hit = swept_vs_shape(mover, dv, ts)) {
                            hits.push_back(contact{id, hit->entry_normal, hit->entry_time});
                        }
                    });
                return nearest_first(std::move(hits), max_hits);
            }

            /**
             * @brief Swept / crossing triggers: the SENSOR colliders the shape @c mover would touch
             *        while sweeping by @c delta -- INCLUDING ones it passes entirely through between
             *        the endpoints.
             *
             * @ref run 's trigger pass diffs sensor overlaps at frame BOUNDARIES, so a fast body can
             * skip a thin pickup / checkpoint / tripwire between two sampled frames without ever
             * overlapping it at an endpoint; this swept query catches those crossings. Complements the
             * begin/end diff -- use @c TRIGGER_BEGIN / @c TRIGGER_END for staying inside a zone, this
             * for a momentary fast crossing.
             *
             * @param mover  The mover's PRE-move shape (transient, not in the world).
             * @param delta  The frame displacement.
             * @param filter The query's collision filter.
             * @return SENSOR contacts only (solids are the movement pass's job), nearest-first by toi
             *         (a sensor already overlapped at the start reads as toi 0).
             * @note A body with several sensor colliders reports once PER collider -- eid-dedup is the
             *       caller's job via @ref dedup_by_entity.
             */
            [[nodiscard]] std::vector <contact> swept_triggers(const moving_shape_t& mover, vec delta,
                                                               filter_props filter = {}) const {
                const units::displacement dd{delta};
                const vec dv = dd.value;
                std::vector <contact> hits;
                for_each_in_envelope(
                    swept_bound(mover, dd), collider_id::INVALID, collider_id::INVALID, /*query_tiles=*/true,
                    [&](const shape_t& ts, const filter_props& tf,
                        const material_props& tm, const collider_id& id) {
                        if (tm.response != response_mode::SENSOR) {
                            return; // triggers are sensors; solids are resolved by the movement pass
                        }
                        if (!should_collide(filter, tf)) {
                            return;
                        }
                        if (const auto hit = swept_vs_shape(mover, dv, ts)) {
                            hits.push_back(contact{id, hit->entry_normal, hit->entry_time});
                        }
                    });
                return nearest_first(std::move(hits));
            }

            /**
             * @brief Collapse a multi-hit result to one contact PER ENTITY (eid), keeping the nearest
             *        (smallest toi) collider of each.
             *
             * One entity often owns several colliders by role -- hurtbox, hitbox, weak points, shield
             * zones -- all sharing its eid; without this a beam reports the same enemy once per
             * collider. Operates on the output of @ref raycast_all / @ref cast_all /
             * @ref swept_triggers.
             *
             * @param hits A multi-hit result (handles must still be live -- call on fresh query
             *             results, same frame).
             * @return One contact per entity, still ordered nearest-first.
             * @note Keys purely on eid, so colliders sharing an eid (INCLUDING the default 0 on
             *       un-keyed colliders) merge -- assign eids per logical entity.
             */
            [[nodiscard]] std::vector <contact> dedup_by_entity(const std::vector <contact>& hits) const {
                std::vector <contact> out;
                out.reserve(hits.size());
                std::vector <entity_id_t> kept; // eid parallel to `out`, so the nearest per entity wins
                for (const auto& h : hits) {
                    const entity_id_t e = get_eid(h.who);
                    bool merged = false;
                    for (std::size_t k = 0; k < kept.size(); ++k) {
                        if (kept[k] == e) {
                            merged = true;
                            if (h.toi < out[k].toi) {
                                out[k] = h; // a nearer collider of the same entity supersedes
                            }
                            break;
                        }
                    }
                    if (!merged) {
                        out.push_back(h);
                        kept.push_back(e);
                    }
                }
                return nearest_first(std::move(out));
            }

            /**
             * @brief First collider the finite segment @c s crosses (nearest along the ray), or
             *        @c std::nullopt.
             *
             * Across BOTH dynamic residents and static tiles (the result's @c collider_id may be
             * @c BODY or @c TILE). Bullets are excluded (not in the tree). Targets may be any shape
             * incl. segments (walls/slopes), visited as the full @ref shape_t -- never narrowed.
             *
             * @param s      The finite query segment.
             * @param filter The query's collision filter.
             * @return The nearest crossing's @ref contact (toi clamped to >= 0; an origin already
             *         inside a shape reads as 0), or @c std::nullopt.
             */
            [[nodiscard]] std::optional <contact> raycast(const segment& s, filter_props filter = {}) const {
                std::optional <contact> out;
                // Keep the nearest crossing; clip the ray to the best hit so far so the tree prunes
                // farther boxes and the grid DDA stops once a cell's entry exceeds it.
                for_each_along_ray(
                    s, [&] { return out ? out->toi : 1.0f; },
                    [&](const shape_t& ts, const filter_props& tf, const collider_id& id) {
                        if (!should_collide(filter, tf)) {
                            return;
                        }
                        if (const auto hit = ray_vs_shape(s, ts)) {
                            const float toi = std::max(0.0f, hit->entry_param); // origin-inside -> 0
                            if (!out || toi < out->toi) {
                                out = contact{id, hit->entry_normal, toi};
                            }
                        }
                    });
                return out;
            }

            /**
             * @brief Multi-hit ray: EVERY collider the finite segment @c s crosses, ordered
             *        nearest-first by toi -- not just the nearest like @ref raycast.
             *
             * Across residents AND tiles; bullets excluded (not in the tree). Covers beams, piercing
             * shots, melee/sword arcs, explosion sweeps, and boss multi-hurtbox scans.
             *
             * @param s        The finite query segment.
             * @param filter   The query's collision filter.
             * @param max_hits Keep only the nearest N crossings; @c 0 = all.
             * @return Contacts nearest-first.
             * @note A body with several colliders reports once PER collider -- dedup by eid is the
             *       caller's job via @ref dedup_by_entity.
             */
            [[nodiscard]] std::vector <contact> raycast_all(const segment& s, filter_props filter = {},
                                                            std::size_t max_hits = 0) const {
                std::vector <contact> hits;
                // Constant clip 1.0 -> never prune: collect every crossing (toi in [0,1]).
                for_each_along_ray(
                    s, [] { return 1.0f; },
                    [&](const shape_t& ts, const filter_props& tf, const collider_id& id) {
                        if (!should_collide(filter, tf)) {
                            return;
                        }
                        if (const auto hit = ray_vs_shape(s, ts)) {
                            hits.push_back(contact{id, hit->entry_normal, std::max(0.0f, hit->entry_param)});
                        }
                    });
                return nearest_first(std::move(hits), max_hits);
            }

            /**
             * @brief Is @c to visible from @c from -- i.e. nothing in @c blockers lies strictly between?
             *
             * A convenience over @ref raycast.
             *
             * @param from     Ray origin.
             * @param to       Ray endpoint (the target being checked for visibility).
             * @param blockers Filter selecting the colliders that occlude.
             * @return @c true when there is no hit, or the first blocker is at or beyond @c to.
             */
            [[nodiscard]] bool line_of_sight(vec from, vec to, filter_props blockers = {}) const {
                const auto hit = raycast(segment{from, to}, blockers);
                return !hit || hit->toi >= 1.0f;
            }

            /**
             * @brief Ground snapping (§19 #2) -- keep a grounded actor glued to a floor that receded
             *        beneath it this frame, so it hugs a downhill slope / staircase instead of
             *        launching off each lip.
             *
             * Sweeps the actor's OWN shape straight down (-up) by at most @c max_drop; if it lands on
             * a WALKABLE surface within reach (the same solids it slides on -- @c ONE_WAY floors
             * included, from above), it translates the actor down onto that surface, leaving the
             * standard skin gap so the next frame's cast does not re-hit at toi 0.
             *
             * @param cid      A live kinematic @c BODY actor (aborts otherwise).
             * @param max_drop Max reach below the feet (>= 0). Size it to the tallest one-frame step
             *                 down (a few px + slope*speed*dt), small enough that a true cliff (drop >
             *                 @c max_drop) still lets the actor fall.
             * @return The ground @ref contact it snapped to (normal usable for slope-aligned
             *         velocity), or @c std::nullopt when no walkable ground is within @c max_drop (a
             *         real ledge/cliff or a >45 deg face -- the actor should fall). A flush floor
             *         (toi 0) returns the contact with no move.
             * @note POLICY is the caller's: call AFTER @ref run, only when the actor was grounded last
             *       frame and is not rising. Mutates the actor (moves it down).
             */
            [[nodiscard]] std::optional <contact> snap_to_ground(collider_id cid, float max_drop) {
                ENFORCE(is_valid(cid) && cid.type_id == collider_id::BODY);
                auto& self = m_bodies_storage[cid.value];
                ENFORCE(self.kind == detail::body_kind::KINEMATIC)("snap_to_ground: actor must be kinematic");
                ENFORCE(max_drop >= 0.0f)("snap_to_ground: max_drop must be non-negative");

                // Sweep the actor's shape straight down; cast(idx, BODY, ...) excludes self and uses
                // the actor's own filter, so we snap onto exactly the solids it would slide on.
                const units::displacement probe{-m_cfg.up * max_drop};
                const auto hit = cast(cid.value, collider_id::BODY, probe, solid_acceptor());
                if (!hit) {
                    return std::nullopt; // nothing below within reach -> fall
                }
                // Only a WALKABLE surface counts, keeping snapped <=> grounded coherent: a >45 deg
                // face below reads as a cliff/wall, not a floor.
                if (!is_walkable(hit->normal)) {
                    return std::nullopt;
                }
                // Land `skin` short of the surface (the same anti-jitter cushion move_and_slide keeps).
                const float dist = std::max(0.0f, hit->toi * max_drop - m_cfg.skin);
                translate(self, -m_cfg.up * dist);
                refit_proxy(self);
                return hit;
            }

            /**
             * @brief Step-up / ledge forgiveness (§19 #3) -- a mover walking into a small lip (a low
             *        step riser, a curb, a tile edge) rides up over it rather than jamming against it.
             *
             * Lifts the actor up to @c max_step (capped by headroom), re-casts the horizontal @c step
             * from the raised position, and -- if the path is now clear AND a WALKABLE tread waits
             * below -- carries it forward over the lip and settles it down onto that tread. A riser
             * taller than @c max_step, no headroom to lift, a ledge with no tread, or a non-walkable
             * (steep/side) tread leaves the actor untouched.
             *
             * @param cid      A live kinematic @c BODY actor (aborts otherwise).
             * @param step     The horizontal displacement to apply FROM WHERE THE ACTOR IS NOW (its
             *                 component along @c up is ignored).
             * @param max_step Max lift height (>= 0).
             * @return The tread @ref contact it settled on (normal usable for slope-aligned velocity),
             *         or @c std::nullopt when nothing was stepped over.
             *
             * @warning CONTRACT: this performs the WHOLE stepped move -- it IS the move, not a nudge
             *          layered on one already taken. The caller must not also apply @c step elsewhere:
             *          as an alternative to a slide, call from the pre-move position with the full
             *          frame delta and skip the ordinary slide on success; after a slide that hit a
             *          lip, pass the REMAINING delta (intended - already-moved), or the actor advances
             *          twice.
             * @note Call on a body resting at the skin-short gap the solver leaves (the post-@ref run
             *       state). A body flush against its support reads every cast as a toi-0 contact, so a
             *       flush actor must be nudged off-surface first. Mutates the actor. POLICY is the
             *       caller's: enable it for characters that climb steps, not crates/projectiles.
             */
            [[nodiscard]] std::optional <contact> step_up(collider_id cid, vec step, float max_step) {
                ENFORCE(is_valid(cid) && cid.type_id == collider_id::BODY);
                auto& self = m_bodies_storage[cid.value];
                ENFORCE(self.kind == detail::body_kind::KINEMATIC)("step_up: actor must be kinematic");
                ENFORCE(max_step >= 0.0f)("step_up: max_step must be non-negative");

                // Only the horizontal part carries the actor over a lip (vertical intent is gravity/jump).
                const vec horiz = step - euler::dot(step, m_cfg.up) * m_cfg.up;
                if (near_zero(horiz)) {
                    return std::nullopt;
                }
                const units::displacement fwd{horiz};

                // Nothing in the way: not a step-up situation -- leave the move to the normal solver.
                if (!cast(cid.value, collider_id::BODY, fwd, solid_acceptor())) {
                    return std::nullopt;
                }

                // Probe straight up for headroom, then lift by as much of max_step as actually clears.
                const auto head = cast(cid.value, collider_id::BODY,
                                       units::displacement{m_cfg.up * max_step}, solid_acceptor());
                const float lift = head ? std::max(0.0f, head->toi * max_step - m_cfg.skin) : max_step;
                if (lift <= constants::POINT_EPS) {
                    return std::nullopt; // pinned below a ceiling -- no room to step up
                }
                translate(self, m_cfg.up * lift);

                // From the raised position, must the horizontal step now clear the lip entirely; a
                // remaining hit means the obstruction is taller than we lifted -> a true wall.
                if (cast(cid.value, collider_id::BODY, fwd, solid_acceptor())) {
                    translate(self, -m_cfg.up * lift); // undo the lift
                    refit_proxy(self);
                    return std::nullopt;
                }
                translate(self, horiz); // carry forward over the lip

                // Settle down onto the tread. We climbed `lift`, so we fall at most that far (never
                // past the original floor). The tread must be WALKABLE (normal faces up past the
                // ground threshold, exactly as snap_to_ground requires) -- a clear forward path does
                // NOT prove a standable surface waits below it. A missing tread (a ledge) OR a steep
                // /side/vertical face means this was not a clean step: undo the whole move so the
                // actor is never left stranded mid-air or "standing" on a wall.
                const auto tread = cast(cid.value, collider_id::BODY,
                                        units::displacement{-m_cfg.up * lift}, solid_acceptor());
                if (!tread || !is_walkable(tread->normal)) {
                    translate(self, -horiz);
                    translate(self, -m_cfg.up * lift);
                    refit_proxy(self);
                    return std::nullopt;
                }
                const float drop = std::max(0.0f, tread->toi * lift - m_cfg.skin);
                translate(self, -m_cfg.up * drop);
                refit_proxy(self);
                return tread;
            }

            /**
             * @brief Footing / edge sensors (§19 #4) -- report the solid ground under three points of
             *        the actor's footprint (left edge, centre, right edge) by probing straight down.
             *
             * A STATE the game reads to drive teeter/balance, edge-stop, coyote-time and ledge-grab;
             * the library's job is the query, not the response (a teetering character does not fall --
             * that is animation, not dynamics). The left/right pair is exactly Sonic's twin floor
             * sensors (A/B). Each probe is a self-excluding point cast gated to WALKABLE SOLID ground,
             * so footing agrees with move-and-slide / @ref snap_to_ground / @ref step_up on what
             * "ground" is (a @c ONE_WAY platform counts only from above).
             *
             * @param cid      A live @c BODY actor (aborts if invalid).
             * @param max_drop Reach below the feet that still counts as support (>= 0) -- small for a
             *                 touching "am I at the edge?" check, larger to also catch a step within stride.
             * @return A @ref footing with the per-foot contacts plus its
             *         @c grounded()/fully_supported()/at_ledge()/ledge_left()/ledge_right() helpers.
             * @note Pure query (const) -- never moves the actor.
             */
            [[nodiscard]] footing ground_support(collider_id cid, float max_drop) const {
                ENFORCE(is_valid(cid) && cid.type_id == collider_id::BODY);
                ENFORCE(max_drop >= 0.0f)("ground_support: max_drop must be non-negative");
                const auto& self = m_bodies_storage[cid.value];

                // Footprint = the actor's enclosing box; "down" is -up, "tangent" the perpendicular
                // (={1,0} for the default up={0,1}). The bottom-face centre and half-width follow from
                // the box's support along each axis, so this is correct for any axis-aligned up.
                const aabb fp = detail::tight_box(self.shape);
                const vec up = m_cfg.up;
                const vec down = -up;
                const vec tangent{up.y(), -up.x()};
                const vec half = (fp.max - fp.min) * 0.5f;
                const vec mid = (fp.min + fp.max) * 0.5f;
                const float half_w = std::abs(tangent.x()) * half.x() + std::abs(tangent.y()) * half.y();
                const float half_d = std::abs(down.x()) * half.x() + std::abs(down.y()) * half.y();
                const vec foot = mid + down * half_d; // bottom-face centre

                const units::displacement reach{down * max_drop};
                // "Support" means WALKABLE ground, not merely a solid: a steep slope / side face /
                // vertical segment underfoot is not something you stand on, and the rest of the system
                // (move_and_slide's grounded, snap_to_ground, step_up) only counts normal.up >
                // GROUND_THRESHOLD as ground. Gate the probe the same way so footing agrees. A near
                // steep face does not mask a walkable surface farther down: cast_core skips rejected
                // candidates, so a flat floor below a steep lip (within max_drop) is still found.
                auto walkable_solid = [this](const material_props& m, const vec& n) {
                    return solid_pred{}(m, n) && is_walkable(n);
                };
                auto probe = [&](const vec& p) -> std::optional <contact> {
                    // a zero-size aabb point swept down: self-excluded + walkable-solid gated, like a
                    // raycast that respects material AND slope (a plain raycast would report sensors,
                    // steep walls, and the actor itself).
                    return cast_core(moving_shape_t{aabb{p, p}}, reach, self.filter,
                                     walkable_solid, cid.value);
                };

                footing f;
                f.left = probe(foot - tangent * half_w);
                f.centre = probe(foot);
                f.right = probe(foot + tangent * half_w);
                return f;
            }

        private:
            // ====================================================================================
            // Internals, part 2: the slide solver (move_and_slide), carriers (MP1/2/3 helpers),
            // trigger bookkeeping, grid boundary-bake, shared constants, and the member state.
            // ====================================================================================
            /// @brief Outcome of @ref move_and_slide: post-slide velocity, grounded flag, and the
            ///        surfaces hit this move (for @ref run to emit as @c COLLISION events).
            struct slide_result {
                vec velocity; // post-slide velocity (also written back to the body)
                bool grounded = false;
                std::array <contact, 4> contacts{}; // surfaces hit this move (for run() -> events)
                int count = 0;
            };

            /**
             * @brief Move kinematic body @c idx by velocity*dt this frame, resolving against the
             *        residents the @c acceptor deems solid.
             *
             * Sweeps, stops a @c skin short of each contact, slides the leftover along the surface,
             * and damps the velocity via the surface material -- up to @c max_slide_iter passes (a
             * floor+wall corner needs 2). The body's stored shape/velocity and its broadphase proxy
             * are updated in place.
             *
             * @tparam Fn Acceptor @c acceptor(material, hit_normal) -> bool selecting solid surfaces
             *            given each candidate's actual contact normal (so @c ONE_WAY decides on the
             *            face crossed). Sensors/ignored bodies return false so they never block
             *            movement (@ref run reports them via its trigger pass).
             * @param idx      The kinematic body's slot index.
             * @param dt       Frame duration.
             * @param acceptor The solid-surface acceptor.
             * @return A @ref slide_result with the post-slide velocity, grounded flag, and contacts hit.
             */
            template<typename Fn>
            slide_result move_and_slide(uint32_t idx, units::duration dt, Fn&& acceptor) {
                auto& self = m_bodies_storage[idx];
                ENFORCE(self.kind == detail::body_kind::KINEMATIC);

                slide_result res;
                res.velocity = self.velocity;
                units::displacement remaining = units::velocity{self.velocity} * dt; // v * dt
                for (int iter = 0; iter < m_cfg.max_slide_iter; ++iter) {
                    if (near_zero(remaining.value)) {
                        break;
                    }
                    auto hit = cast(idx, collider_id::BODY, remaining, acceptor);
                    if (!hit) {
                        translate(self, remaining.value); // clear path: take the whole step
                        break;
                    }
                    // Advance to just short of the surface (the skin keeps a permanent gap so the
                    // next iteration's cast does not re-hit at toi 0).
                    const float len = euler::length(remaining.value);
                    const float skin_frac = (len > constants::POINT_EPS) ? (m_cfg.skin / len) : 0.0f;
                    const float advance = std::max(0.0f, hit->toi - skin_frac);
                    translate(self, (remaining * units::fraction{advance}).value);

                    const vec n = hit->normal;
                    if (near_zero(n)) {
                        break; // undefined normal -> can't slide; bail
                    }

                    // Slide: the leftover budget is (1 - toi) of the step (NOT 1 - advance --
                    // the skin is a physical cushion, not part of the motion budget); remove its
                    // into-surface component so the rest glides along the surface.
                    const units::displacement leftover = remaining * units::fraction{1.0f - hit->toi};
                    remaining = units::displacement{leftover.value - euler::dot(leftover.value, n) * n};

                    // Velocity response -- material-driven (friction/restitution from the SURFACE),
                    // applied to velocity only; position sliding above is pure geometry. material_of
                    // dispatches on the contact's handle, so a tile surface works like a resident.
                    res.velocity = detail::eval_velocity_response(units::velocity{res.velocity}, n,
                                                                  material_of(hit->who)).value;

                    // Grounded if the contact faces up enough to stand on (~45 deg max slope).
                    if (is_walkable(n)) {
                        res.grounded = true;
                    }
                    if (res.count < static_cast <int>(res.contacts.size())) {
                        res.contacts[res.count++] = *hit;
                    }
                }

                // Persist: write back the projected velocity, and re-fit the proxy ONLY when the
                // moved tight box has escaped the stored fat box. Passing a freshly fattened box
                // every frame would defeat update_leaf's containment short-circuit (the margin
                // shifts with the body, so it never stays contained) and re-graft every frame.
                self.velocity = res.velocity;
                const aabb tight = detail::tight_box(self.shape);
                if (!detail::contains(m_space_partition[self.proxy].box, tight)) {
                    update_leaf(m_space_partition, self.proxy, fatten(self)); // escaped -> fresh fat box
                }
                return res;
            }

            /**
             * @brief A persistent sensor-overlap pair, the unit the trigger diff works on.
             *
             * @c key is the diff identity: the two handles in canonical order, INCLUDING generation --
             * so a slot reused by a different body (new generation) is a distinct pair and the old
             * end / new begin both fire correctly, rather than the diff silently treating them as
             * "still overlapping". The @c collider_ids are kept so an END can name the pair even after
             * a body is removed.
             */
            struct sensor_pair {
                // {lo.value, lo.gen, lo.type, hi.value, hi.gen, hi.type}; std::array < is lexicographic.
                // type_id is part of the key: a BODY and a TILE can share value+generation (a cell
                // index can equal a body slot), so omitting it would alias their pairs.
                std::array <uint32_t, 6> key{};
                collider_id sensor{};
                collider_id other{};
            };

            /**
             * @brief Canonical, full-identity pair key for the trigger diff.
             * @param a,b The two handles, ordered by the whole @ref collider_id (value, generation,
             *            type_id -- via the defaulted @c operator<=>).
             * @return @c {lo.value, lo.gen, lo.type, hi.value, hi.gen, hi.type}, lexicographically
             *         comparable.
             */
            static std::array <uint32_t, 6> pair_key(const collider_id& a, const collider_id& b) {
                const collider_id& lo = (a < b) ? a : b;
                const collider_id& hi = (a < b) ? b : a;
                return {
                    lo.value, lo.generation, static_cast <uint32_t>(lo.type_id),
                    hi.value, hi.generation, static_cast <uint32_t>(hi.type_id)
                };
            }

            /**
             * @brief Emit a @c TRIGGER_BEGIN / @c TRIGGER_END event for a sensor pair.
             * @param kind @c TRIGGER_BEGIN or @c TRIGGER_END.
             * @param p    The sensor pair (its stored ids name the event).
             */
            void emit_trigger(event_kind kind, const sensor_pair& p) {
                m_events.emplace_back(kind, p.sensor, p.other, vec{}, -1.0f);
            }

            /// @brief A contact is "ground" when @c dot(n, up) exceeds this -- @c cos(45 deg) ~ 0.707
            ///        (max walkable slope ~45 degrees).
            static constexpr float GROUND_THRESHOLD = 0.707f;

            /**
             * @brief THE single definition of "is this surface standable ground?"
             * @param contact_normal The contact's outward normal.
             * @return @c true iff it faces up past @ref GROUND_THRESHOLD.
             * @note Shared by move-and-slide's grounded flag, @ref snap_to_ground, @ref step_up 's
             *       tread, and @ref ground_support so they cannot drift apart.
             */
            [[nodiscard]] bool is_walkable(const vec& contact_normal) const {
                return euler::dot(contact_normal, m_cfg.up) > GROUND_THRESHOLD;
            }

            /**
             * @brief Finalize a multi-hit result: order nearest-first by toi, then optionally cap to N.
             * @param hits     The unsorted hits.
             * @param max_hits Keep only the nearest N; @c 0 = all.
             * @return The hits sorted nearest-first (and truncated). Shared by @ref raycast_all /
             *         @ref cast_all / @ref swept_triggers / @ref dedup_by_entity.
             */
            [[nodiscard]] static std::vector <contact> nearest_first(std::vector <contact> hits,
                                                                     std::size_t max_hits = 0) {
                std::sort(hits.begin(), hits.end(),
                          [](const contact& a, const contact& b) { return a.toi < b.toi; });
                if (max_hits > 0 && hits.size() > max_hits) {
                    hits.resize(max_hits);
                }
                return hits;
            }

            /// @brief A @c ONE_WAY surface blocks only when @c dot(contact_normal, block_normal)
            ///        exceeds this -- @c 0.5 (~cos 60 deg) blocks a head-on crossing while letting
            ///        side/grazing contacts pass.
            static constexpr float ONE_WAY_DOT = 0.5f;

            /// @brief Is a vector within @c POINT_EPS of zero length? (squared-length test, no sqrt).
            static constexpr bool near_zero(const vec& v) {
                return euler::length_squared(v) < constants::POINT_EPS * constants::POINT_EPS;
            }

            /// @brief Translate a resident body's shape in place by @c v.
            static void translate(detail::resident_body& body, const vec& v) {
                shape_t new_shape{aabb{}};
                std::visit([&v, &new_shape](const auto& s) {
                    new_shape = physics::translate(s, v);
                }, body.shape);
                body.shape = new_shape;
            }

            /// @brief Translate a bullet (nonresident) body's shape in place by @c v.
            static void translate(detail::nonresident_body& body, const vec& v) {
                moving_shape_t new_shape{aabb{}};
                std::visit([&v, &new_shape](const auto& s) {
                    new_shape = physics::translate(s, v);
                }, body.shape);
                body.shape = new_shape;
            }

            /**
             * @brief Boundary-compile the static grid (§19 #4): merge adjacent opted-in solid tiles
             *        into bigger AABB residents so a run of tiles has no internal seams to snag fast
             *        movers.
             *
             * Drives the grid's generic @c compile_runs with the world's "mergeable group" rule (a
             * cell-filling solid @c BLOCK aabb sharing material + filter with its neighbour).
             *
             * @note ONE-SHOT: runs once, lazily, on the first @ref run -- add all mergeable static
             *       tiles before that. The compile is destructive (it clears the source cells and
             *       installs untracked residents), so @ref add rejects a mergeable tile afterwards and
             *       @ref clear resets the bake for a level reload. Non-mergeable tiles stay editable.
             */
            void compile_static_grid() {
                if (!m_static_grid || m_compiled) {
                    return;
                }
                m_compiled = true;

                // A cell is mergeable iff it opted in and is a solid BLOCK aabb that fills its cell;
                // two such cells share a group iff they have the same material + filter.
                const auto same_group = [](const detail::tile& seed, const detail::tile& cell, const aabb& cb) {
                    if (!cell.mergeable || cell.material.response != response_mode::BLOCK
                        || !std::holds_alternative <aabb>(cell.shape)) {
                        return false;
                    }
                    const aabb cs = std::get <aabb>(cell.shape);
                    const float e = constants::POINT_EPS;
                    const bool fills = std::abs(cs.min.x() - cb.min.x()) < e && std::abs(cs.min.y() - cb.min.y()) < e
                                       && std::abs(cs.max.x() - cb.max.x()) < e && std::abs(cs.max.y() - cb.max.y()) <
                                       e;
                    if (!fills) {
                        return false;
                    }
                    const material_props& a = seed.material;
                    const material_props& b = cell.material;
                    const bool same_mat = a.restitution == b.restitution && a.friction == b.friction
                                          && a.response == b.response
                                          && a.block_normal.x() == b.block_normal.x()
                                          && a.block_normal.y() == b.block_normal.y();
                    const bool same_filter = seed.filter.category == cell.filter.category
                                             && seed.filter.mask == cell.filter.mask;
                    return same_mat && same_filter;
                };
                m_static_grid->compile_runs(same_group, [this](const aabb& region, const detail::tile& sample) {
                    const uint32_t i = m_bodies_storage.allocate();
                    auto& stored = m_bodies_storage[i];
                    stored.shape = shape_t{region};
                    stored.kind = detail::body_kind::STATIC;
                    stored.material = sample.material;
                    stored.filter = sample.filter;
                    stored.eid = sample.eid;
                    stored.velocity = vec{0, 0};
                    stored.proxy = insert_leaf(m_space_partition, i, fatten(stored));
                });
            }

            // ---- carriers (moving platforms / conveyors / crushers) -----------------------------

            /**
             * @brief Does actor @c actor_idx ride carrier @c carrier_idx?
             *
             * After the filter (layers interact) and material (carrier solid from above) gates, three
             * shape-aware gates that hold for aabb/circle riders on ANY @c up:
             *   1. VERTICAL: the actor's underside rests at/just above the carrier's top, within the
             *      skin gap (excludes a body wedged against a side).
             *   2. PERPENDICULAR: the actor and carrier STRICTLY overlap across @c up (excludes a mere
             *      top edge/corner touch).
             *   3. CONTACT (shape-aware): nudging the actor down by the gap makes its REAL shape
             *      intersect the carrier (excludes a circle merely near a corner).
             *
             * @param actor_idx   Candidate rider's slot index.
             * @param carrier_idx Carrier's slot index.
             * @return @c true iff the actor is supported on top of the carrier.
             */
            [[nodiscard]] bool is_riding(uint32_t actor_idx, uint32_t carrier_idx) const {
                const auto& a = m_bodies_storage[actor_idx];
                const auto& c = m_bodies_storage[carrier_idx];
                if (!should_collide(a.filter, c.filter)) {
                    return false; // layers don't interact -> not carried
                }
                if (!solid_pred{}(c.material, m_cfg.up)) {
                    return false; // carrier isn't solid from above (SENSOR/IGNORE, or one-way wrong side)
                }

                const vec u = m_cfg.up;
                const vec perp{u.y(), -u.x()};
                const float eps = constants::POINT_EPS;
                const float gap = m_cfg.skin * 2.0f + eps; // touching .. within the move_and_slide skin gap

                // [lo, hi] extent of a shape projected onto `axis` -- exact per shape (a circle is
                // center·axis ± r; others use their enclosing-box corners).
                struct span {
                    float lo, hi;
                };
                const auto extent = [](const shape_t& s, const vec& axis) -> span {
                    return std::visit([&](const auto& sh) -> span {
                        using S = std::decay_t <decltype(sh)>;
                        if constexpr (std::is_same_v <S, circle>) {
                            const float c0 = sh.center.x() * axis.x() + sh.center.y() * axis.y();
                            return span{c0 - sh.radius, c0 + sh.radius};
                        } else {
                            const aabb b = enclose(sh);
                            const float p0 = b.min.x() * axis.x() + b.min.y() * axis.y();
                            const float p1 = b.max.x() * axis.x() + b.min.y() * axis.y();
                            const float p2 = b.min.x() * axis.x() + b.max.y() * axis.y();
                            const float p3 = b.max.x() * axis.x() + b.max.y() * axis.y();
                            return span{std::min({p0, p1, p2, p3}), std::max({p0, p1, p2, p3})};
                        }
                    }, s);
                };

                // (1) vertical: actor underside at/within-gap of carrier top.
                const span au = extent(a.shape, u), cu = extent(c.shape, u);
                if (!(au.lo >= cu.hi - eps && au.lo <= cu.hi + gap)) {
                    return false;
                }
                // (2) perpendicular: strict overlap across up (not a mere edge/corner touch).
                const span ap = extent(a.shape, perp), cp = extent(c.shape, perp);
                if (!(ap.hi > cp.lo + eps && ap.lo < cp.hi - eps)) {
                    return false;
                }
                // (3) shape-aware contact: the actor, nudged down by the gap, really touches the carrier.
                const vec down{-u.x() * gap, -u.y() * gap};
                const shape_t nudged = std::visit([&](const auto& s) {
                    return shape_t{physics::translate(s, down)};
                }, a.shape);
                return std::visit([&](const auto& as) {
                    return std::visit([&](const auto& cs) { return physics::intersects(as, cs); }, c.shape);
                }, nudged);
            }

            /**
             * @brief Re-fit a resident's broadphase proxy after it moved -- only when its tight box
             *        escaped the stored fat box (same containment short-circuit as @ref move_and_slide).
             * @param b The moved resident.
             */
            void refit_proxy(detail::resident_body& b) {
                const aabb tight = detail::tight_box(b.shape);
                if (!detail::contains(m_space_partition[b.proxy].box, tight)) {
                    update_leaf(m_space_partition, b.proxy, fatten(b));
                }
            }

            /**
             * @brief Move a carrier rigidly by @c d (it is never blocked -- it pushes/carries, MP2/3).
             * @param idx The carrier's slot index.
             * @param d   The rigid displacement.
             */
            void move_carrier_rigid(uint32_t idx, const vec& d) {
                auto& c = m_bodies_storage[idx];
                translate(c, d);
                refit_proxy(c);
            }

            /**
             * @brief Carry rider @c j (locked to carrier @c carrier_idx) by @c d -- a collision-aware
             *        move that hits every solid EXCEPT its carrier.
             *
             * The rider rides freely but still stops a skin short of walls / ceilings / other
             * carriers. (MP1: a blocked rider simply stops; @ref emit_crush_if_pinned turns "still
             * pinned" into a @c CRUSH event.)
             *
             * @param j           The rider's slot index.
             * @param carrier_idx The carrier to exclude from the move's collisions.
             * @param d           The intended displacement.
             */
            void carry_translate(uint32_t j, uint32_t carrier_idx, const vec& d) {
                if (near_zero(d)) {
                    return;
                }
                auto& a = m_bodies_storage[j];
                const auto hit = cast_core(detail::narrow(a.shape), units::displacement{d}, a.filter,
                                           solid_acceptor(), j, carrier_idx);
                float frac = 1.0f;
                if (hit) {
                    const float len = euler::length(d);
                    const float skin_frac = (len > constants::POINT_EPS) ? (m_cfg.skin / len) : 0.0f;
                    frac = std::max(0.0f, hit->toi - skin_frac);
                }
                if (frac <= 0.0f) {
                    return;
                }
                translate(a, vec{d.x() * frac, d.y() * frac});
                refit_proxy(a);
            }

            /**
             * @brief MP3 crush: after a carry/push, emit a @c CRUSH event if the actor still overlaps
             *        the carrier (it could not move clear -- pinned against other solid geometry).
             *
             * Uses STRICT penetration, so a clean carry/push (which leaves a skin gap, or at best a
             * flush touch) does not read as a crush -- only a genuine pin fires.
             *
             * @param actor_idx   The carried/pushed actor's slot index.
             * @param carrier_idx The carrier's slot index.
             * @param crush_dir   The (un-normalized) direction the actor was being moved; reported as
             *                    the event normal.
             */
            void emit_crush_if_pinned(uint32_t actor_idx, uint32_t carrier_idx, const vec& crush_dir) {
                // STRICT penetration (overlap -> a positive depth), not inclusive intersects: a clean
                // carry/push leaves the actor at best TOUCHING the carrier (a zero-gap rider rides flush
                // on the top), which must NOT read as a crush. Actors and carriers are always movers
                // (aabb|circle), so narrow both -> overlap()'s aabb/circle overloads cover every combo.
                const moving_shape_t a = detail::narrow(m_bodies_storage[actor_idx].shape);
                const moving_shape_t c = detail::narrow(m_bodies_storage[carrier_idx].shape);
                const bool pinned = std::visit([&](const auto& as) {
                    return std::visit([&](const auto& cs) { return physics::overlap(as, cs).has_value(); }, c);
                }, a);
                if (!pinned) {
                    return;
                }
                const float len = euler::length(crush_dir);
                const vec n = len > constants::POINT_EPS
                                  ? vec{crush_dir.x() / len, crush_dir.y() / len}
                                  : vec{0, 0};
                m_events.emplace_back(
                    event_kind::CRUSH,
                    collider_id{actor_idx, m_bodies_storage.generation(actor_idx), collider_id::BODY},
                    collider_id{carrier_idx, m_bodies_storage.generation(carrier_idx), collider_id::BODY},
                    n, 0.0f);
            }

            /**
             * @brief MP2 push displacement: the move that shoves actor box @c a clear of carrier box
             *        @c c ALONG the carrier's motion @c d, leaving a skin gap.
             * @param a The actor's tight box.
             * @param c The carrier's tight box.
             * @param d The carrier's motion (only its non-zero axes are pushed on).
             * @return The clearing displacement (box-level; the actual move is shape-aware).
             */
            [[nodiscard]] vec clear_push(const aabb& a, const aabb& c, const vec& d) const {
                const float eps = constants::POINT_EPS;
                const float s = m_cfg.skin;
                const float px = d.x() > eps
                                     ? (c.max.x() - a.min.x() + s)
                                     : d.x() < -eps
                                           ? (c.min.x() - a.max.x() - s)
                                           : 0.0f;
                const float py = d.y() > eps
                                     ? (c.max.y() - a.min.y() + s)
                                     : d.y() < -eps
                                           ? (c.min.y() - a.max.y() - s)
                                           : 0.0f;
                return vec{px, py};
            }

            // ---- member state -------------------------------------------------------------------
            world_config m_cfg; ///< Construction-time configuration.
            detail::bodies_storage m_bodies_storage; ///< Slot-map of resident bodies (static/kinematic/carrier).
            detail::bullets_storage m_bullets_storage; ///< Slot-map of bullets (not in the tree).

            dynamic_aabb_tree m_space_partition; ///< Broadphase: the dynamic AABB tree over residents.
            std::optional <grid <detail::tile>> m_static_grid; ///< Static tiles; unset = no grid.
            bool m_compiled = false; ///< The one-shot tile boundary-bake has run (reset by clear()).

            std::vector <world_event> m_events; ///< Reused per-frame event buffer (returned by run()).
            std::vector <uint32_t> m_rider_scratch; ///< Reused per-carrier rider list (carrier pass).
            std::vector <uint32_t> m_push_scratch; ///< Reused per-carrier pushed-actor list (MP2).
            std::vector <sensor_pair> m_triggers_curr; ///< This frame's sensor overlaps.
            std::vector <sensor_pair> m_triggers_prev; ///< Last frame's overlaps (for the begin/end diff).
            std::vector <collider_id> m_sensor_tiles; ///< SENSOR tile handles (lazily pruned).
    };

    // ===========================================================================================================
    // Implementation
    // ===========================================================================================================
    inline world::world(const world_config& cfg)
        : m_cfg(cfg) {
        if (m_cfg.grid) {
            // The grid shares the world's extent (one coordinate frame). Require bounds and
            // an exact tiling -- a non-dividing extent is a config error, caught here loudly
            // rather than silently clamping tiles to a mismatched box.
            ENFORCE(m_cfg.bounds)("world_config.grid requires world_config.bounds (the shared extent)");
            const aabb& b = *m_cfg.bounds;
            const vec ts = m_cfg.grid->tile_size;
            ENFORCE(ts.x() > 0.0f && ts.y() > 0.0f)("grid tile_size must be positive");
            const float fcols = (b.max.x() - b.min.x()) / ts.x();
            const float frows = (b.max.y() - b.min.y()) / ts.y();
            const auto cols = static_cast <uint32_t>(std::lround(fcols));
            const auto rows = static_cast <uint32_t>(std::lround(frows));
            ENFORCE(cols > 0 && rows > 0)("grid extent (bounds / tile_size) must be at least one cell");
            ENFORCE(std::abs(fcols - static_cast <float>(cols)) < 1e-3f
                && std::abs(frows - static_cast <float>(rows)) < 1e-3f)
                ("world_config.bounds must be an integer multiple of grid tile_size");
            m_static_grid.emplace(grid <detail::tile>::from_tile_size(b.min, ts, cols, rows));
        }
    }

    inline collider_id world::add(entity_id_t eid, const static_body& body) {
        auto idx = m_bodies_storage.allocate();
        auto& stored = m_bodies_storage[idx];

        stored.shape = body.shape;
        stored.kind = detail::body_kind::STATIC;
        stored.filter = body.filter;
        stored.material = body.material;
        stored.eid = eid;

        auto box = fatten(stored);
        stored.proxy = insert_leaf(m_space_partition, idx, box);
        return {idx, m_bodies_storage.generation(idx), collider_id::BODY};
    }

    inline collider_id world::add(entity_id_t eid, const kinematic_body& body) {
        auto idx = m_bodies_storage.allocate();
        auto& stored = m_bodies_storage[idx];

        stored.shape = detail::widen(body.shape); // moving_shape_t -> shape_t (always valid)
        stored.kind = detail::body_kind::KINEMATIC;
        stored.filter = body.filter;
        stored.material = body.material;
        stored.velocity = body.velocity;
        stored.eid = eid;

        auto box = fatten(stored);
        stored.proxy = insert_leaf(m_space_partition, idx, box);
        return {idx, m_bodies_storage.generation(idx), collider_id::BODY};
    }

    inline collider_id world::add(entity_id_t eid, const carrier_body& body) {
        auto idx = m_bodies_storage.allocate();
        auto& stored = m_bodies_storage[idx];

        stored.shape = detail::widen(body.shape);
        stored.kind = detail::body_kind::CARRIER;
        stored.filter = body.filter;
        stored.material = body.material;
        stored.velocity = body.velocity;
        stored.surface_velocity = body.surface_velocity;
        stored.eid = eid;

        auto box = fatten(stored);
        stored.proxy = insert_leaf(m_space_partition, idx, box);
        return {idx, m_bodies_storage.generation(idx), collider_id::BODY};
    }

    inline collider_id world::add(entity_id_t eid, const bullet& body) {
        auto idx = m_bullets_storage.allocate();
        auto& stored = m_bullets_storage[idx];

        stored.shape = body.shape;
        stored.filter = body.filter;
        stored.material = body.material;
        stored.velocity = body.velocity;
        stored.eid = eid;
        return {idx, m_bullets_storage.generation(idx), collider_id::BULLET};
    }

    inline collider_id world::add(entity_id_t eid, const tile_body& body) {
        ENFORCE(m_static_grid)("add(tile_body) requires a grid (world_config.grid)");
        const aabb bound = detail::tight_box(body.shape);
        const vec centre = bound.center();
        const uint32_t cell = m_static_grid->to_cell(centre);
        ENFORCE(cell != grid<detail::tile>::INVALID_CELL)("tile centre is outside the grid bounds");
        // A tile is only stored in (and only found via) its centre's cell, so it must fit
        // within that cell -- otherwise queries through the overhang would silently miss it.
        ENFORCE(detail::contains(m_static_grid->cell_box_at(cell), bound))
            ("tile shape must fit within a single grid cell");
        // Mergeable (static) geometry is baked once on the first run(); it cannot be added
        // afterwards (the bake is destructive and not re-run). Non-mergeable tiles are free.
        ENFORCE(!(body.mergeable && m_compiled))
            ("mergeable tiles must be added before the first run() (static geometry is baked once)");
        m_static_grid->set(centre,
                           detail::tile{body.shape, body.material, body.filter, eid, body.mergeable});
        // Stamp the post-set generation: a later overwrite/clear of this cell bumps it,
        // so this handle then reads as invalid instead of silently aliasing the new tile.
        const collider_id id{cell, m_static_grid->cell_generation(cell), collider_id::TILE};
        // Track sensor tiles so the trigger pass can scan them (bodies-only loop can't).
        // Stale entries (cell overwritten/cleared) are pruned lazily in the trigger pass.
        if (body.material.response == response_mode::SENSOR) {
            m_sensor_tiles.push_back(id);
        }
        return id;
    }

    inline void world::remove(collider_id cid) {
        if (!is_valid(cid)) {
            return;
        }
        if (cid.type_id == collider_id::BODY) {
            auto& stored = m_bodies_storage[cid.value];
            remove_leaf(m_space_partition, stored.proxy);
            m_bodies_storage.deallocate(cid.value);
        } else if (cid.type_id == collider_id::BULLET) {
            m_bullets_storage.deallocate(cid.value);
        } else {
            // TILE
            m_static_grid->clear_at(cid.value);
        }
    }

    inline void world::clear() {
        m_bodies_storage.clear();
        m_bullets_storage.clear();
        m_space_partition.reset();
        if (m_static_grid) {
            m_static_grid->reset();
        }
        m_events.clear();
        m_triggers_curr.clear();
        m_triggers_prev.clear();
        m_sensor_tiles.clear();
        m_compiled = false; // a fresh level may bake again
    }

    inline void world::set_shape(collider_id cid, const shape_t& shape) {
        ENFORCE(is_valid(cid));
        if (cid.type_id == collider_id::BODY) {
            auto& stored = m_bodies_storage[cid.value];
            if (stored.kind != detail::body_kind::STATIC) {
                // any mover (KINEMATIC actor or CARRIER) must stay an aabb | circle -- a
                // segment/triangle would later trip detail::narrow() in the move/carrier pass.
                ENFORCE(std::holds_alternative<aabb>(shape) || std::holds_alternative<circle>(shape));
            }
            stored.shape = shape;
            auto box = fatten(stored);
            update_leaf(m_space_partition, stored.proxy, box);
        } else if (cid.type_id == collider_id::BULLET) {
            auto& stored = m_bullets_storage[cid.value];
            stored.shape = detail::narrow(shape); // ENFORCE non-segment + shape_t -> moving_shape_t
        } else {
            // TILE: reshape in place. The new shape must still fit the same cell
            // (no re-bucketing) -- else it could overhang into a cell that won't find it.
            const aabb nb = detail::tight_box(shape);
            ENFORCE(detail::contains(m_static_grid->cell_box_at(cid.value), nb))
                ("set_shape: a tile's new shape must fit within its cell");
            // A mergeable tile is frozen after the one-shot bake -- otherwise a mergeable
            // tile that survived the bake (e.g. a slope, not an aabb) could be reshaped into a
            // cell-filling BLOCK aabb and smuggle un-baked static geometry past add()'s guard.
            ENFORCE(!(m_compiled && m_static_grid->at(cid.value)->mergeable))
                ("a mergeable tile cannot be reshaped after the first run() (static geometry is baked once)");
            m_static_grid->at(cid.value)->shape = shape;
        }
    }

    inline void world::set_velocity(collider_id cid, const vec& v) {
        ENFORCE(is_valid(cid));
        // Tiles are static -- no velocity. (A moving tile is a kinematic body, not a tile.)
        ENFORCE(cid.type_id != collider_id::TILE)("a tile has no velocity");
        if (cid.type_id == collider_id::BODY) {
            auto& stored = m_bodies_storage[cid.value];
            // A static body has no velocity; kinematic actors AND carriers (scripted paths) do.
            ENFORCE(stored.kind != detail::body_kind::STATIC)("a static body has no velocity");
            stored.velocity = v;
        } else {
            auto& stored = m_bullets_storage[cid.value];
            stored.velocity = v;
        }
    }

    inline void world::set_surface_velocity(collider_id cid, const vec& v) {
        ENFORCE(is_valid(cid) && cid.type_id == collider_id::BODY);
        auto& stored = m_bodies_storage[cid.value];
        ENFORCE(stored.kind == detail::body_kind::CARRIER)("surface_velocity is carrier-only");
        stored.surface_velocity = v;
    }

    inline bool world::is_valid(collider_id cid) const {
        if (cid.type_id == collider_id::BODY) {
            return m_bodies_storage.is_alive(cid.value)
                   && m_bodies_storage.generation(cid.value) == cid.generation;
        }
        if (cid.type_id == collider_id::BULLET) {
            return m_bullets_storage.is_alive(cid.value)
                   && m_bullets_storage.generation(cid.value) == cid.generation;
        }
        // TILE: live iff the grid exists, the cell is occupied, AND the cell has not been
        // mutated (overwritten/cleared) since this handle was made -- the generation check
        // closes the stale-handle alias.
        return m_static_grid && m_static_grid->at(cid.value) != nullptr
               && m_static_grid->cell_generation(cid.value) == cid.generation;
    }

    inline shape_t world::get_shape(collider_id cid) const {
        ENFORCE(is_valid(cid));
        if (cid.type_id == collider_id::BODY) {
            return m_bodies_storage[cid.value].shape;
        }
        if (cid.type_id == collider_id::BULLET) {
            return detail::widen(m_bullets_storage[cid.value].shape);
        }
        return m_static_grid->at(cid.value)->shape; // TILE: stored verbatim
    }

    inline vec world::get_velocity(collider_id cid) const {
        ENFORCE(is_valid(cid));
        if (cid.type_id == collider_id::BODY) {
            return m_bodies_storage[cid.value].velocity;
        }
        if (cid.type_id == collider_id::BULLET) {
            return m_bullets_storage[cid.value].velocity;
        }
        return vec{0, 0}; // TILE: static
    }

    inline entity_id_t world::get_eid(collider_id cid) const {
        ENFORCE(is_valid(cid));
        if (cid.type_id == collider_id::BODY) {
            return m_bodies_storage[cid.value].eid;
        }
        if (cid.type_id == collider_id::BULLET) {
            return m_bullets_storage[cid.value].eid;
        }
        return m_static_grid->at(cid.value)->eid; // TILE: from the cell payload
    }

    inline const std::vector <world_event>& world::run(const aabb& active_region, float dt) {
        m_events.clear();
        compile_static_grid(); // one-shot tile boundary-bake, before anything queries
        carrier_pass(active_region, dt); // §19 #1: carry (MP1) + push (MP2) + crush (MP3)
        movement_pass(active_region, dt); // kinematic move-and-slide -> COLLISION events
        bullet_pass(active_region, dt); // bullets -> BULLET_HIT / BULLET_EXPIRED
        trigger_pass(); // sensor overlap diff -> TRIGGER_BEGIN / END
        return m_events;
    }
}
