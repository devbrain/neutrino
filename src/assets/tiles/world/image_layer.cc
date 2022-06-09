//
// Created by igor on 08/05/2022.
//

#include <neutrino/assets/tiles/world/image_layer.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::assets {
  image_layer::image_layer(tile_handle tid)
  : m_image (tid) {
    ENFORCE(tid); //-V1044
  }

  tile_handle image_layer::tile_id() const noexcept {
    return m_image;
  }
}
