//
// Created by igor on 25/05/2022.
//

#include <neutrino/kernel/gfx/world_window.hh>

namespace neutrino::kernel {
  world_window::world_window (math::point2d screen_pos, math::dimension2di_t dimensions)
      : m_screen_top_left (screen_pos), m_world_top_left (), m_dimensions (dimensions) {

  }

  world_window::world_window (math::point2d screen_pos, math::point2d world_pos, math::dimension2di_t dimensions)
      : m_screen_top_left (screen_pos), m_world_top_left (world_pos), m_dimensions (dimensions) {

  }

  void world_window::screen_pos (math::point2d pos) noexcept {
    m_screen_top_left = pos;
  }

  math::point2d world_window::screen_pos () const noexcept {
    return m_screen_top_left;
  }

  void world_window::add_screen_pos (math::point2d pos) noexcept {
    m_screen_top_left += pos;
  }

  void world_window::add_screen_pos (int dx, int dy) noexcept {
    m_screen_top_left += math::point2d (dx, dy);
  }

  void world_window::world_pos (math::point2d pos) noexcept {
    m_world_top_left = pos;
  }

  math::point2d world_window::world_pos () const noexcept {
    return m_world_top_left;
  }

  void world_window::add_world_pos (math::point2d pos) noexcept {
    m_world_top_left += pos;
  }

  void world_window::add_world_pos (int dx, int dy) noexcept {
    m_world_top_left += math::point2d (dx, dy);
  }

  void world_window::dimensions (math::dimension2di_t dims) noexcept {
    m_dimensions = dims;
  }

  math::dimension2di_t world_window::dimensions () const noexcept {
    return m_dimensions;
  }

  void world_window::add_dimensions (math::dimension2di_t dims) noexcept {
    m_dimensions += dims;
  }

  void world_window::add_dimensions (int dx, int dy) noexcept {
    m_dimensions += math::dimension2di_t (dx, dy);
  }

  void world_window::clip (const math::dimension2di_t& screen_dims, const math::dimension2di_t& world_dims) {
    // fix dimensions
    if (m_dimensions.x <= 0) {
      m_dimensions.x = 1;
    }
    else if (m_dimensions.x >= screen_dims.x) {
      m_dimensions.x = screen_dims.x;
    }
    if (m_dimensions.y <= 0) {
      m_dimensions.y = 1;
    }
    else if (m_dimensions.y >= screen_dims.y) {
      m_dimensions.y = screen_dims.y;
    }

    // fix screen
    if (m_screen_top_left.x < 0) {
      m_screen_top_left.x = 0;
    }
    if (m_screen_top_left.y < 0) {
      m_screen_top_left.y = 0;
    }
    if (m_screen_top_left.x + m_dimensions.x > screen_dims.x) {
      m_screen_top_left.x = screen_dims.x - m_dimensions.x;
    }
    if (m_screen_top_left.y + m_dimensions.y > screen_dims.y) {
      m_screen_top_left.y = screen_dims.y - m_dimensions.y;
    }
    // fix world
    if (m_world_top_left.x < 0) {
      m_world_top_left.x = 0;
    }
    if (m_world_top_left.y < 0) {
      m_world_top_left.y = 0;
    }
    if (m_world_top_left.x + m_dimensions.x > world_dims.x) {
      m_world_top_left.x = world_dims.x - m_dimensions.x;
    }
    if (m_world_top_left.y + m_dimensions.y > world_dims.y) {
      m_world_top_left.y = world_dims.y - m_dimensions.y;
    }
  }
}