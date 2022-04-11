//
// Created by igor on 16/10/2021.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_OPENGL_VIDEO_SYSTEM_HH
#define INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_OPENGL_VIDEO_SYSTEM_HH

#include <neutrino/kernel/systems/video/video_system.hh>

namespace neutrino::kernel {
  class opengl_video_system : public video_system {
    protected:
      void clear() override;
      void init(hal::window& w) override;
      void present() override;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_OPENGL_VIDEO_SYSTEM_HH
