//
// Created by igor on 04/07/2026.
//

#include <neutrino/video/sprite/bitmask.hh>

namespace neutrino {
    // =========================================================================
    // bitmask Implementation
    // =========================================================================

    bitmask::bitmask(int width, int height)
        : m_width(width), m_height(height), m_bits(static_cast<std::size_t>(width * height), 0) {
    }

    void bitmask::set(int x, int y, bool val) {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
            m_bits[static_cast<std::size_t>(y * m_width + x)] = val ? 1 : 0;
        }
    }

    bool bitmask::get(int x, int y) const noexcept {
        if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
            return false;
        }
        return m_bits[static_cast<std::size_t>(y * m_width + x)] != 0;
    }

    bool bitmask::overlaps(const bitmask& other, int dx, int dy) const noexcept {
        int x1 = std::max(0, dx);
        int y1 = std::max(0, dy);
        int x2 = std::min(m_width, other.m_width + dx);
        int y2 = std::min(m_height, other.m_height + dy);

        if (x1 >= x2 || y1 >= y2) {
            return false;
        }

        const uint8_t* this_data = m_bits.data();
        const uint8_t* other_data = other.m_bits.data();

        for (int y = y1; y < y2; ++y) {
            const uint8_t* row_this = this_data + static_cast<std::size_t>(y * m_width + x1);
            const uint8_t* row_other = other_data + static_cast<std::size_t>((y - dy) * other.m_width + (x1 - dx));
            int count = x2 - x1;
            for (int i = 0; i < count; ++i) {
                if (row_this[i] && row_other[i]) {
                    return true;
                }
            }
        }
        return false;
    }

}