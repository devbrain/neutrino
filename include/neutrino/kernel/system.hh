//
// Created by igor on 07/10/2021.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_SYSTEM_HH
#define INCLUDE_NEUTRINO_KERNEL_SYSTEM_HH

#include <chrono>
#include <neutrino/kernel/systems/video/video_system.hh>

namespace neutrino::kernel {
  class system {
    public:
      system();
      virtual ~system();

      virtual void setup();
      virtual void on_paused(bool paused);
      virtual void update(std::chrono::milliseconds ms) = 0;
      virtual void present() = 0;
    protected:
      struct systems {
        systems();
        video_system* m_video;
      };
      systems m_systems;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_SYSTEM_HH
