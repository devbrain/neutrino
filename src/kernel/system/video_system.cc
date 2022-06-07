//
// Created by igor on 02/06/2022.
//

#include <neutrino/kernel/system/video_system.hh>

namespace neutrino::kernel {

  video_system::video_system(hal::renderer& r, math::dimension2di_t d)
  : m_renderer(&r) , m_dims(d) {}

  hal::renderer& video_system::renderer() const {
    return *m_renderer;
  }

  math::dimension2di_t video_system::dimensions() const {
    return m_dims;
  }


}