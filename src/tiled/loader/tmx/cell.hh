//
// Created by igor on 19/07/2021.
//

#ifndef NEUTRINO_CELL_HH
#define NEUTRINO_CELL_HH

#include <bitflags/bitflags.hpp>

namespace neutrino::tiled::tmx
{
    /**
     * @brief A cell is a square on a map layer that is filled with a tile.
     */
    class cell
    {
    public:

        static cell decode_gid(unsigned int gid);

        explicit cell(unsigned gid, bool hflip = false, bool vflip = false, bool dflip = false)
                : m_gid(gid), m_hflip(hflip), m_vflip(vflip), m_dflip(dflip)
        {
        }
        /**
     * @brief Get the global id of the tile.
     *
     * @returns the global id of the tile.
     */
        [[nodiscard]] unsigned gid() const noexcept
        {
            return m_gid;
        }

        /**
         * @brief Tell whether the tile must be flipped horizontally.
         *
         * @returns true if the tile must be flipped horizontally
         */
        [[nodiscard]] bool hor_flipped() const noexcept
        {
            return m_hflip;
        }

        /**
         * @brief Tell whether the tile must be flipped vertically.
         *
         * @returns true if the tile must be flipped vertically
         */
        [[nodiscard]] bool vert_flipped() const noexcept
        {
            return m_vflip;
        }

        /**
         * @brief Tell whether the tile must be flipped diagonally.
         *
         * @returns true if the tile must be flipped diagonally
         */
        [[nodiscard]] bool diag_flipped() const noexcept
        {
            return m_dflip;
        }

    private:
        unsigned m_gid;
        bool m_hflip;
        bool m_vflip;
        bool m_dflip;
    };
}

#endif
