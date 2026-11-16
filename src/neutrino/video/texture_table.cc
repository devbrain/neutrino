//
// Created by igor on 15/11/2026.
//

#include "texture_table.hh"
#include <failsafe/enforce.hh>

namespace neutrino::detail {
    std::size_t texture_table::add(texture_slot&& slot) {
        const auto n = get_free();
        auto p = std::make_unique <texture_slot>(std::move(slot));
        if (n == m_table.size()) {
            m_table.push_back(std::move(p));
        } else {
            m_table[n] = std::move(p);
        }
        return n;
    }

    const texture_slot& texture_table::get(std::size_t idx) const {
        ENFORCE(idx < m_table.size(), "texture slot", idx, "is out of range (", m_table.size(), ")");
        ENFORCE(!is_free(idx), "Attempt to get free texture at slot", idx);
        return *m_table[idx];
    }

    texture_slot& texture_table::get(std::size_t idx) {
        ENFORCE(idx < m_table.size(), "texture slot", idx, "is out of range (", m_table.size(), ")");
        ENFORCE(!is_free(idx), "Attempt to get free texture at slot", idx);
        return *m_table[idx];
    }

    bool texture_table::is_valid(std::size_t idx) const noexcept {
        if (idx >= m_table.size() || idx == INVALID_SLOT) {
            return false;
        }
        return !is_free(idx);
    }

    void texture_table::remove(std::size_t idx) {
        ENFORCE(idx < m_table.size(), "texture slot", idx, "is out of range (", m_table.size(), ")");
        ENFORCE(m_table[idx], "Attempt to double free texture slot at", idx);
        m_table[idx] = nullptr;
        mark_free(idx);
    }

    bool texture_table::is_free(std::size_t idx) const {
        ENFORCE(idx < m_table.size(), "texture slot", idx, "is out of range (", m_table.size(), ")");
        return !m_table[idx];
    }

    void texture_table::mark_free(std::size_t idx) {
        m_free_list.emplace_back(idx);
    }

    std::size_t texture_table::get_free() {
        if (m_free_list.empty()) {
            return m_table.size();
        }
        auto n = m_free_list.back();
        m_free_list.pop_back();
        return n;
    }
}
