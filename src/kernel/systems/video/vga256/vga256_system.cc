//
// Created by igor on 16/10/2021.
//

#include <algorithm>
#include <neutrino/kernel/systems/video/vga256/vga256.hh>
#include <neutrino/kernel/systems/video/vga256/vga256_system.hh>

namespace neutrino::kernel {
  vga256_system::vga256_system(const vga256& model)
  : m_model(model), m_render_w(0), m_render_h(0) {}

  hal::texture vga256_system::create_primary_texture () {
    m_render_w = std::min(m_model.width(), width());
    m_render_h = std::min(m_model.height(), height());

    return {get_renderer(),
            hal::pixel_format::make_rgba_32bit (),
            (unsigned)m_model.width(),
            (unsigned)m_model.height(),
            hal::texture::access::STREAMING};
  }

  void vga256_system::update () {
    auto[px, pitch] = m_texture.lock ();
    auto* pixels = (uint32_t*) px;
    const auto& pal = m_model.palette();
    const auto& surf = m_model.surface();

    for (int y = 0; y < m_render_h; y++) {
      for (int x = 0; x < m_render_w; x++) {
        auto idx = m_render_w*y + x;
        pixels[idx] = m_texture.map_rgb (pal[surf[idx]]);
      }
    }
    m_texture.unlock ();
  }
}
