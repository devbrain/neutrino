//
// Created by igor on 20/09/2021.
//

#include <neutrino/tiled/world/layer.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::tiled {
  tiles_layer::tiles_layer(unsigned int width,unsigned  int height)
  : m_width(width), m_height(height), m_tiles(width*height) {
  }

  const tile_t& tiles_layer::at(unsigned int x, unsigned int y) const {
    try {
      return m_tiles[get_index (x, y)];
    } catch (exception& e) {
      RAISE_EX_WITH_CAUSE(std::move(e), "Failed to get tile");
    }
  }

  tile_t& tiles_layer::at(unsigned int x, unsigned int y) {
    try {
      return m_tiles[get_index (x, y)];
    }catch (exception& e) {
      RAISE_EX_WITH_CAUSE(std::move(e), "Failed to set tile");
    }
  }

  unsigned int tiles_layer::width() const noexcept {
    return m_width;
  }

  unsigned int tiles_layer::height() const noexcept {
    return m_height;
  }

  std::size_t tiles_layer::get_index(unsigned int x, unsigned int y) const {
    if (x > m_width) {
      RAISE_EX("x coordinate is out of the layer bounds ", x , ">", m_width);
    }
    if (y > m_height) {
      RAISE_EX("y coordinate is out of the layer bounds ", y , ">", m_height);
    }
    return m_width*y + x;
  }
}