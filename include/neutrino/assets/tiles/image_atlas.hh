//
// Created by igor on 26/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_TILES_IMAGE_ATLAS_HH
#define INCLUDE_NEUTRINO_ASSETS_TILES_IMAGE_ATLAS_HH

#include <map>
#include <variant>

#include "neutrino/assets/tilesheet_description.hh"
#include "neutrino/assets/types.hh"
#include <neutrino/assets/tiles/tile_handle.hh>

namespace neutrino::assets {
  class image_atlas {
    public:
      // color
      // hal::surface
      using entry_t = std::variant<lazy_tilesheet, image, color, hal::surface, assets::tilesheet>;
      using map_t = std::map<atlas_id_t, entry_t>;
      using iterator_t = map_t::const_iterator;
    public:
      [[nodiscard]] atlas_id_t add(lazy_tilesheet ts);
      [[nodiscard]] atlas_id_t add(image loader);
      [[nodiscard]] atlas_id_t add(color bgcolor);
      [[nodiscard]] atlas_id_t add(hal::surface img);
      [[nodiscard]] atlas_id_t add(assets::tilesheet ts);

      [[nodiscard]] iterator_t begin() const;
      [[nodiscard]] iterator_t end() const;
    private:
      map_t m_map;
  };
}

#endif //INCLUDE_NEUTRINO_ASSETS_TILES_IMAGE_ATLAS_HH
