//
// Created by igor on 30/06/2021.
//

#ifndef NEUTRINO_TEXTURE_IMPL_HH
#define NEUTRINO_TEXTURE_IMPL_HH

#include <hal/sdl/texture.hh>

namespace neutrino::hal::detail {
    struct texture_impl {
        template<typename ... Args>
        texture_impl(Args&&... args)
        : texture(std::forward<Args>(args)...) {}
        sdl::texture texture;
    };
}

#endif //NEUTRINO_TEXTURE_IMPL_HH
