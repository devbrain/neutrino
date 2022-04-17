//
// Created by igor on 16/10/2021.
//

#ifndef SRC_KERNEL_SYSTEMS_VIDEO_ACCEL_RENDERER_VIDEO_SYSTEM_HH
#define SRC_KERNEL_SYSTEMS_VIDEO_ACCEL_RENDERER_VIDEO_SYSTEM_HH

#include <neutrino/kernel/systems/video/accel_renderer_video_system.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::kernel {

  void accel_renderer_video_system::init(hal::window& w) {
    video_system::init (w);
    if (auto* win = dynamic_cast<hal::window_2d*>(&w); win) {
      m_renderer = win->get_renderer ();
      m_renderer.logical_size (width (), height ());
    } else {
      RAISE_EX("Accelerated2d video system should be created from window_2d");
    }
  }

  void accel_renderer_video_system::present() {
    m_renderer.present();
  }

  hal::renderer& accel_renderer_video_system::get_renderer() {
    return m_renderer;
  }

  void accel_renderer_video_system::clear () {
    auto color = m_renderer.active_color();
    m_renderer.active_color({0,0,0});
    m_renderer.clear ();
    m_renderer.active_color(color);
  }
}

#endif //SRC_KERNEL_SYSTEMS_VIDEO_ACCEL_RENDERER_VIDEO_SYSTEM_HH
