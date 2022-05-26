//
// Created by igor on 26/05/2022.
//

#include <neutrino/assets/tiles/image_atlas.hh>

namespace neutrino::assets {
  atlas_id_t image_atlas::add(lazy_tilesheet ts) {
    auto v = atlas_id_t (m_map.size());
    m_map.insert (std::make_pair(v, std::move(ts)));
    return v;
  }

  atlas_id_t image_atlas::add(image loader) {
    auto v = atlas_id_t (m_map.size());
    m_map.insert (std::make_pair(v, std::move(loader)));
    return v;
  }

  atlas_id_t image_atlas::add(color bgcolor) {
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
