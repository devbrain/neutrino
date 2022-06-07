//
// Created by igor on 02/06/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_SYSTEM_VIDEO_SYSTEM_HH
#define INCLUDE_NEUTRINO_KERNEL_SYSTEM_VIDEO_SYSTEM_HH

#include <neutrino/hal/video/renderer.hh>
#include <neutrino/math/rect.hh>

namespace neutrino {
  class application;
  namespace kernel {
    class video_system {
        friend class neutrino::application;
      public:
        [[nodiscard]] hal::renderer& renderer () const;
        [[nodiscard]] math::dimension2di_t dimensions () const;
      private:
        video_system (hal::renderer& r, math::dimension2di_t d);
      private:
        hal::renderer* m_renderer;
        math::dimension2di_t m_dims;
    };
  }
}

#endif //INCLUDE_NEUTRINO_KERNEL_SYSTEM_VIDEO_SYSTEM_HH
