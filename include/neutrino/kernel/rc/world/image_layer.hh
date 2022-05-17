//
// Created by igor on 08/05/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_RC_WORLD_IMAGE_LAYER_HH
#define INCLUDE_NEUTRINO_KERNEL_RC_WORLD_IMAGE_LAYER_HH

#include <neutrino/kernel/rc/types.hh>
#include <neutrino/kernel/rc/tile_handle.hh>

namespace neutrino::kernel {
  class image_layer {
    public:
      explicit image_layer(tile_handle tid);

      [[nodiscard]] tile_handle tile_id() const noexcept;
    private:
      tile_handle m_image;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_RC_WORLD_IMAGE_LAYER_HH
