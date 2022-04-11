//
// Created by igor on 07/10/2021.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_SYSTEM_HH
#define INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_SYSTEM_HH


#include <neutrino/hal/video/renderer.hh>
#include <neutrino/hal/video/window.hh>

namespace neutrino::kernel {
  template <typename Base>
  class main_window;

  class video_system {
      template <typename> friend class main_window;
    public:
      virtual ~video_system();
      virtual void clear() = 0;
      virtual void update() = 0;
      virtual void present() = 0;
    protected:
      [[nodiscard]] int width () const noexcept;
      [[nodiscard]] int height () const noexcept;
      virtual void init(hal::window& w) = 0;
    private:
      int m_width {};
      int m_height {};
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_SYSTEM_HH
