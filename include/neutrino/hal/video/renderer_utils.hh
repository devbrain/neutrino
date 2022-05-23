//
// Created by igor on 22/05/2022.
//

#ifndef INCLUDE_NEUTRINO_HAL_VIDEO_RENDERER_UTILS_HH
#define INCLUDE_NEUTRINO_HAL_VIDEO_RENDERER_UTILS_HH

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

  struct use_color {
    use_color(renderer& ren, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    use_color(renderer& ren, const color& c);
    ~use_color();

    renderer& m_renderer;
    hal::color m_old_color;
  };

  struct clip_area {
    clip_area(renderer& rend, const math::rect& region);
    ~clip_area();

    renderer& r;
    std::optional<math::rect> old;
  };
}

#endif //INCLUDE_NEUTRINO_HAL_VIDEO_RENDERER_UTILS_HH
