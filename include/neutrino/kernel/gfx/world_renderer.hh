//
// Created by igor on 11/05/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_GFX_WORLD_RENDERER_HH
#define INCLUDE_NEUTRINO_KERNEL_GFX_WORLD_RENDERER_HH

#include <chrono>

#include <neutrino/assets/tiles/world/world.hh>
#include <neutrino/kernel/gfx/world_window.hh>
#include <neutrino/utils/spimpl.h>

namespace neutrino::kernel {

  class world_renderer {
    public:
      world_renderer();
      ~world_renderer();

      void set(assets::world* w);
      void set(assets::world* w, const assets::world_assets* atlas);
      void update (std::chrono::milliseconds ms);
      void draw(const world_window& window, hal::renderer& renderer);

    private:
      struct impl;
      spimpl::unique_impl_ptr<impl> m_pimpl;
  };

}

#endif //INCLUDE_NEUTRINO_KERNEL_GFX_WORLD_RENDERER_HH
