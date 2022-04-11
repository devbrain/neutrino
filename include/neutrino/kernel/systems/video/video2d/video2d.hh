//
// Created by igor on 20/10/2021.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_VIDEO2D_VIDEO2D_HH
#define INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_VIDEO2D_VIDEO2D_HH

#include <neutrino/kernel/systems/video/accel_renderer_video_system.hh>
#include <neutrino/hal/video/pixel_format.hh>
#include <neutrino/math/rect.hh>
#include <neutrino/math/point.hh>

namespace neutrino::kernel {
  class video2d_system : public accel_renderer_video_system {
    public:
      explicit video2d_system(hal::pixel_format texture_format = hal::pixel_format::make_rgba_32bit());
    protected:
      void point (int x, int y);
      void point (const math::point2d& p);

      void line (int x1, int y1, int x2, int y2);
      void line (const math::point2d& p1, const math::point2d& p2);
      // horizontal line
      void hline (int x1, int x2, int y);
      // vertical line
      void vline (int x, int y1, int y2);

      void rectangle (const math::rect& rec);
      void rectangle_filled (const math::rect& rec);

      void rounded_rectangle (const math::rect& r, int radius);
      void rounded_rectangle_filled (const math::rect& r, int radius);

      // Anti-alising line
      void aa_line (int x1, int y1, int x2, int y2);
      void aa_line (const math::point2d& a, const math::point2d& b);

      void thick_line (int x1, int y1, int x2, int y2, int width);
      void thick_line (const math::point2d& a, const math::point2d& b, int width);

      void circle (int x, int y, int radius);
      void circle (const math::point2d& p, int radius);

      void aa_circle (int x, int y, int radius);
      void aa_circle (const math::point2d& p, int radius);

      void circle_filled (int x, int y, int radius);
      void circle_filled (const math::point2d& p, int radius);

      void ellipse (int x, int y, int rx, int ry);
      void ellipse (const math::point2d& p, int rx, int ry);

      void aa_ellipse (int x, int y, int rx, int ry);
      void aa_ellipse (const math::point2d& p, int rx, int ry);

      void ellipse_filled (int x, int y, int rx, int ry);
      void ellipse_filled (const math::point2d& p, int rx, int ry);

      void arc (int x, int y, int rad, float start_angle, float end_angle);
      void arc (const math::point2d& p, int rad, float start_angle, float end_angle);

      void pie (int x, int y, int rad, float start_angle, float end_angle);
      void pie (const math::point2d& p, int rad, float start_angle, float end_angle);

      void pie_filled (int x, int y, int rad, float start_angle, float end_angle);
      void pie_filled (const math::point2d& p, int rad, float start_angle, float end_angle);

    private:
      hal::texture create_primary_texture () override;
    private:
      hal::pixel_format m_texture_format;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_SYSTEMS_VIDEO_VIDEO2D_VIDEO2D_HH
