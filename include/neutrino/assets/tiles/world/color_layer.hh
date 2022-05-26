//
// Created by igor on 08/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_TILES_WORLD_COLOR_LAYER_HH
#define INCLUDE_NEUTRINO_ASSETS_TILES_WORLD_COLOR_LAYER_HH

#include <neutrino/assets/tiles/types.hh>
#include <neutrino/assets/tiles/tile_handle.hh>

namespace neutrino::assets {
  class color_layer {
    public:
      explicit color_layer(tile_handle tid);

      [[nodiscard]] tile_handle tile_id() const noexcept;
    private:
      tile_handle m_color;
  };
}

#endif //INCLUDE_NEUTRINO_ASSETS_TILES_WORLD_IMAGE_LAYER_HH
