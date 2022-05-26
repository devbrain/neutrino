//
// Created by igor on 08/05/2022.
//

#include <neutrino/assets/tiles/world/tiles_layer.hh>

namespace neutrino::assets {
  tiles_layer::tiles_layer (std::size_t width, std::size_t height)
  : m_width(width),
    m_height(height) {
    m_layer.resize (width*height);
  }

  void tiles_layer::set(std::size_t x, std::size_t y, tile_handle th) {
    m_layer[m_width*y + x] = th;
  }

  void tiles_layer::set(std::size_t x, std::size_t y, atlas_id_t atlas_id, cell_id_t cell_id) {
    set(x, y ,tile_handle(atlas_id, cell_id));
  }

  void tiles_layer::set_empty(std::size_t x, std::size_t y) {
    m_layer[m_width*y + x] = {};
  }

  tile_handle tiles_layer::get(std::size_t x, std::size_t y) const {
    return m_layer[m_width*y + x];
  }

  std::size_t tiles_layer::width() const noexcept {
    return m_width;
  }

  std::size_t tiles_layer::height() const noexcept {
    return m_height;
  }
}