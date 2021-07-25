//
// Created by igor on 20/07/2021.
//

#ifndef NEUTRINO_TERRAIN_HH
#define NEUTRINO_TERRAIN_HH

#include "component.hh"

namespace neutrino::tiled::tmx {
    /**
   * @brief A terrain describes a kind of terrain on the map.
   */
    class terrain : public component {
    public:
        /**
         * @brief Terrain constructor.
         */
        terrain(const std::string& name, unsigned tile)
                : m_name(name), m_tile(tile)
        {
        }

        /**
         * @brief Get the name of the terrain.
         *
         * @returns the name of the terrain
         */
        [[nodiscard]] const std::string& name() const noexcept {
            return m_name;
        }

        /**
         * @brief Get the representing tile for the terrain.
         *
         * @returns the representing tile
         */
        [[nodiscard]] unsigned tile() const noexcept {
            return m_tile;
        }

    private:
        const std::string m_name;
        const unsigned m_tile;
    };

}

#endif
