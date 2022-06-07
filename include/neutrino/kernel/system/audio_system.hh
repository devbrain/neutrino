//
// Created by igor on 02/06/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_SYSTEM_AUDIO_SYSTEM_HH
#define INCLUDE_NEUTRINO_KERNEL_SYSTEM_AUDIO_SYSTEM_HH

namespace neutrino {
  class application;
  namespace kernel {
    class audio_system {
        friend class neutrino::application;
    };
  }
}

#endif //INCLUDE_NEUTRINO_KERNEL_SYSTEM_AUDIO_SYSTEM_HH
