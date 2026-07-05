//
// Created by igor on 04/07/2026.
//

#pragma once

#include <vector>
#include <cstdint>
#include <neutrino/neutrino_export.h>

namespace neutrino {
    /**
     * @brief A CPU-resident bitmask representing solid vs transparent pixels of a sprite frame.
     *
     * Used for per-pixel collision detection.
     */
    class NEUTRINO_EXPORT bitmask {
        public:
            bitmask() = default;

            /**
             * @brief Construct an empty bitmask with specified dimensions.
             */
            bitmask(int width, int height);

            [[nodiscard]] int width() const noexcept { return m_width; }
            [[nodiscard]] int height() const noexcept { return m_height; }

            /**
             * @brief Set the collision bit at coordinates (x, y).
             */
            void set(int x, int y, bool val);

            /**
             * @brief Get the collision bit at coordinates (x, y).
             */
            [[nodiscard]] bool get(int x, int y) const noexcept;

            /**
             * @brief Checks if this bitmask overlaps/collides with another bitmask.
             * @param other The other bitmask to test against.
             * @param dx Relative X offset of the other bitmask.
             * @param dy Relative Y offset of the other bitmask.
             * @return true if solid pixels overlap, false otherwise.
             */
            [[nodiscard]] bool overlaps(const bitmask& other, int dx, int dy) const noexcept;

        private:
            int m_width{0};
            int m_height{0};
            std::vector<uint8_t> m_bits;
    };
}