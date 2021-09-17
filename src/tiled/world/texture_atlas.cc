//
// Created by igor on 28/08/2021.
//

#include <neutrino/tiled/world/texture_atlas.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::tiled {

  texture_id_t texture_atlas::add (hal::texture&& texture, sprite_coords_t&& coords) {
    auto n = texture_id_t{m_atlas.size()};
    m_atlas.emplace (n, atlas_t {std::move(texture), std::move(coords)});
    return n;
  }

  texture_id_t texture_atlas::add (hal::texture&& texture, int width, int height) {
    sprite_coords_t coords;
    coords.emplace_back(0, 0, width, height);

    return add(std::move (texture), std::move (coords));
  }

  hal::texture& texture_atlas::get (texture_id_t id) {
    auto i = m_atlas.find (id);
    if (i == m_atlas.end()) {
      RAISE_EX("No texture with id", id.value_of(), "found");
    }
    return std::get<0>(i->second);
  }

  const hal::texture& texture_atlas::get (texture_id_t id) const {
    auto i = m_atlas.find (id);
    if (i == m_atlas.end()) {
      RAISE_EX("No texture with id", id.value_of(), "found");
    }
    return std::get<0>(i->second);
  }

  bool texture_atlas::exists (texture_id_t id) const noexcept {
    return m_atlas.find (id) != m_atlas.end();
  }

  void texture_atlas::remove (texture_id_t id) {
    auto i = m_atlas.find (id);
    if (i != m_atlas.end()) {
      m_atlas.erase (i);
    }
  }
}