//
// Created by igor on 30/06/2021.
//

#ifndef NEUTRINO_TEXTURE_IMPL_HH
#define NEUTRINO_TEXTURE_IMPL_HH

#include <hal/sdl/texture.hh>

namespace neutrino::hal::detail {
  struct texture_impl {

    static uint32_t get_format (const sdl::texture& tex) {
      uint32_t format;
      SDL_QueryTexture (tex.const_handle (), &format, nullptr, nullptr, nullptr);
      return format;
    }

    template <typename ... Args>
    texture_impl (Args&& ... args)
        : texture (std::forward<Args> (args)...),
          format (sdl::object<SDL_PixelFormat> (SDL_AllocFormat (get_format (texture)), true)) {
    }

    sdl::texture texture;
    sdl::object <SDL_PixelFormat> format;
  };
}

#endif //NEUTRINO_TEXTURE_IMPL_HH
