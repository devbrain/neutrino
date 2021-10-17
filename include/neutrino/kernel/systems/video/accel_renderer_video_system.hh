//
// Created by igor on 16/10/2021.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_ACCEL_RENDERER_VIDEO_SYSTEM_HH
#define INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_ACCEL_RENDERER_VIDEO_SYSTEM_HH

#include <neutrino/hal/video/texture.hh>
#include <neutrino/kernel/systems/video/video_system.hh>

namespace neutrino::kernel {
  class accel_renderer_video_system : public video_system {
    protected:
      virtual hal::texture create_primary_texture () = 0;
      void init(hal::window_2d& w) override;
      void present() override;
    protected:
      hal::texture m_texture;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_ACCEL_RENDERER_VIDEO_SYSTEM_HH
