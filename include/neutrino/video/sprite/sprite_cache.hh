//
// Created by igor on 12/07/2026.
//

#pragma once

/**
 * @file sprite_cache.hh
 * @brief Content-keyed, refcounted cache of built @ref sprite_set assets.
 *
 * `acquire(sprite_def)` builds on a miss and shares on a hit, keyed by @ref key_for
 * content, with a bounded LRU cold pool. The handle it returns -- @ref sprite_set_handle --
 * is a **RAII refcounting lease** (copy retains, destruction releases), so a per-instance
 * playhead can hold one and keep its set resident: the animations a `sprite_state` plays
 * can never be unregistered under a live instance.
 *
 * The cache must outlive every handle it issues.
 */

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include <neutrino/neutrino_export.h>
#include <neutrino/world/content_key.hh>
#include <neutrino/video/sprite/sprite_def.hh>
#include <neutrino/video/sprite/sprite_set.hh>
#include <neutrino/video/sprite/sprite_state.hh>

namespace neutrino {
    class sprite_cache;
    class sprite_instance;

    /**
     * @brief RAII lease on a cache-owned @ref sprite_set, addressed by name.
     *
     * Copy retains (the set stays resident), destruction releases. Empty leases
     * (default-constructed / moved-from) reference nothing. Name lookups forward to the
     * leased set.
     */
    class NEUTRINO_EXPORT sprite_set_handle {
        public:
            sprite_set_handle() = default;
            sprite_set_handle(const sprite_set_handle& other);
            sprite_set_handle& operator=(const sprite_set_handle& other);
            sprite_set_handle(sprite_set_handle&& other) noexcept;
            sprite_set_handle& operator=(sprite_set_handle&& other) noexcept;
            ~sprite_set_handle();

            [[nodiscard]] bool valid() const noexcept { return m_set != nullptr; }

            /// @brief The registered visual bound to @p name in the set, or nullopt.
            [[nodiscard]] std::optional <sprite_visual_ref> visual(std::string_view name) const {
                return m_set ? m_set->visual(name) : std::nullopt;
            }

            /// @brief The registered animation bound to clip @p name, or nullopt.
            [[nodiscard]] std::optional <sprite_animation_id> clip(std::string_view name) const {
                return m_set ? m_set->clip(name) : std::nullopt;
            }

            /**
             * @brief Spawn a per-instance playhead for clip @p name.
             *
             * Returns a @ref sprite_instance that holds a *copy* of this lease (so the set
             * -- and the animations it owns -- stays resident while the instance lives) plus
             * its own `sprite_state`. An unknown clip yields an invalid instance.
             */
            [[nodiscard]] sprite_instance spawn(std::string_view name) const;

        private:
            friend class sprite_cache;
            sprite_set_handle(sprite_cache* cache, content_key key, std::uint64_t token,
                              const sprite_set* set) noexcept
                : m_cache(cache), m_key(key), m_token(token), m_set(set) {
            }

            sprite_cache*     m_cache{nullptr};
            content_key       m_key{};
            std::uint64_t     m_token{0};
            const sprite_set* m_set{nullptr};
    };

    /**
     * @brief Content-keyed, refcounted store of built @ref sprite_set assets.
     *
     * @ref acquire builds a set on a miss and shares it on a hit, keyed by @ref key_for
     * content; idle sets linger in a bounded LRU cold pool before eviction. Each hit hands
     * back a @ref sprite_set_handle lease, and the cache must outlive every handle it issues.
     */
    class NEUTRINO_EXPORT sprite_cache {
        public:
            /// @brief Default number of idle sets the cold pool retains.
            static constexpr std::size_t default_cold_budget = 8;

            explicit sprite_cache(std::size_t cold_budget = default_cold_budget);
            ~sprite_cache();

            sprite_cache(const sprite_cache&)            = delete;
            sprite_cache& operator=(const sprite_cache&) = delete;

            /**
             * @brief Acquire the set for @p def, building it on a miss.
             *
             * A hit resurrects/shares the entry; a miss calls @ref build_sprite_set. The
             * returned lease owns one reference.
             *
             * @pre An application must be initialized.
             * @throws (via @ref build_sprite_set) when a miss cannot build.
             */
            [[nodiscard]] sprite_set_handle acquire(const sprite_def& def);

            /// @brief Number of resident sets (leased plus cold).
            [[nodiscard]] std::size_t resident_count() const noexcept;

            /// @brief Number of idle sets currently held in the cold pool.
            [[nodiscard]] std::size_t cold_count() const noexcept;

        private:
            friend class sprite_set_handle;
            void retain(const sprite_set_handle& handle);
            void release(const sprite_set_handle& handle);

            struct impl;
            std::unique_ptr <impl> m_impl;
    };

    /**
     * @brief A per-entity sprite playhead: a `sprite_state` plus a lease on its set.
     *
     * Created by @ref sprite_set_handle::spawn. Owns its `sprite_state` and a
     * @ref sprite_set_handle lease; the destructor unregisters the state **first** (while
     * the animations it references are still resident via the lease), then drops the lease.
     * Move-only. This is what a `render_layer` actor holds and draws (`draw_sprite(pos,
     * inst.state())`).
     */
    class NEUTRINO_EXPORT sprite_instance {
        public:
            sprite_instance() = default;
            ~sprite_instance();

            sprite_instance(const sprite_instance&)            = delete;
            sprite_instance& operator=(const sprite_instance&) = delete;
            sprite_instance(sprite_instance&&) noexcept;
            sprite_instance& operator=(sprite_instance&&) noexcept;

            [[nodiscard]] bool valid() const noexcept { return m_state.valid(); }

            /// @brief The runtime state to draw (`draw_sprite(pos, inst.state())`).
            [[nodiscard]] sprite_state_id state() const noexcept { return m_state; }

            /// @brief Switch playback to clip @p name, continuing elapsed time. False if unknown.
            bool switch_to(std::string_view name);

            /// @brief Restart clip @p name from its first frame. False if unknown.
            bool restart(std::string_view name);

        private:
            friend class sprite_set_handle;
            sprite_instance(sprite_set_handle lease, sprite_state_id state) noexcept
                : m_lease(std::move(lease)), m_state(state) {
            }

            sprite_set_handle m_lease;
            sprite_state_id   m_state;
    };
}
