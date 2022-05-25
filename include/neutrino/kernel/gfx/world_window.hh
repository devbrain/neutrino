//
// Created by igor on 25/05/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_GFX_WORLD_WINDOW_HH
#define INCLUDE_NEUTRINO_KERNEL_GFX_WORLD_WINDOW_HH

#include <neutrino/math/rect.hh>
namespace neutrino::kernel {

  class world_window {
    public:
      world_window() = default;
      world_window(math::point2d screen_pos, math::dimension2di_t dimensions);
      world_window(math::point2d screen_pos, math::point2d world_pos, math::dimension2di_t dimensions);

      void screen_pos(math::point2d pos) noexcept;
      [[nodiscard]] math::point2d screen_pos() const noexcept;
      void add_screen_pos(math::point2d pos) noexcept;
      void add_screen_pos(int dx, int dy) noexcept;

      void world_pos(math::point2d pos) noexcept;
      [[nodiscard]] math::point2d world_pos() const noexcept;
      void add_world_pos(math::point2d pos) noexcept;
      void add_world_pos(int dx, int dy) noexcept;

      void dimensions(math::dimension2di_t dims) noexcept;
      [[nodiscard]] math::dimension2di_t dimensions() const noexcept;
      void add_dimensions(math::dimension2di_t dims) noexcept;
      void add_dimensions(int dx, int dy) noexcept;

      void clip(const math::dimension2di_t& screen_dims, const math::dimension2di_t& world_dims);
    private:
      math::point2d m_screen_top_left;
      math::point2d m_world_top_left;
      math::dimension2di_t m_dimensions;
  };

}

#endif //INCLUDE_NEUTRINO_KERNEL_GFX_WORLD_WINDOW_HH
