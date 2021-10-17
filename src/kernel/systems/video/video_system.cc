//
// Created by igor on 08/10/2021.
//

#include <neutrino/kernel/systems/video/video_system.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::kernel {
  video_system::~video_system () = default;

  void video_system::init(hal::window_2d& win) {
    m_renderer = win.get_renderer ();
    auto[w, h] = win.dimensions ();
    m_renderer.logical_size (w, h);

    m_width = w;
    m_height = h;
  }

  void video_system::clear () {
      m_renderer.clear ();
  }

  hal::renderer& video_system::get_renderer () {
    ENFORCE(m_renderer);
    return m_renderer;
  }

  int video_system::width () const noexcept {
    return m_width;
  }

  int video_system::height () const noexcept {
    return m_height;
  }
}
