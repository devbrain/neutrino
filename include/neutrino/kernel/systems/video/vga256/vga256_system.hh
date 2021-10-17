//
// Created by igor on 16/10/2021.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_VGA256_VGA256_SYSTEM_HH
#define INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_VGA256_VGA256_SYSTEM_HH

#include <neutrino/kernel/systems/video/accel_renderer_video_system.hh>

namespace neutrino::kernel {
  class vga256;

  class vga256_system : public accel_renderer_video_system {
    public:
      explicit vga256_system(const vga256& model);
    private:
      hal::texture create_primary_texture () override;
      void update() override;
    private:
      const vga256& m_model;
      int m_render_w;
      int m_render_h;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_VGA256_VGA256_SYSTEM_HH
