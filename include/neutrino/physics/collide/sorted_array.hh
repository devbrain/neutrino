//
// Created by igor on 7/22/24.
//

#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <limits>
#include <type_traits>
#include <utility>
#include <vector>

#include <failsafe/enforce.hh>

namespace neutrino::physics {
    // ------------------------------------------------------------------------------
    // A flat sorted set: elements kept in ascending order in contiguous storage, with
    // binary-search lookup and no duplicates (the cache-friendly flat_set pattern).
    //
    // The container logic (ordering, uniqueness, search, removal) lives once in
    // sorted_set<Storage>; the only things that differ between a growable and a
    // fixed-capacity set are the storage layout and whether an insert can fail when
    // full -- both captured by the Storage policy below. The public aliases are:
    //   sorted_array<T>            -- dynamic, heap-backed (never "full")
    //   fixed_sorted_array<T, N>   -- inline std::array, capacity N
    //
    // Requirements on T:
    //   * a strict-weak ordering via operator< and equality via operator== (the two may
    //     key on the same field while T carries a payload, i.e. == compares only the key;
    //     remove()'s out-parameter then returns the full stored element);
    //   * copy-constructible and copy-assignable (insert/remove/pop_back copy values);
    //   * fixed_sorted_array<T, N> additionally requires T to be default-constructible
    //     (the inline std::array value-initialises all N slots).
    // For resource-owning T (e.g. shared_ptr), fixed_sorted_array releases vacated slots
    // promptly (resets them to T{}) so erased/cleared elements don't linger.
    // ------------------------------------------------------------------------------
    namespace detail {
        // Storage policy: dynamic, heap-backed (std::vector). Never full.
        template <typename T>
        class dynamic_array_storage {
                std::vector<T> m_data;

            public:
                using value_type = T;
                using const_iterator = typename std::vector<T>::const_iterator;

                dynamic_array_storage() = default;

                // Pre-reserve storage for `capacity` elements (adds none).
                explicit dynamic_array_storage(std::size_t capacity) {
                    m_data.reserve(capacity);
                }

                // Fill with the half-open integral range [min_val, max_val) in order.
                // Requires an incrementable, ordered T; an empty/inverted range is empty.
                dynamic_array_storage(const T& min_val, const T& max_val) {
                    for (T i = min_val; i < max_val; ++i) {
                        m_data.emplace_back(i);
                    }
                }

                [[nodiscard]] std::size_t size() const { return m_data.size(); }
                [[nodiscard]] static constexpr std::size_t capacity() {
                    return std::numeric_limits<std::size_t>::max();
                }
                [[nodiscard]] bool full() const { return false; }

                [[nodiscard]] const T& operator[](std::size_t i) const { return m_data[i]; }
                [[nodiscard]] T& operator[](std::size_t i) { return m_data[i]; }

                [[nodiscard]] const_iterator begin() const { return m_data.begin(); }
                [[nodiscard]] const_iterator end() const { return m_data.end(); }

                // Insert `v` at `pos` (vector handles the shift, optimally). Always succeeds.
                bool insert_at(std::size_t pos, const T& v) {
                    m_data.insert(m_data.begin() + static_cast<std::ptrdiff_t>(pos), v);
                    return true;
                }

                void erase_at(std::size_t pos) {
                    m_data.erase(m_data.begin() + static_cast<std::ptrdiff_t>(pos));
                }

                void clear() { m_data.clear(); }
                void reserve(std::size_t n) { m_data.reserve(n); }
        };

        // Storage policy: fixed-capacity, inline std::array (no heap). Insert fails when full.
        template <typename T, std::size_t N>
        class fixed_array_storage {
                static_assert(std::is_default_constructible_v<T>,
                              "fixed_sorted_array requires a default-constructible element type");

                std::array<T, N> m_data{};
                std::size_t m_size = 0;

            public:
                using value_type = T;
                using const_iterator = typename std::array<T, N>::const_iterator;

                [[nodiscard]] std::size_t size() const { return m_size; }
                [[nodiscard]] static constexpr std::size_t capacity() { return N; }
                [[nodiscard]] bool full() const { return m_size == N; }

                [[nodiscard]] const T& operator[](std::size_t i) const { return m_data[i]; }
                [[nodiscard]] T& operator[](std::size_t i) { return m_data[i]; }

                [[nodiscard]] const_iterator begin() const { return m_data.begin(); }
                [[nodiscard]] const_iterator end() const {
                    return m_data.begin() + static_cast<std::ptrdiff_t>(m_size);
                }

                // Insert `v` at `pos`, shifting the suffix right by one. Returns false (no
                // change) when the set is already full. move_backward lowers to a memmove
                // for trivially-copyable T and move-assigns otherwise.
                bool insert_at(std::size_t pos, const T& v) {
                    if (m_size == N) {
                        return false;
                    }
                    const auto first = m_data.begin() + static_cast<std::ptrdiff_t>(pos);
                    const auto last = m_data.begin() + static_cast<std::ptrdiff_t>(m_size);
                    std::move_backward(first, last, last + 1);
                    m_data[pos] = v;
                    ++m_size;
                    return true;
                }

                // Shift the tail left by one over the hole at `pos`, then release the now
                // -vacated trailing slot so a resource-owning T isn't retained in dead
                // storage (a no-op for trivially-destructible T).
                void erase_at(std::size_t pos) {
                    const auto hole = m_data.begin() + static_cast<std::ptrdiff_t>(pos);
                    const auto last = m_data.begin() + static_cast<std::ptrdiff_t>(m_size);
                    std::move(hole + 1, last, hole);
                    --m_size;
                    if constexpr (!std::is_trivially_destructible_v<T>) {
                        m_data[m_size] = T{};
                    }
                }

                void clear() {
                    if constexpr (!std::is_trivially_destructible_v<T>) {
                        for (std::size_t i = 0; i < m_size; ++i) {
                            m_data[i] = T{}; // release resources held in the live slots
                        }
                    }
                    m_size = 0;
                }
        };
    } // namespace detail

    template <class Storage>
    class sorted_set {
            Storage m_storage;

        public:
            using value_type = typename Storage::value_type;
            using const_iterator = typename Storage::const_iterator;

            static_assert(std::is_copy_constructible_v<value_type> && std::is_copy_assignable_v<value_type>,
                          "sorted_set requires a copyable element type (insert/remove/pop_back copy values)");

            sorted_set() = default;

            // Forward storage-specific construction (e.g. the dynamic capacity / [min,max)
            // range constructors). Constrained so it never hijacks copy/move and only
            // participates when the chosen Storage actually supports the arguments.
            template <class... Args>
                requires (sizeof...(Args) >= 1)
                      && std::constructible_from<Storage, Args...>
                      && (sizeof...(Args) != 1
                          || (!std::same_as<std::remove_cvref_t<Args>, sorted_set> && ...))
            explicit sorted_set(Args&&... args) : m_storage(std::forward<Args>(args)...) {}

            // Inserts `value`. Returns false (no change) if an equal element is already
            // present, or -- for a bounded storage -- if the set is full (see full()).
            bool insert(const value_type& value) {
                const std::size_t pos = lower_bound_index(value);
                if (pos < m_storage.size() && m_storage[pos] == value) {
                    return false; // duplicate
                }
                return m_storage.insert_at(pos, value); // false only when a fixed set is full
            }

            // Removes the element equal to `value`. Returns false if absent. When present
            // and `old_val` is non-null, the removed element is copied into *old_val first.
            bool remove(const value_type& value, value_type* old_val = nullptr) {
                const std::size_t pos = index_of(value);
                if (pos == size()) {
                    return false;
                }
                if (old_val) {
                    *old_val = m_storage[pos];
                }
                m_storage.erase_at(pos);
                return true;
            }

            // Removes the element at `idx`. Precondition: idx < size().
            void remove_by_index(std::size_t idx) {
                ENFORCE(idx < size());
                m_storage.erase_at(idx);
            }

            // Removes and returns the largest element. Precondition: !empty().
            value_type pop_back() {
                ENFORCE(!empty());
                const std::size_t last = size() - 1;
                value_type out = m_storage[last];
                m_storage.erase_at(last);
                return out;
            }

            void clear() { m_storage.clear(); }

            [[nodiscard]] std::size_t size() const { return m_storage.size(); }
            [[nodiscard]] bool empty() const { return size() == 0; }
            [[nodiscard]] bool full() const { return m_storage.full(); }
            [[nodiscard]] static constexpr std::size_t capacity() { return Storage::capacity(); }

            [[nodiscard]] bool exists(const value_type& value) const {
                return std::binary_search(begin(), end(), value);
            }

            // Index of the element equal to `value`, or size() if absent.
            [[nodiscard]] std::size_t index_of(const value_type& value) const {
                const std::size_t pos = lower_bound_index(value);
                return (pos < size() && m_storage[pos] == value) ? pos : size();
            }

            [[nodiscard]] bool is_valid_index(std::size_t index) const { return index < size(); }

            // Element access. Precondition: index < size(). The non-const overload allows
            // mutating a keyed element's payload -- do NOT change the ordering key.
            [[nodiscard]] const value_type& get(std::size_t index) const {
                ENFORCE(index < size());
                return m_storage[index];
            }

            [[nodiscard]] value_type& get(std::size_t index) {
                ENFORCE(index < size());
                return m_storage[index];
            }

            // Pre-reserve (only available when the storage supports it, i.e. dynamic).
            void reserve(std::size_t n)
                requires requires(Storage& s, std::size_t k) { s.reserve(k); } {
                m_storage.reserve(n);
            }

            // Read-only ordered iteration (const-only: mutating order would break it).
            [[nodiscard]] const_iterator begin() const { return m_storage.begin(); }
            [[nodiscard]] const_iterator end() const { return m_storage.end(); }
            [[nodiscard]] const_iterator cbegin() const { return begin(); }
            [[nodiscard]] const_iterator cend() const { return end(); }

        private:
            [[nodiscard]] std::size_t lower_bound_index(const value_type& value) const {
                return static_cast<std::size_t>(std::lower_bound(begin(), end(), value) - begin());
            }
    };

    template <typename T>
    using sorted_array = sorted_set<detail::dynamic_array_storage<T>>;

    template <typename T, std::size_t N>
    using fixed_sorted_array = sorted_set<detail::fixed_array_storage<T, N>>;
}
