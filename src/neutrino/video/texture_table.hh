//
// Created by igor on 15/11/2026.
//

#pragma once

#include <limits>
#include <optional>
#include <vector>
#include <memory>

#include <neutrino/video/texture.hh>
#include "video/texture_spec.hh"

namespace neutrino::detail {
    class texture_allocator;
    /// @brief Test-only access to a table of its own. Defined only in the test TU.
    struct texture_table_test_access;

    /**
     * @brief The texture allocator's record of one live or failed handle (painter.md §3.3).
     */
    struct texture_slot {
        /// @brief What the handle asked for. The size follows texture_allocator::resize().
        texture_spec           spec;
        /// @brief The held texture; empty for a failed handle.
        std::optional<texture> tex;
        /// @brief See texture_handle::needs_redraw().
        bool                   needs_redraw{true};
    };

    /**
     * @brief The texture allocator's slots, one per live or failed handle, addressed by index.
     *
     * The table holds the textures in use; the allocator's pool holds the ones given back. A
     * freed index is reused, the most recently freed first. Only texture_allocator makes a
     * table. get() and remove() enforce an occupied index; is_valid() asks without enforcing.
     */
    class texture_table {
        friend class texture_allocator;
        friend struct texture_table_test_access;
        public:
            /// @brief The index of no slot: what an empty handle holds.
            static constexpr std::size_t INVALID_SLOT = std::numeric_limits<std::size_t>::max();
        public:
            texture_table(const texture_table&) = delete;
            texture_table& operator=(const texture_table&) = delete;
            texture_table(texture_table&&) = delete;
            texture_table& operator=(texture_table&&) = delete;

            /// @brief Store @p slot; returns its index.
            std::size_t add(texture_slot&& slot);
            /// @brief Free the slot at @p idx, destroying what it holds. ENFORCE: occupied.
            void remove(std::size_t idx);


            /// @brief The slot at @p idx. ENFORCE: occupied.
            [[nodiscard]] const texture_slot& get(std::size_t idx) const;
            /// @brief The slot at @p idx. ENFORCE: occupied.
            [[nodiscard]] texture_slot& get(std::size_t idx);
            /// @brief True if @p idx is an occupied slot; false for INVALID_SLOT.
            [[nodiscard]] bool is_valid(std::size_t idx) const noexcept;

            /// @brief Calls @p fn on every occupied slot, in slot order.
            template<typename Fn>
            void for_each(Fn&& fn) {
                for (auto& slot : m_table) {
                    if (slot) {
                        fn(*slot);
                    }
                }
            }

        private:
            texture_table() = default;
            ~texture_table() = default;

            /// True if the slot at @p idx is free. ENFORCE: in range.
            [[nodiscard]] bool is_free(std::size_t idx) const;
            /// Makes @p idx the next index add() reuses.
            void mark_free(std::size_t idx);
            /// The index add() fills: the most recently freed one, or a new one at the end.
            std::size_t get_free();
        private:
            /// One entry per index; null when free.
            std::vector<std::unique_ptr<texture_slot>> m_table;
            /// Freed indices, the most recent last.
            std::vector<std::size_t> m_free_list;
    };
}
