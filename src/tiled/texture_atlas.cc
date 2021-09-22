//
// Created by igor on 28/08/2021.
//

#include <neutrino/tiled/texture_atlas.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::tiled {

  void texture_atlas::add (hal::texture&& texture, sprite_coords_t&& coords) {
    m_atlas.emplace_back (std::move(texture), std::move(coords));
  }

  void texture_atlas::add (hal::texture&& texture, int width, int height) {
    sprite_coords_t coords;
    coords.emplace_back(0, 0, width, height);

    return add(std::move (texture), std::move (coords));
  }

  std::tuple<const hal::texture&, math::rect> texture_atlas::get(texture_id_t id, tile_id_t tile_id) const {
    if (id.value_of() >= m_atlas.size()) {
      RAISE_EX("No texture with id", id.value_of(), "found");
    }
    const auto& x = m_atlas[id.value_of()];

    if (tile_id.value_of() >= std::get<1>(x).size()) {
      RAISE_EX("No tile with id", tile_id.value_of(), "found in the texture", id.value_of());
    }
    return {std::get<0>(x), std::get<1>(x)[tile_id.value_of()]};
  }
}
