//
// Created by igor on 26/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_TILES_WORLD_WORLD_ASSETS_HH
#define INCLUDE_NEUTRINO_ASSETS_TILES_WORLD_WORLD_ASSETS_HH

#include <neutrino/assets/tiles/image_atlas.hh>
#include <neutrino/assets/tiles/animation_description.hh>

namespace neutrino::assets {
  struct world_assets {
    image_atlas images;
    animation_description animation_sequences;
  };
}

#endif //INCLUDE_NEUTRINO_ASSETS_TILES_WORLD_WORLD_ASSETS_HH
