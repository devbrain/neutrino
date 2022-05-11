//
// Created by igor on 08/05/2022.
//

#include <neutrino/kernel/rc/world/image_layer.hh>

namespace neutrino::kernel {
  image_layer::image_layer(tile_id_t tid)
  : m_image (tid) {}

  tile_id_t image_layer::tile_id() const noexcept {
    return m_image;
  }
}
