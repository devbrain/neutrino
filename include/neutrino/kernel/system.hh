//
// Created by igor on 07/10/2021.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_SYSTEM_HH
#define INCLUDE_NEUTRINO_KERNEL_SYSTEM_HH

#include <chrono>

namespace neutrino::kernel {
  class system {
    public:
      virtual ~system();

      virtual void on_paused(bool paused);
      virtual void update(std::chrono::milliseconds ms) = 0;
      virtual void present() = 0;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_SYSTEM_HH
