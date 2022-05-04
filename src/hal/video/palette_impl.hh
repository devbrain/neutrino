//
// Created by igor on 03/05/2022.
//

#ifndef SRC_HAL_VIDEO_PALETTE_IMPL_HH
#define SRC_HAL_VIDEO_PALETTE_IMPL_HH

#include <hal/sdl/palette.hh>

namespace neutrino::hal::detail {
  struct palette_impl {
    palette_impl () = default;

    template <typename ... Args>
    explicit palette_impl (Args&& ... args)
        : palette (std::forward<Args> (args)...) {

    }

    sdl::palette palette;
  };
}

#endif //SRC_HAL_VIDEO_PALETTE_IMPL_HH
