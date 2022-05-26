//
// Created by igor on 08/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_TILES_WORLD_TILES_LAYER_HH
#define INCLUDE_NEUTRINO_ASSETS_TILES_WORLD_TILES_LAYER_HH

#include <vector>
#include <neutrino/assets/tiles/types.hh>
#include <neutrino/assets/tiles/tile_handle.hh>

namespace neutrino::assets {
  class tiles_layer {
    public:
      tiles_layer (std::size_t width, std::size_t height);

      void set(std::size_t x, std::size_t y, atlas_id_t atlas_id, cell_id_t cell_id);
      void set(std::size_t x, std::size_t y, tile_handle th);
      void set_empty(std::size_t x, std::size_t y);

      [[nodiscard]] tile_handle get(std::size_t x, std::size_t y) const;
      [[nodiscard]] std::size_t width() const noexcept;
      [[nodiscard]] std::size_t height() const noexcept;
    private:
      std::size_t m_width;
      std::size_t m_height;
      std::vector<tile_handle> m_layer;
  };
}

#endif //INCLUDE_NEUTRINO_ASSETS_TILES_WORLD_TILES_LAYER_HH
