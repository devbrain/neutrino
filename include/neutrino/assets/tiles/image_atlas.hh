//
// Created by igor on 26/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_TILES_IMAGE_ATLAS_HH
#define INCLUDE_NEUTRINO_ASSETS_TILES_IMAGE_ATLAS_HH

#include <map>
#include <variant>

#include <neutrino/assets/tiles/tilesheet_description.hh>
#include <neutrino/assets/tiles/types.hh>
#include <neutrino/assets/tiles/tile_handle.hh>
#include <neutrino/assets/tiles/world/tilesheet_resource.hh>

namespace neutrino::assets {
  class image_atlas {
    public:
      using entry_t = std::variant<resource<tilesheet_resource>, color>;
      using map_t = std::map<atlas_id_t, entry_t>;
      using iterator_t = map_t::const_iterator;
    public:

      [[nodiscard]] atlas_id_t add(color bgcolor, const std::string& name);

      /*
       * T is one of tilesheet_resource_reader materials
       */
      template <class T>
      [[nodiscard]] atlas_id_t add(T ts, const std::string& name) {
        auto v = atlas_id_t (m_map.size());
        tilesheet_resource_reader ldr;
        auto x = ldr.load (ts, name);
        m_map.insert (std::make_pair(v, std::move(x)));
        return v;
      }

      [[nodiscard]] iterator_t begin() const;
      [[nodiscard]] iterator_t end() const;
    private:
      map_t m_map;
  };
}

#endif //INCLUDE_NEUTRINO_ASSETS_TILES_IMAGE_ATLAS_HH
