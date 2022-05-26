//
// Created by igor on 16/05/2022.
//

#include <neutrino/kernel/gfx/grid.hh>
#include <neutrino/kernel/gfx/world_renderer.hh>

namespace neutrino::kernel {
  grid::grid()
  : grid(0, 0, 0, 0) {
  }

  grid::grid (const assets::world* w)
      : grid(w->tile_width(), w->tile_height(), w->width(), w->height()) {
  }

  grid::grid(unsigned tile_width, unsigned tile_height,
             unsigned width_in_tiles, unsigned height_in_tiles)
      :m_tile_width(static_cast<int>(tile_width)),
       m_tile_height(static_cast<int>(tile_height)),
       m_world_width(static_cast<int>(width_in_tiles)),
       m_world_height(static_cast<int>(height_in_tiles))
  {
  }

  static int clamp(int v, int m, int M) {
    if (v < m) {
      return m;
    }
    if (v > M) {
      return M;
    }
    return v;
  }

  void grid::evaluate (const world_window& w) {
    auto wp = w.world_pos();
    auto d = w.dimensions();
    evaluate (wp[0], wp[1], d[0], d[1]);
  }

  void grid::evaluate (int world_pos_x, int world_pos_y, int window_width, int window_height) {
    auto wtx = clamp (world_pos_x, 0, m_world_width*m_tile_width - window_width);
    auto wty = clamp (world_pos_y, 0, m_world_height*m_tile_height - window_height);

    auto wbx = wtx + window_width;
    auto wby = wty + window_height;

    m_top_left_tile_x = wtx / m_tile_width;
    m_left_pixels_start = wtx % m_tile_width;

    m_top_left_tile_y = wty / m_tile_height;
    m_up_pixels_start = wty % m_tile_height;

    m_bottom_right_tile_x = wbx / m_tile_width;
    m_right_pixels_end = wbx % m_tile_width;

    if (m_right_pixels_end == 0) {
      m_bottom_right_tile_x--;
      m_right_pixels_end = m_tile_width;
    }

    m_bottom_right_tile_y = wby / m_tile_height;
    m_bottom_pixels_end = wby % m_tile_height;
    if (m_bottom_pixels_end == 0) {
      m_bottom_right_tile_y --;
      m_bottom_pixels_end = m_tile_height;
    }

  }

  void grid::adjust(int tx, int ty, math::rect& r) const {
    if (tx == m_top_left_tile_x) {
      r.point.x += m_left_pixels_start;
      r.dims.x -= m_left_pixels_start;
    } else if (tx == m_bottom_right_tile_x) {
      r.dims.x = m_right_pixels_end;
    }

    if (ty == m_top_left_tile_y) {
      r.point.y += m_up_pixels_start;
      r.dims.y -= m_up_pixels_start;
    } else if (ty == m_bottom_right_tile_y) {
      r.dims.y = m_bottom_pixels_end;
    }

  }

  math::rect grid::empty(int tx, int ty) const {
    math::rect src(tx * m_tile_width, ty * m_tile_height, m_tile_width, m_tile_height);
    adjust (tx, ty, src);
    return src;
  }

  int grid::top_left_tile_x() const {
    return m_top_left_tile_x;
  }

  int grid::top_left_tile_y() const {
    return m_top_left_tile_y;
  }

  int grid::bottom_right_tile_x() const {
    return m_bottom_right_tile_x;
  }

  int grid::bottom_right_tile_y() const {
    return m_bottom_right_tile_y;
  }

  int grid::up_pixels_start() const {
    return m_up_pixels_start;
  }

  int grid::left_pixels_start() const {
    return m_left_pixels_start;
  }

  int grid::bottom_pixels_end() const {
    return m_bottom_pixels_end;
  }

  int grid::right_pixels_end() const {
    return m_right_pixels_end;
  }

  int grid::tile_width() const {
    return m_tile_width;
  }

  int grid::tile_height() const {
    return m_tile_height;
  }

  math::dimension2di_t grid::eval_transormed_dims(const math::dimension2di_t& orig,
                                                  const assets::rotation_info& ri) {
    if (ri.degree == 0) {
      if (ri.hflip) {
        return orig;
      } else if (ri.vflip) {
        return orig;
      }
    } else {
      if (ri.degree == 90) {
        return {orig[1], orig[0]};
      } else if (ri.degree == 180) {
        return orig;
      } else if (ri.degree == 270) {
        return {orig[1], orig[0]};
      }
    }
    RAISE_EX("unknown rotation");
  }
}
