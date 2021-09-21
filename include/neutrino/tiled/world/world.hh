//
// Created by igor on 06/07/2021.
//

#ifndef NEUTRINO_TILED_WORLD_WORLD_HH
#define NEUTRINO_TILED_WORLD_WORLD_HH

#include <neutrino/tiled/world/layer.hh>

namespace neutrino::tiled {
  class world_builder;

  class world {
    friend class world_builder;
    public:
      world(const world&) = delete;
      world& operator = (const world&) = delete;

      world(world&&) = default;

      template <class Functor>
      void visit_layers(Functor&& visitor);
    private:
      world();
    private:
      std::vector<tiles_layer> m_layers;
  };
  // ==========================================================================
  template <class Functor>
  void world::visit_layers(Functor&& visitor) {
    for (const auto& the_layer : m_layers) {
      visitor(the_layer);
    }
  }
}

#endif
