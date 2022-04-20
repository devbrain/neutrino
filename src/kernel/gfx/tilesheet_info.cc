//
// Created by igor on 19/04/2022.
//

#include <neutrino/kernel/gfx/tilesheet_info.hh>

namespace neutrino::gfx {
  tile_sheet_info::tile_sheet_info (unsigned tile_width,
                                    unsigned tile_height,
                                    unsigned spacing,
                                    unsigned margin,
                                    unsigned offset_x,
                                    unsigned offset_y,
                                    std::size_t num_tiles)
  : m_tile_width{tile_width}, m_tile_height{tile_height}, m_spacing{spacing},
    m_margin{margin}, m_offset_x(offset_x), m_offset_y(offset_y), m_num_tiles(num_tiles) {
  }

  unsigned tile_sheet_info::tile_width () const noexcept {
    return m_tile_width;
  }

  unsigned tile_sheet_info::tile_height () const noexcept {
    return m_tile_height;
  }

  unsigned tile_sheet_info::spacing () const noexcept {
    return m_spacing;
  }

  unsigned tile_sheet_info::margin () const noexcept {
    return m_margin;
  }

  unsigned int tile_sheet_info::offset_x () const noexcept {
    return m_offset_x;
  }

  unsigned int tile_sheet_info::offset_y () const noexcept {
    return m_offset_y;
  }

  size_t tile_sheet_info::num_tiles () const noexcept {
    return m_num_tiles;
  }
}