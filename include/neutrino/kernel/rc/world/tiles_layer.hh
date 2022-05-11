//
// Created by igor on 08/05/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_RC_WORLD_TILES_LAYER_HH
#define INCLUDE_NEUTRINO_KERNEL_RC_WORLD_TILES_LAYER_HH

#include <vector>
#include <neutrino/kernel/rc/types.hh>

namespace neutrino::kernel {
  class tiles_layer {
    public:
      tiles_layer (std::size_t width, std::size_t height);

      void set(std::size_t x, std::size_t y, atlas_id_t atlas_id, cell_id_t cell_id);
      void set_empty(std::size_t x, std::size_t y);

      [[nodiscard]] tile_id_t get(std::size_t x, std::size_t y) const;
      [[nodiscard]] std::size_t width() const noexcept;
      [[nodiscard]] std::size_t height() const noexcept;
    private:
      union tile_rep {
         uint16_t x[2];
         uint32_t id;
         tile_rep()
         : id(std::numeric_limits<uint32_t>::max()) {}
      };
      std::size_t m_width;
      std::size_t m_height;
      std::vector<tile_rep> m_layer;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_RC_WORLD_TILES_LAYER_HH
