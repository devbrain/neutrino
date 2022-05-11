//
// Created by igor on 08/05/2022.
//

#include <neutrino/kernel/rc/world/tiles_layer.hh>

namespace neutrino::kernel {
  tiles_layer::tiles_layer (std::size_t width, std::size_t height)
  : m_width(width),
    m_height(height) {
    m_layer.resize (width*height);
  }

  void tiles_layer::set(std::size_t x, std::size_t y, atlas_id_t atlas_id, cell_id_t cell_id) {
    tile_rep tl;
    if (!is_invalid (atlas_id) && !is_invalid (cell_id)) {
      tl.x[0] = static_cast<uint16_t>(cell_id.value_of());
      tl.x[1] = static_cast<uint16_t>(atlas_id.value_of());
    }
    m_layer[m_width*y + x] = tl;
  }

  void tiles_layer::set_empty(std::size_t x, std::size_t y) {
    m_layer[m_width*y + x] = tile_rep();
  }

  tile_id_t tiles_layer::get(std::size_t x, std::size_t y) const {
    auto tl = m_layer[m_width*y + x];
    if (tl.id == std::numeric_limits<uint32_t>::max()) {
      return {};
    }
    return {atlas_id_t(tl.x[1]), cell_id_t(tl.x[0])};
  }

  std::size_t tiles_layer::width() const noexcept {
    return m_width;
  }

  std::size_t tiles_layer::height() const noexcept {
    return m_height;
  }
}