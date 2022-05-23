//
// Created by igor on 22/05/2022.
//

#include <neutrino/hal/video/renderer_utils.hh>

namespace neutrino::hal {
  use_texture::use_texture (renderer& r, texture& tex)
      : m_renderer (r),
        m_old_texture (r.target ()) {
    r.target (tex);
  }

  use_texture::~use_texture () {
    if (m_old_texture) {
      m_renderer.target (*m_old_texture);
    }
    else {
      m_renderer.restore_default_target ();
    }
  }

  clip_area::clip_area (renderer& rend, const math::rect& region)
      : r (rend) {
    if (r.clipping_enabled ()) {
      old = r.clip ();
    }
    r.clip (region);
  }

  clip_area::~clip_area () {
    if (old) {
      r.clip (*old);
    }
    else {
      r.disable_clippping ();
    }
  }

  use_color::use_color (renderer& ren, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
      : m_renderer (ren),
        m_old_color (ren.active_color ()) {
    m_renderer.active_color (color (r, g, b, a));
  }

  use_color::use_color (renderer& ren, const color& c)
      : m_renderer (ren),
        m_old_color (ren.active_color ()) {
    m_renderer.active_color (c);
  }

  use_color::~use_color () {
    m_renderer.active_color (m_old_color);
  }
}