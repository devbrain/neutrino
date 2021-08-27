//
// Created by igor on 30/06/2021.
//

#ifndef NEUTRINO_RENDERER_IMPL_HH
#define NEUTRINO_RENDERER_IMPL_HH

#include <hal/sdl/render.hh>

namespace neutrino::hal::detail {
  struct renderer_impl {
    template <typename ... Args>
    renderer_impl (Args &&... args)
        : renderer (std::forward<Args> (args)...) {

    }
    sdl::renderer renderer;
  };
}

#endif //NEUTRINO_RENDERER_IMPL_HH
