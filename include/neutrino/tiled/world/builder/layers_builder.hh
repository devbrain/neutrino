//
// Created by igor on 20/09/2021.
//

#ifndef INCLUDE_NEUTRINO_TILED_WORLD_LAYERS_BUILDER_HH
#define INCLUDE_NEUTRINO_TILED_WORLD_LAYERS_BUILDER_HH

#include <neutrino/tiled/world/layer.hh>

namespace neutrino::tiled {
  class layers_builder {
    public:
      layers_builder(unsigned int width, unsigned int height);
      layers_builder& add_tile(unsigned int x, unsigned int y, texture_id_t texture_id, tile_id_t tile_id, flip_t flip);

      [[nodiscard]] tiles_layer build() const;
    private:
      unsigned int m_width;
      unsigned int m_height;
      // x,y,tile_description
      using tile_info = std::tuple<unsigned int, unsigned int, tile_description>;
      std::vector<tile_info> m_tiles;
  };
}

#endif //INCLUDE_NEUTRINO_TILED_WORLD_LAYERS_BUILDER_HH
