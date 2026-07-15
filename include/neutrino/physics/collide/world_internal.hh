//
// Internal (detail-namespace) implementation types that the `world` class is built on.
// Split out of world_types.hh so the public vocabulary header stays free of broadphase/storage
// internals: world_types.hh is the client-facing types, world_internal.hh is world's private guts.
//

/**
 * @file world_internal.hh
 * @brief The @c neutrino::physics::detail implementation types behind @ref neutrino::physics::world.
 *
 * Holds the non-public machinery the world is built on, split out of @ref world_types.hh so the
 * public vocabulary header carries only client-facing types:
 *   - @ref neutrino::physics::detail::tile -- the static grid's per-cell payload.
 *   - @ref neutrino::physics::detail::body_kind / @ref neutrino::physics::detail::resident_body /
 *     @ref neutrino::physics::detail::nonresident_body -- the stored body records.
 *   - @ref neutrino::physics::detail::internal_storage -- the generation-tracked slot-map arena
 *     (with @c bodies_storage / @c bullets_storage aliases) and its live-only iterator.
 *   - @c narrow / @c widen -- shape-variant conversions between @ref neutrino::physics::shape_t and
 *     @ref neutrino::physics::moving_shape_t.
 *   - @ref neutrino::physics::detail::eval_velocity_response -- the surface-material velocity response.
 *   - @c tight_box -- the un-fattened bounding box of a shape.
 *
 * @note Pulls in @ref world_types.hh for the public vocabulary these internals depend on. Included
 *       by @ref world.hh; game code includes only @ref world_types.hh.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <variant>
#include <vector>

#include <neutrino/physics/collide/world_types.hh>
#include <failsafe/enforce.hh>

namespace neutrino::physics::detail {
    /**
         * @brief The static grid's per-cell payload.
         *
         * The tile's geometry stored verbatim (so @ref world::get_shape is a straight read), plus its
         * material/filter and identity @c eid (for events, like residents).
         */
    struct tile {
        shape_t shape{aabb{}};   ///< The tile geometry (stored verbatim).
        material_props material; ///< Surface response.
        filter_props filter;     ///< Collision layers.
        entity_id_t eid{};       ///< Game entity id (event payload).
        bool mergeable{false};   ///< Opted into boundary-compilation (adjacent solids merged into spans).
    };

    /// @brief The internal classification of a resident body (set by @ref world::add).
    enum class body_kind {
        STATIC,    ///< Immovable.
        KINEMATIC, ///< An actor: move-and-slide each frame.
        CARRIER    ///< A kinematic "solid": moves rigidly on a path and carries/pushes riders.
    };

    /// @brief A body stored in the broadphase tree (static, kinematic, or carrier).
    struct resident_body {
        shape_t shape{aabb{}};      ///< Current shape.
        material_props material;    ///< Surface response.
        filter_props filter;        ///< Collision layers.
        vec velocity{0, 0};         ///< Per-frame velocity (kinematic/carrier).
        vec surface_velocity{0, 0}; ///< CARRIER only: tangential drag imparted to riders (conveyor).
        entity_id_t eid{};          ///< Game entity id (event payload).
        body_kind kind{body_kind::STATIC}; ///< Static / kinematic / carrier.
        node_ptr proxy{};           ///< Broadphase tree handle.
        uint32_t generation{0};     ///< Slot recycle counter (handle staleness).
        bool alive{true};           ///< False while the slot is on the free list.
    };

    /// @brief A bullet record -- NOT in the tree (bullets sweep against the world via one-way CCD).
    struct nonresident_body {
        moving_shape_t shape{aabb{}}; ///< Mover shape (bullets are always movers -> tight variant).
        material_props material;      ///< Surface response.
        filter_props filter;          ///< Collision layers.
        vec velocity{0, 0};           ///< Velocity.
        entity_id_t eid{};            ///< Game entity id (event payload).
        uint32_t generation{0};       ///< Slot recycle counter (handle staleness).
        bool alive{true};             ///< False while the slot is on the free list.
    };

    /// @brief Does @c T carry a @c generation field? (Selects whether @ref internal_storage
        ///        recycle-tracks a slot.)
    template<typename T>
    concept has_generation = requires(T obj)
    {
        obj.generation;
    };

    /**
         * @brief A generation-tracked slot-map arena for body records, with live-only iteration.
         *
         * Slots are recycled via a free list; deallocating a slot bumps its generation (when @c Body
         * satisfies @ref has_generation) so a stale handle reads as invalid. Marking a slot dead does
         * not move storage, so an in-flight iterator survives mid-loop deallocation and reallocation.
         *
         * @tparam Body The stored record type (@ref resident_body or @ref nonresident_body).
         */
    template<typename Body>
    class internal_storage {
        public:
            /// @brief Construct an empty arena.
            internal_storage() = default;

            /// @brief Allocate a slot (reusing a free one if available) and return its index.
            uint32_t allocate() {
                if (!m_free.empty()) {
                    auto idx = m_free.back();
                    m_free.pop_back();
                    ENFORCE(idx < m_pool.size());
                    init(m_pool[idx]);
                    return idx;
                }
                m_pool.emplace_back();
                init(m_pool.back());
                return static_cast <uint32_t>(m_pool.size() - 1);
            }

            /**
                 * @brief Free slot @p idx, bumping its generation so prior handles go stale.
                 * @param idx A live slot index (asserts liveness).
                 */
            void deallocate(uint32_t idx) {
                ENFORCE(idx < m_pool.size());
                ENFORCE(m_pool[idx].alive);
                m_pool[idx].alive = false;
                if constexpr (has_generation <Body>) {
                    ++m_pool[idx].generation;
                }
                m_free.push_back(idx);
            }

            /**
                 * @brief Drop every element (capacity retained).
                 * @note Each live slot is deallocated (so its generation bumps and the free list is
                 *       rebuilt), leaving all generations MONOTONIC -- a handle held across a clear
                 *       never aliases a slot reused afterwards.
                 */
            void clear() {
                for (uint32_t i = 0; i < m_pool.size(); ++i) {
                    if (m_pool[i].alive) {
                        deallocate(i);
                    }
                }
            }

            /// @brief Index -> record access.
            Body& operator[](uint32_t idx) {
                return m_pool[idx];
            }

            /// @copydoc operator[](uint32_t)
            const Body& operator[](uint32_t idx) const {
                return m_pool[idx];
            }

            /// @brief Is slot @p idx in range and live?
            [[nodiscard]] bool is_alive(uint32_t idx) const { return idx < m_pool.size() && m_pool[idx].alive; }

            /**
                 * @brief Generation of slot @p idx.
                 * @param idx Slot index.
                 * @return The slot's generation; @c 0 for a @c Body without a generation field (nothing
                 *         to recycle-detect, so validity reduces to liveness). Keeps every
                 *         @c .generation access guarded.
                 */
            [[nodiscard]] uint32_t generation([[maybe_unused]] uint32_t idx) const {
                if constexpr (has_generation <Body>) {
                    return m_pool[idx].generation;
                } else {
                    return 0;
                }
            }

            // ---- live-only iteration ------------------------------------------------
            /**
                 * @brief Forward iterator over the @e live slots (dead slots are skipped).
                 *
                 * Marking a slot dead via @ref deallocate does not move storage, so an in-flight
                 * iterator stays valid -- you can @c deallocate(it.index()) the current element
                 * mid-loop and continue. The iterator dereferences through the vector each step, so it
                 * also survives a reallocation from @ref allocate.
                 *
                 * @tparam Const Whether this is the const iterator.
                 */
            template<bool Const>
            class iterator_t {
                using pool_t = std::conditional_t <Const, const std::vector <Body>, std::vector <Body>>;
                pool_t* m_pool = nullptr;
                std::size_t m_idx = 0;

                void skip_dead() {
                    while (m_idx < m_pool->size() && !(*m_pool)[m_idx].alive) {
                        ++m_idx;
                    }
                }

                public:
                    using value_type = Body;
                    using reference = std::conditional_t <Const, const Body&, Body&>;
                    using pointer = std::conditional_t <Const, const Body*, Body*>;
                    using difference_type = std::ptrdiff_t;
                    using iterator_category = std::forward_iterator_tag;

                    iterator_t() = default;

                    iterator_t(pool_t* pool, std::size_t idx)
                        : m_pool(pool), m_idx(idx) { skip_dead(); }

                    [[nodiscard]] reference operator*() const { return (*m_pool)[m_idx]; }
                    [[nodiscard]] pointer operator->() const { return &(*m_pool)[m_idx]; }

                    /// @brief Slot index of the current element -- pass to @ref deallocate to kill it.
                    [[nodiscard]] uint32_t index() const { return static_cast <uint32_t>(m_idx); }

                    iterator_t& operator++() {
                        ++m_idx;
                        skip_dead();
                        return *this;
                    }

                    iterator_t operator++(int) {
                        auto tmp = *this;
                        ++(*this);
                        return tmp;
                    }

                    [[nodiscard]] bool operator==(const iterator_t& o) const { return m_idx == o.m_idx; }
                    [[nodiscard]] bool operator!=(const iterator_t& o) const { return m_idx != o.m_idx; }
            };

            using iterator = iterator_t <false>;       ///< Mutable live-slot iterator.
            using const_iterator = iterator_t <true>;  ///< Const live-slot iterator.

            /// @brief Iterator to the first live slot.
            [[nodiscard]] iterator begin() { return iterator(&m_pool, 0); }
            /// @brief Past-the-end iterator.
            [[nodiscard]] iterator end() { return iterator(&m_pool, m_pool.size()); }
            /// @copydoc begin()
            [[nodiscard]] const_iterator begin() const { return const_iterator(&m_pool, 0); }
            /// @copydoc end()
            [[nodiscard]] const_iterator end() const { return const_iterator(&m_pool, m_pool.size()); }
            /// @brief Const iterator to the first live slot.
            [[nodiscard]] const_iterator cbegin() const { return begin(); }
            /// @brief Const past-the-end iterator.
            [[nodiscard]] const_iterator cend() const { return end(); }

        private:
            /// @brief (Re)initialise slot @p b as a fresh live record, preserving its generation.
            static void init(Body& b) {
                if constexpr (has_generation <Body>) {
                    const uint32_t gen = b.generation; // the one thing that must survive
                    b = Body{}; // clear shape/velocity/proxy/kind/...
                    b.generation = gen;
                    b.alive = true;
                } else {
                    b = Body{}; // clear shape/velocity/proxy/kind/...
                    b.alive = true;
                }
            }

        private:
            std::vector <Body> m_pool; ///< Dense record store (indices are slot handles).
            std::vector <uint32_t> m_free; ///< Free-list of recycled slot indices.
    };

    using bodies_storage = internal_storage <resident_body>;    ///< Storage of resident bodies.
    using bullets_storage = internal_storage <nonresident_body>; ///< Storage of bullets.

    /**
         * @brief Narrow a @ref shape_t to a @ref moving_shape_t (aabb | circle).
         * @param s The wide shape.
         * @return The mover variant.
         * @note The segment/triangle case is unreachable for movers (the typed DTOs guarantee it at
         *       construction); the @c ENFORCE is a never-fires safety net.
         */
    constexpr moving_shape_t narrow(const shape_t& s) {
        return std::visit([]<typename T0>(const T0& shp) -> moving_shape_t {
            using S = std::decay_t <T0>;
            if constexpr (std::is_same_v <S, aabb> || std::is_same_v <S, circle>) {
                return moving_shape_t{shp};
            } else {
                // segment or triangle: not a valid mover (movers are aabb | circle)
                ENFORCE(false)("a moving body/bullet must be an aabb or circle");
                return moving_shape_t{aabb{}}; // unreachable; satisfies the return type
            }
        }, s);
    }

    /**
         * @brief Widen a @ref moving_shape_t to a @ref shape_t (always valid).
         * @param s The mover shape.
         * @return The wide shape variant.
         */
    constexpr shape_t widen(const moving_shape_t& s) {
        return std::visit([](const auto& shp) -> shape_t { return shp; }, s);
    }

    /**
         * @brief Geometric velocity response at a contact: the post-collision velocity after removing
         *        the blocked (into-surface) component and applying the surface material.
         *
         * @verbatim
         *   v_n = dot(v, n) * n        -- component of v along the contact normal
         *   v_t = v - v_n              -- component tangent to the surface
         *   v'  = (1 - friction) * v_t -- friction damps sliding (0 = frictionless "ice", 1 = no slide)
         *       - restitution * v_n    -- restitution reflects the normal part (0 = absorb, 1 = bounce)
         * @endverbatim
         *
         * This is purely the BLOCK / bounce math. WHETHER a contact is resolved at all (BLOCK vs
         * ONE_WAY vs SENSOR/IGNORE) is the caller's decision via @c material.response. Position
         * sliding is handled separately (move-and-slide projects the leftover displacement); this
         * affects velocity only.
         *
         * @param vel      The pre-collision velocity.
         * @param n        The contact normal -- MUST be unit length (@c swept_hit::entry_normal is); a
         *                 non-unit @c n mis-scales the projection by |n|^2. @c n == {0,0} (the
         *                 "undefined normal") degrades to @c (1 - friction) * v (no normal removal, no
         *                 bounce) -- callers should skip the response on a zero normal.
         * @param material The SURFACE's material (the body hit), per the surface-owned response model.
         *                 @c friction / @c restitution are expected in [0,1] (friction > 1 reverses
         *                 tangential motion; restitution > 1 injects energy).
         * @return The post-collision velocity.
         * @note The intermediates are @c vec, not @c auto -- euler is an expression-template library,
         *       so @c auto would capture a lazy expression that can dangle past this scope.
         */
    inline
    units::velocity eval_velocity_response(units::velocity vel, const vec& n,
                                           const material_props& material) {
        const vec v = vel.value;
        const vec v_n = euler::dot(v, n) * n; // component along the contact normal
        const vec v_t = v - v_n; // component tangent to the surface
        return units::velocity{(1.0f - material.friction) * v_t - material.restitution * v_n};
    }

    /**
         * @brief Tight (un-fattened) bounding box of a shape.
         * @param s Any @ref shape_t.
         * @return The shape's enclosing AABB. Centralizes the repeated
         *         @c std::visit([](const auto& s){ return enclose(s); }, shape) across the world.
         */
    [[nodiscard]] inline aabb tight_box(const shape_t& s) {
        return std::visit([](const auto& x) { return enclose(x); }, s);
    }
    /// @copydoc tight_box(const shape_t&)
    [[nodiscard]] inline aabb tight_box(const moving_shape_t& s) {
        return std::visit([](const auto& x) { return enclose(x); }, s);
    }
}
