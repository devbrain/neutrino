//
// Created by igor on 07/07/2026.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

#include <neutrino/neutrino_export.h>
#include <neutrino/world/content_key.hh>
#include <neutrino/world/world.hh>
#include <neutrino/video/world/tileset_bundle.hh>

namespace neutrino {
    /**
     * @brief Non-owning reference to a cached tileset bundle.
     *
     * Returned by @ref resource_cache::acquire and passed back to
     * @ref resource_cache::release. @ref bundle points into cache-owned storage that
     * stays put for the entry's whole life, so the handle is safe to hold and to
     * resolve through every frame while it is acquired. @ref key names the entry so
     * @ref release can find it.
     */
    struct bundle_handle {
        content_key           key{};
        const tileset_bundle* bundle{nullptr};
        /// Identifies the exact cache entry this handle was issued for. Content keys
        /// recycle (same content -> same key), so a rebuilt entry reuses the old key;
        /// the token lets @ref resource_cache::release reject a stale handle whose
        /// entry was torn down and rebuilt rather than decrement the new entry.
        std::uint64_t         token{0};

        [[nodiscard]] bool valid() const noexcept { return bundle != nullptr; }

        /// @brief Registered visual for a local tile (invalid ref if absent).
        [[nodiscard]] sprite_visual_ref visual(world_local_tile_id id) const {
            return bundle->visual(id);
        }

        /// @brief Shared animation state for a local tile (invalid id if not animated).
        [[nodiscard]] sprite_state_id state(world_local_tile_id id) const {
            return bundle->state(id);
        }
    };

    /**
     * @brief Content-keyed, refcounted cache of tileset render bundles.
     *
     * Turns "load/unload only the delta" on a level switch into a refcount: a
     * tileset shared between the old and new level is acquired again (its refcount
     * rises) instead of rebuilt, and only genuinely new tilesets call
     * @ref build_bundle while genuinely departed ones eventually unload.
     *
     * Identity is by tileset *content* (@ref build_bundle inputs: image bytes, grid
     * geometry, per-tile images and animations — never the map-level `first_gid`),
     * so two tilesets with the same content share one bundle and a changed image
     * yields a new one. File-backed images are identified cheaply by a
     * (path, mtime, size) stat key that maps to a cached content hash, recomputed
     * only when the file changes.
     *
     * When an entry's refcount reaches zero it moves to a bounded LRU cold pool: it
     * stays registered and resident (animations keep ticking, re-acquire is
     * instant) until the pool overflows, at which point the least-recently-used cold
     * bundle is torn down.
     *
     * The caller's contract is acquire-before-release; the sprite manager's own
     * `uses()` guard remains the safety net. Not copyable.
     */
    class NEUTRINO_EXPORT resource_cache {
        public:
            /// @brief Default number of idle bundles the cold pool retains.
            static constexpr std::size_t default_cold_budget = 8;

            explicit resource_cache(std::size_t cold_budget = default_cold_budget);
            ~resource_cache();

            resource_cache(const resource_cache&) = delete;
            resource_cache& operator=(const resource_cache&) = delete;

            /**
             * @brief Acquire the bundle for @p tileset, building it on a cache miss.
             *
             * A hit bumps the refcount (resurrecting the entry from the cold pool if
             * needed); a miss calls @ref build_bundle and inserts it at refcount 1.
             *
             * @pre An application must be initialized.
             * @throws (via @ref build_bundle / image loading) when a miss cannot build.
             */
            [[nodiscard]] bundle_handle acquire(const world_tileset& tileset);

            /**
             * @brief Release a previously acquired handle.
             *
             * Drops the entry's refcount; at zero the bundle moves to the cold pool,
             * which may evict and tear down the least-recently-used cold bundle.
             * Invalid or already-evicted handles are ignored.
             */
            void release(const bundle_handle& handle);

            /// @brief Number of resident bundles (in use plus cold).
            [[nodiscard]] std::size_t resident_count() const noexcept;

            /// @brief Number of idle bundles currently held in the cold pool.
            [[nodiscard]] std::size_t cold_count() const noexcept;

        private:
            struct impl;
            std::unique_ptr<impl> m_impl;
    };
}
