//
// Created by igor on 26/05/2022.
//

#include <neutrino/assets/tiles/image_atlas.hh>

namespace neutrino::assets {

  atlas_id_t image_atlas::add(color bgcolor, [[maybe_unused]] const std::string& name) {
    auto v = atlas_id_t (m_map.size());
    m_map.insert (std::make_pair(v, bgcolor));
    return v;
  }

  image_atlas::iterator_t image_atlas::begin() const {
    return m_map.begin();
  }

  image_atlas::iterator_t image_atlas::end() const {
    return m_map.end();
  }
}
