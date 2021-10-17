//
// Created by igor on 16/10/2021.
//

#ifndef SRC_KERNEL_SYSTEMS_VIDEO_ACCEL_RENDERER_VIDEO_SYSTEM_HH
#define SRC_KERNEL_SYSTEMS_VIDEO_ACCEL_RENDERER_VIDEO_SYSTEM_HH

#include <neutrino/kernel/systems/video/accel_renderer_video_system.hh>

namespace neutrino::kernel {

  void accel_renderer_video_system::init(hal::window_2d& w) {
    video_system::init (w);
    m_texture = create_primary_texture();
  }

  void accel_renderer_video_system::present() {
    get_renderer().copy (m_texture);
  }
}

#endif //SRC_KERNEL_SYSTEMS_VIDEO_ACCEL_RENDERER_VIDEO_SYSTEM_HH
