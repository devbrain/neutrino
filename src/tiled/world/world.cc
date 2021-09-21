//
// Created by igor on 06/07/2021.
//

#include <neutrino/tiled/world/world.hh>

namespace neutrino::tiled {
  world::world (orientation_t orientation, unsigned int width, unsigned int height,
                unsigned int tilewidth, unsigned int tileheight, const hal::color& bgcolor,
                render_order_t render_order, unsigned int hex_side_length, stagger_axis_t axis,
                stagger_index_t index, bool infinite)
      : m_orientation (orientation), m_width (width), m_height (height),
        m_tilewidth (tilewidth), m_tileheight (tileheight), m_bgcolor (bgcolor),
        m_render_order (render_order), m_hex_side_length (hex_side_length),
        m_axis (axis), m_index (index), m_infinite (infinite) {
  }

  orientation_t world::orientation () const {
    return m_orientation;
  }

  unsigned int world::width () const {
    return m_width;
  }

  unsigned int world::height () const {
    return m_height;
  }

  unsigned int world::tile_width () const {
    return m_tilewidth;
  }

  unsigned int world::tile_height () const {
    return m_tileheight;
  }

  const hal::color& world::bg_color () const {
    return m_bgcolor;
  }

  render_order_t world::render_order () const {
    return m_render_order;
  }

  unsigned int world::hex_side_length () const {
    return m_hex_side_length;
  }

  stagger_axis_t world::stagger_axis () const {
    return m_axis;
  }

  stagger_index_t world::stagger_index () const {
    return m_index;
  }

  bool world::infinite () const {
    return m_infinite;
  }
}