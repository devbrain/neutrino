//
// Created by igor on 07/07/2026.
//

#pragma once

#include <cstddef>
#include <list>
#include <optional>
#include <unordered_map>

namespace neutrino::utils {
    /**
     * @brief A capacity-bounded, recency-ordered set of unique keys.
     *
     * Tracks *ordering only* — it stores no values. Callers keep the real payload
     * elsewhere (keyed by the same @p Key) and use this index to decide which key is
     * the least-recently-used and should be dropped when the budget is exceeded. The
     * classic use is a cold pool: the owner inserts a key when its entry falls idle
     * and removes it when the entry becomes live again, letting the index name the
     * eviction victim.
     *
     * A `std::list` holds the keys in order (front = least-recently-used, back =
     * most-recently-used) and a map from key to its list position makes @ref touch,
     * @ref erase and @ref contains O(1).
     *
     * @tparam Key  Hashable, equality-comparable, copyable key type.
     * @tparam Hash Hash functor for @p Key (defaults to `std::hash<Key>`).
     */
    template <class Key, class Hash = std::hash<Key>>
    class lru_index {
        public:
            /**
             * @brief Construct an index that keeps at most @p capacity keys.
             *
             * @p capacity 0 means nothing is retained: @ref touch immediately reports
             * its own key as evicted.
             */
            explicit lru_index(std::size_t capacity)
                : m_capacity(capacity) {
            }

            /**
             * @brief Insert @p key as most-recently-used, or move it there if present.
             *
             * @return The least-recently-used key that was evicted to stay within
             *         @ref capacity, or `std::nullopt` if nothing was evicted. Only a
             *         *new* key can trigger an eviction; re-touching a present key just
             *         reorders and never evicts.
             */
            std::optional<Key> touch(const Key& key) {
                if (const auto it = m_pos.find(key); it != m_pos.end()) {
                    m_order.splice(m_order.end(), m_order, it->second); // move to MRU
                    return std::nullopt;
                }
                if (m_capacity == 0) {
                    return key; // nothing is retained; the key is evicted immediately
                }
                m_order.push_back(key);
                m_pos.emplace(key, std::prev(m_order.end()));
                if (m_pos.size() > m_capacity) {
                    return pop_oldest();
                }
                return std::nullopt;
            }

            /**
             * @brief Remove @p key if present (e.g. its entry became live again).
             *
             * No-op when @p key is absent. Never evicts.
             */
            void erase(const Key& key) {
                if (const auto it = m_pos.find(key); it != m_pos.end()) {
                    m_order.erase(it->second);
                    m_pos.erase(it);
                }
            }

            /**
             * @brief Remove and return the least-recently-used key.
             *
             * @return The evicted key, or `std::nullopt` when the index is empty.
             */
            std::optional<Key> pop_oldest() {
                if (m_order.empty()) {
                    return std::nullopt;
                }
                Key oldest = std::move(m_order.front());
                m_order.pop_front();
                m_pos.erase(oldest);
                return oldest;
            }

            [[nodiscard]] bool contains(const Key& key) const {
                return m_pos.find(key) != m_pos.end();
            }

            [[nodiscard]] std::size_t size() const noexcept { return m_pos.size(); }
            [[nodiscard]] std::size_t capacity() const noexcept { return m_capacity; }
            [[nodiscard]] bool empty() const noexcept { return m_pos.empty(); }

            void clear() {
                m_order.clear();
                m_pos.clear();
            }

        private:
            std::list<Key> m_order; ///< front = LRU, back = MRU.
            std::unordered_map<Key, typename std::list<Key>::iterator, Hash> m_pos;
            std::size_t m_capacity;
    };
}
