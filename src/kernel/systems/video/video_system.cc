//
// Created by igor on 08/10/2021.
//

#include <neutrino/kernel/systems/video/video_system.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::kernel {
  video_system::~video_system () = default;

  void video_system::init(hal::window& win) {

    auto[w, h] = win.dimensions ();


    m_width = w;
    m_height = h;
  }



  int video_system::width () const noexcept {
    return m_width;
  }

  int video_system::height () const noexcept {
    return m_height;
  }
}
