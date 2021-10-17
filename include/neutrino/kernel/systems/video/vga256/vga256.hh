//
// Created by igor on 16/10/2021.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_VGA256_VGA256_HH
#define INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_VGA256_VGA256_HH

#include <memory>
#include <cstdint>
#include <array>
#include <vector>

#include <neutrino/hal/video/color.hh>
#include <neutrino/kernel/systems/video/video_system.hh>

namespace neutrino::kernel {
  class vga256 {
    public:
      using palette_t = std::array<hal::color, 256>;
      using surface_t = std::vector<uint8_t>;
    public:
      vga256(int w, int h);

      vga256(const vga256&) = delete;
      vga256& operator = (const vga256&) = delete;

      palette_t& palette () noexcept;
      surface_t& surface () noexcept;

      [[nodiscard]] const palette_t& palette () const noexcept;
      [[nodiscard]] const surface_t& surface () const noexcept;

      [[nodiscard]] int width () const noexcept;
      [[nodiscard]] int height () const noexcept;

      void cls ();

      std::unique_ptr<video_system> create_system() const;
    private:
      int m_width;
      int m_height;
      palette_t m_pal;
      surface_t m_surface;
  };
}


#endif //INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_VGA256_VGA256_HH
