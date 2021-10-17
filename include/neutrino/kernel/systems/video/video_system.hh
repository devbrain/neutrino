//
// Created by igor on 07/10/2021.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_SYSTEM_HH
#define INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_SYSTEM_HH


#include <neutrino/hal/video/renderer.hh>
#include <neutrino/hal/video/window.hh>

namespace neutrino::kernel {
  class systems_manager;
  class main_window;

  class video_system {
      friend class systems_manager;
      friend class main_window;
    public:
      virtual ~video_system();
      void clear();
      virtual void update() = 0;
      virtual void present() = 0;
    protected:
      [[nodiscard]] hal::renderer& get_renderer();

      [[nodiscard]] int width () const noexcept;
      [[nodiscard]] int height () const noexcept;
      virtual void init(hal::window_2d& w);

    private:
      hal::renderer m_renderer;
      int m_width {};
      int m_height {};
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_SYSTEM_HH
