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
    world res;
    std::swap (res.m_layers, m_layers);
    return res;
  }
}
