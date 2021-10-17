//
// Created by igor on 16/10/2021.
//

#include <neutrino/kernel/systems/video/vga256/vga256.hh>
#include <neutrino/kernel/systems/video/vga256/vga256_system.hh>

namespace neutrino::kernel {
  vga256::vga256(int w, int h)
  : m_width(w), m_height(h), m_surface(w*h, 0) {
    m_pal.fill ({0,0,0});
  }

  vga256::palette_t& vga256::palette () noexcept{
    return m_pal;
  }

  vga256::surface_t& vga256::surface () noexcept{
    return m_surface;
  }

  const vga256::palette_t& vga256::palette () const noexcept{
    return m_pal;
  }

  const vga256::surface_t& vga256::surface () const noexcept{
    return m_surface;
  }

  int vga256::width () const noexcept {
    return m_width;
  }

  int vga256::height () const noexcept {
    return m_height;
  }

  void vga256::cls () {
    std::fill(m_surface.begin(), m_surface.end(), 0);
  }

  std::unique_ptr<video_system> vga256::create_system() const {
    return std::make_unique<vga256_system>(*this);
  }
}