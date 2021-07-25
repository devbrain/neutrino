//
// Created by igor on 20/07/2021.
//

#include "tile_set.hh"
#include <neutrino/utils/exception.hh>

namespace neutrino::tiled::tmx {
    const tile* tile_set::get_tile(unsigned id) const noexcept {
        for (auto& tile : m_tiles) {
            if (tile->id() == id) {
                return tile.get();
            }
        }
        return nullptr;
    }

    math::rect tile_set::get_coords(unsigned id, math::dimension_t size) const noexcept {
        unsigned width = (size[0] - 2 * m_margin + m_spacing) / (m_tilewidth + m_spacing); // number of tiles
        unsigned height = (size[1] - 2 * m_margin + m_spacing) / (m_tileheight + m_spacing); // number of tiles

        unsigned tu = id % width;
        unsigned tv = id / width;
        ENFORCE(tv < height);

        unsigned du = m_margin + tu * m_spacing + m_x;
        unsigned dv = m_margin + tv * m_spacing + m_y;
        ENFORCE((int)((tu + 1) * m_tilewidth + du) <= size[0]);
        ENFORCE((int)((tv + 1) * m_tileheight + dv) <= size[1]);

        return { (int)(tu * m_tilewidth + du), (int)(tv * m_tileheight + dv), (int)m_tilewidth, (int)m_tileheight };
    }
}