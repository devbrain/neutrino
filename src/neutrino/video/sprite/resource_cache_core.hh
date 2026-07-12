//
// Created by igor on 12/07/2026.
//

#pragma once

//
// Internal shared engine for the content-keyed, refcounted render-resource caches
// (the tile `resource_cache` and the sprite cache). Generic over the owned bundle
// type; knows nothing about tilesets or sprites -- the front-end computes the content
// key and supplies a build callable.
//
// An entry lives while any handle holds it (refcount > 0); at refcount 0 it moves to a
// bounded LRU cold pool (still registered/resident, re-acquire instant) until the pool
// overflows and the least-recently-used cold bundle is evicted (its Bundle destructor
// unregisters its resources). Tokens distinguish an entry from a later rebuild under a
// recycled key, so a stale handle can't decrement the wrong entry.
//
// resource_lease<Bundle> is the RAII refcounting handle a sprite instance holds: copy
// retains, destruction releases. The tile front-end keeps its own value handle +
// manual acquire/release on top of the same primitives.
//

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>

#include <neutrino/world/content_key.hh>

#include "utils/lru.hh"

namespace neutrino {
    template <class Bundle>
    class resource_cache_core {
        public:
            /// @brief Non-owning value handle into cache-owned storage.
            struct handle {
                content_key   key{};
                const Bundle* bundle{nullptr};
                std::uint64_t token{0};

                [[nodiscard]] bool valid() const noexcept { return bundle != nullptr; }
            };

            explicit resource_cache_core(std::size_t cold_budget)
                : m_cold(cold_budget) {
            }

            /**
             * @brief Acquire the entry for @p key, building it on a miss via @p build.
             *
             * A hit bumps the refcount (resurrecting from the cold pool if idle); a miss
             * calls @p build() -> Bundle and inserts it at refcount 1. @p build runs only
             * on a miss, before insertion, so a throwing build leaves the cache untouched.
             */
            template <class Build>
            handle acquire(const content_key& key, Build&& build) {
                if (const auto it = m_entries.find(key); it != m_entries.end()) {
                    if (it->second.refcount == 0) {
                        m_cold.erase(key); // resurrect from the cold pool
                    }
                    ++it->second.refcount;
                    return handle{key, &it->second.bundle, it->second.token};
                }

                Bundle bundle = std::forward <Build>(build)();
                const std::uint64_t token = m_next_token++;
                const auto [it, inserted] =
                    m_entries.emplace(key, entry{std::move(bundle), 1, token});
                return handle{key, &it->second.bundle, token};
            }

            /// @brief Bump the refcount of a live entry (a lease copy). No-op for an
            ///        invalid or stale handle.
            void retain(const handle& h) {
                if (!h.valid()) {
                    return;
                }
                const auto it = m_entries.find(h.key);
                if (it == m_entries.end() || it->second.token != h.token) {
                    return;
                }
                ++it->second.refcount;
            }

            /// @brief Drop the refcount; at zero the bundle goes cold (which may evict the
            ///        LRU cold bundle). Invalid, already-idle, or stale handles are ignored.
            void release(const handle& h) {
                if (!h.valid()) {
                    return;
                }
                const auto it = m_entries.find(h.key);
                if (it == m_entries.end() || it->second.refcount == 0 || it->second.token != h.token) {
                    return;
                }
                if (--it->second.refcount == 0) {
                    if (const auto evicted = m_cold.touch(h.key)) {
                        m_entries.erase(*evicted);
                    }
                }
            }

            [[nodiscard]] std::size_t resident_count() const noexcept { return m_entries.size(); }
            [[nodiscard]] std::size_t cold_count() const noexcept { return m_cold.size(); }

        private:
            struct entry {
                Bundle        bundle;
                int           refcount;
                std::uint64_t token; ///< Distinguishes this entry from a later rebuild under the same key.
            };

            std::unordered_map <content_key, entry> m_entries;
            utils::lru_index <content_key>          m_cold;
            std::uint64_t                           m_next_token = 1; ///< Monotonic; stamps each fresh build.
    };

    /**
     * @brief RAII refcounting lease on a @ref resource_cache_core entry.
     *
     * Copy retains (refcount up), destruction releases. Holding one keeps its cache
     * entry resident, so a sprite instance's playhead can never outlive the animations
     * its set owns. Empty (default-constructed / moved-from) leases release nothing.
     */
    template <class Bundle>
    class resource_lease {
        public:
            using core   = resource_cache_core <Bundle>;
            using handle = typename core::handle;

            resource_lease() = default;
            resource_lease(core& cache, handle h) noexcept
                : m_cache(&cache), m_handle(h) {
            }

            resource_lease(const resource_lease& other)
                : m_cache(other.m_cache), m_handle(other.m_handle) {
                if (m_cache) {
                    m_cache->retain(m_handle);
                }
            }

            resource_lease& operator=(const resource_lease& other) {
                if (this != &other) {
                    if (other.m_cache) {
                        other.m_cache->retain(other.m_handle); // retain new before releasing old
                    }
                    reset();
                    m_cache  = other.m_cache;
                    m_handle = other.m_handle;
                }
                return *this;
            }

            resource_lease(resource_lease&& other) noexcept
                : m_cache(other.m_cache), m_handle(other.m_handle) {
                other.m_cache = nullptr;
                other.m_handle = {};
            }

            resource_lease& operator=(resource_lease&& other) noexcept {
                if (this != &other) {
                    reset();
                    m_cache  = other.m_cache;
                    m_handle = other.m_handle;
                    other.m_cache = nullptr;
                    other.m_handle = {};
                }
                return *this;
            }

            ~resource_lease() { reset(); }

            [[nodiscard]] bool valid() const noexcept { return m_handle.valid(); }
            [[nodiscard]] const Bundle* operator->() const noexcept { return m_handle.bundle; }
            [[nodiscard]] const Bundle& operator*() const noexcept { return *m_handle.bundle; }
            [[nodiscard]] const handle& raw() const noexcept { return m_handle; }

        private:
            void reset() noexcept {
                if (m_cache) {
                    m_cache->release(m_handle);
                }
                m_cache = nullptr;
                m_handle = {};
            }

            core*  m_cache{nullptr};
            handle m_handle{};
    };
}
