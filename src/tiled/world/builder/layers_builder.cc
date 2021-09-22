//
// Created by igor on 20/09/2021.
//

#include <neutrino/tiled/world/builder/layers_builder.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::tiled {
  layers_builder::layers_builder(unsigned int width, unsigned int height)
  : m_width(width), m_height(height) {}

  layers_builder& layers_builder::add_tile(unsigned int x, unsigned int y, texture_id_t texture_id, tile_id_t tile_id, flip_t flip) {
    if (x >= m_width) {
      RAISE_EX("X coordinate is out of range", x, ">", m_width);
    }
    if (y >= m_height) {
      RAISE_EX("Y coordinate is out of range", y, ">", m_height);
    }
    m_tiles.emplace_back (x, y, tile_description(texture_id, tile_id, flip));
    return *this;
  }

  tiles_layer layers_builder::build() const {
    if (m_tiles.empty()) {
      RAISE_EX("Can not create empty layer");
    }
    tiles_layer res(m_width, m_height);

    for (const auto &[x,y,td] : m_tiles) {
      res.at (x,y) = td;
    }

    return res;
  }
}