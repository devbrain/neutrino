//
// Created by igor on 19/04/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_GFX_TILESHEET_INFO_HH
#define INCLUDE_NEUTRINO_KERNEL_GFX_TILESHEET_INFO_HH

#include <cstddef>
#include <functional>


namespace neutrino::gfx {
  class tile_sheet_info {
    public:
      tile_sheet_info (unsigned tile_width,
                       unsigned tile_height,
                       unsigned spacing,
                       unsigned margin,
                       unsigned offset_x,
                       unsigned offset_y,
                       std::size_t num_tiles);


      [[nodiscard]] unsigned tile_width () const noexcept;
      [[nodiscard]] unsigned tile_height () const noexcept;
      [[nodiscard]] unsigned spacing () const noexcept;
      [[nodiscard]] unsigned margin () const noexcept;
      [[nodiscard]] unsigned offset_x () const noexcept;
      [[nodiscard]] unsigned offset_y () const noexcept;
      [[nodiscard]] std::size_t num_tiles () const noexcept;
    private:
      unsigned m_tile_width;
      unsigned m_tile_height;
      unsigned m_spacing; // The spacing in pixels between the tiles in this tileset
      unsigned m_margin;  // The margin around the tiles in this tileset
      unsigned m_offset_x;
      unsigned m_offset_y;
      std::size_t m_num_tiles;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_GFX_TILESHEET_INFO_HH
