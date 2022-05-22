//
// Created by igor on 22/05/2022.
//

#ifndef INCLUDE_NEUTRINO_HAL_VIDEO_TARGET_TEXTURE_HH
#define INCLUDE_NEUTRINO_HAL_VIDEO_TARGET_TEXTURE_HH

#include <optional>
#include <neutrino/hal/video/renderer.hh>
#include <neutrino/hal/video/texture.hh>

namespace neutrino::hal {
  struct use_texture {
    use_texture(renderer& r, texture& tex);
    ~use_texture();
    renderer& m_renderer;
    std::optional<texture> m_old_texture;
  };
}

#endif //INCLUDE_NEUTRINO_HAL_VIDEO_TARGET_TEXTURE_HH
