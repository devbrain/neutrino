//
// Created by igor on 02/06/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_SYSTEM_CONTEXT_HH
#define INCLUDE_NEUTRINO_KERNEL_SYSTEM_CONTEXT_HH

#include <tuple>
#include <neutrino/kernel/system/video_system.hh>
#include <neutrino/kernel/system/audio_system.hh>

namespace neutrino {
  class application;
  namespace kernel {
    class context {
        friend class neutrino::application;
      public:
        [[nodiscard]] audio_system& audio ();
        [[nodiscard]] video_system& video ();
      private:
        template <class ... Args>
        explicit context (Args&& ... args)
            : data (std::make_tuple (std::forward<Args> (args)...)) {
        }

      private:
        std::tuple<audio_system, video_system> data;
    };
  }
}

#endif //INCLUDE_NEUTRINO_KERNEL_SYSTEM_CONTEXT_HH
