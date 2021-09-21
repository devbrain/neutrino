//
// Created by igor on 21/09/2021.
//

#ifndef INCLUDE_NEUTRINO_TILED_WORLD_WORLD_BUILDER_HH
#define INCLUDE_NEUTRINO_TILED_WORLD_WORLD_BUILDER_HH

#include <neutrino/tiled/world/world.hh>

namespace neutrino::tiled {
  class world_builder {
    public:
      world_builder& add(tiles_layer&& alayer);
      world build();
    private:
      std::vector<tiles_layer> m_layers;
  };
}

#endif //INCLUDE_NEUTRINO_TILED_WORLD_WORLD_BUILDER_HH
