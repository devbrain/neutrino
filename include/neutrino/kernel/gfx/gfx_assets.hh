//
// Created by igor on 16/05/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_GFX_GFX_ASSETS_HH
#define INCLUDE_NEUTRINO_KERNEL_GFX_GFX_ASSETS_HH

#include <neutrino/kernel/gfx/texture_atlas.hh>
#include <neutrino/kernel/rc/animation_description.hh>

namespace neutrino::kernel {
  struct gfx_assets {
    texture_atlas textures;
    animation_description animation_sequences;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_GFX_GFX_ASSETS_HH
