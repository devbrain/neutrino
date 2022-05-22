//
// Created by igor on 22/05/2022.
//

#include <neutrino/hal/video/target_texture.hh>

namespace neutrino::hal {
  use_texture::use_texture(renderer& r, texture& tex)
  : m_renderer(r),
    m_old_texture(r.target())
  {
    r.target (tex);
  }

  use_texture::~use_texture() {
    if (m_old_texture) {
      m_renderer.target(*m_old_texture);
    } else {
      m_renderer.restore_default_target();
    }
  }
}