//
// Created by igor on 16/05/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_GFX_GRID_HH
#define INCLUDE_NEUTRINO_KERNEL_GFX_GRID_HH

#include <neutrino/math/rect.hh>
#include "neutrino/assets/tiles/tile_handle.hh"

namespace neutrino::assets {
  class world;
}

namespace neutrino::kernel {

  class world_window;

  class grid {
    public:
      grid();
      explicit grid (const assets::world* w);
      grid(unsigned tile_width, unsigned tile_height, unsigned width_in_tiles, unsigned height_in_tiles);

      void evaluate (int world_pos_x, int world_pos_y, int window_width, int window_height);
      void evaluate (const world_window& w);

      [[nodiscard]] int top_left_tile_x() const;
      [[nodiscard]] int top_left_tile_y() const;
      [[nodiscard]] int bottom_right_tile_x() const;
      [[nodiscard]] int bottom_right_tile_y() const;

      [[nodiscard]] int up_pixels_start() const;
      [[nodiscard]] int left_pixels_start() const;
      [[nodiscard]] int bottom_pixels_end() const;
      [[nodiscard]] int right_pixels_end() const;

      [[nodiscard]] int tile_width() const;
      [[nodiscard]] int tile_height() const;

      void adjust(int tx, int ty, math::rect& r) const;
      [[nodiscard]] math::rect empty(int tx, int ty) const;
      [[nodiscard]] static math::dimension2di_t eval_transormed_dims(const math::dimension2di_t& orig,
                                                                     const assets::rotation_info& ri);
    private:
      int m_tile_width;
      int m_tile_height;
      int m_world_width;
      int m_world_height;

      int m_top_left_tile_x;
      int m_top_left_tile_y;
      int m_bottom_right_tile_x;
      int m_bottom_right_tile_y;

      int m_up_pixels_start;
      int m_left_pixels_start;
      int m_bottom_pixels_end;
      int m_right_pixels_end;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_GFX_GRID_HH
