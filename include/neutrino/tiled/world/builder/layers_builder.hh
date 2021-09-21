//
// Created by igor on 20/09/2021.
//

#ifndef INCLUDE_NEUTRINO_TILED_WORLD_LAYERS_BUILDER_HH
#define INCLUDE_NEUTRINO_TILED_WORLD_LAYERS_BUILDER_HH

#include <neutrino/tiled/world/layer.hh>

namespace neutrino::tiled {
  class layers_builder {
    public:
      layers_builder(int width, int height);
      layers_builder& add_tile(int x, int y, texture_id_t texture_id, tile_id_t tile_id, flip_t flip);

      tiles_layer build() const;
    private:
      int m_width;
      int m_height;
      // x,y,tile_description
      using tile_info = std::tuple<int, int, tile_description>;
      std::vector<tile_info> m_tiles;
  };
}

#endif //INCLUDE_NEUTRINO_TILED_WORLD_LAYERS_BUILDER_HH
