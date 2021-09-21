//
// Created by igor on 06/07/2021.
//

#ifndef NEUTRINO_TILED_WORLD_TILE_DESCRIPTION_HH
#define NEUTRINO_TILED_WORLD_TILE_DESCRIPTION_HH

#include <cstdint>
#include <vector>
#include <optional>
#include <neutrino/tiled/world/types.hh>

namespace neutrino::tiled {
  enum class flip_t {
      NONE,
      HORIZONTAL,
      VERTICAL
  };

  struct tile_description {
    tile_description(texture_id_t atexture_id, tile_id_t atile_id, flip_t aflip)
    : texture_id(atexture_id), tile_id(atile_id), flip(aflip) {}

    texture_id_t    texture_id;
    tile_id_t       tile_id;
    flip_t          flip;
  };

  using tile_t = std::optional<tile_description>;
}

#endif
