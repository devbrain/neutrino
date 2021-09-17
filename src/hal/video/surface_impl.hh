//
// Created by igor on 26/06/2021.
//

#ifndef NEUTRINO_SURFACE_IMPL_HH
#define NEUTRINO_SURFACE_IMPL_HH

#include <hal/sdl/surface.hh>

namespace neutrino::hal::detail {
  struct surface_impl {
    surface_impl () = default;

    template <typename ... Args>
    explicit surface_impl (Args&& ... args)
        : surface (std::forward<Args> (args)...) {

    }

    sdl::surface surface;
  };
}

#endif
