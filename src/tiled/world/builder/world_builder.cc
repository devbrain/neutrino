//
// Created by igor on 21/09/2021.
//

#include <neutrino/tiled/world/builder/world_builder.hh>

namespace neutrino::tiled {
  world_builder& world_builder::add(tiles_layer&& alayer) {
    m_layers.push_back (std::move (alayer));
    return *this;
  }

  world world_builder::build() {
    world res(m_orientation, m_width, m_height,
              m_tilewidth, m_tileheight,
              m_bgcolor,
              m_render_order, m_hex_side_length, m_axis, m_index, m_infinite);
    std::swap (res.m_layers, m_layers);
    return res;
  }

  world_builder& world_builder::orientation (orientation_t orientation) {
    world_builder::m_orientation = orientation;
    return *this;
  }

  world_builder& world_builder::width (unsigned int width) {
    world_builder::m_width = width;
    return *this;
  }

  world_builder& world_builder::height (unsigned int height) {
    world_builder::m_height = height;
    return *this;
  }

  world_builder& world_builder::tile_width (unsigned int tilewidth) {
    world_builder::m_tilewidth = tilewidth;
    return *this;
  }

  world_builder& world_builder::tile_height (unsigned int tileheight) {
    world_builder::m_tileheight = tileheight;
    return *this;
  }

  world_builder& world_builder::bg_color (const hal::color& bgcolor) {
    world_builder::m_bgcolor = bgcolor;
    return *this;
  }

  world_builder& world_builder::render_order (render_order_t render_order) {
    world_builder::m_render_order = render_order;
    return *this;
  }

  world_builder& world_builder::hex_side_length (unsigned int hex_side_length) {
    world_builder::m_hex_side_length = hex_side_length;
    return *this;
  }

  world_builder& world_builder::stagger_axis (stagger_axis_t axis) {
    world_builder::m_axis = axis;
    return *this;
  }

  world_builder& world_builder::stagger_index (stagger_index_t index) {
    world_builder::m_index = index;
    return *this;
  }

  world_builder& world_builder::infinite (bool infinite) {
    world_builder::m_infinite = infinite;
    return *this;
  }
}
