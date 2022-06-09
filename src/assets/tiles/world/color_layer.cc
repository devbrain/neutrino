//
// Created by igor on 08/05/2022.
//

#include <neutrino/assets/tiles/world/color_layer.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::assets {
  color_layer::color_layer(tile_handle tid)
  : m_color (tid) {
    ENFORCE(tid); //-V1044
  }

  tile_handle color_layer::tile_id() const noexcept {
    return m_color;
  }
}
