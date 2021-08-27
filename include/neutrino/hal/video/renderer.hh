//
// Created by igor on 30/06/2021.
//

#ifndef NEUTRINO_HAL_VIDEO_RENDERER_HH
#define NEUTRINO_HAL_VIDEO_RENDERER_HH

#include <optional>
#include <tuple>
#include <bitflags/bitflags.hpp>
#include <neutrino/utils/spimpl.h>
#include <neutrino/hal/video/surface.hh>
#include <neutrino/hal/video/blend_mode.hh>
#include <neutrino/hal/video/texture.hh>
#include <neutrino/math/rect.hh>
#include <neutrino/math/point.hh>

namespace neutrino::hal {
  namespace detail {
    struct renderer_impl;
  }

  class window;

  class window_2d;

  class renderer {
      friend class texture;

      friend class window_2d;

    public:
      BEGIN_BITFLAGS(flags)
        FLAG(SOFTWARE)
        FLAG(ACCELERATED)
        FLAG(PRESENTVSYNC)
        FLAG(TARGETTEXTURE)
      END_BITFLAGS(flags)

      enum class flip {
          NONE,
          HORIZONTAL,
          VERTICAL
      };
    public:
      renderer () = default;
      renderer (const window &w, flags f);

      explicit renderer (const window &w);
      explicit renderer (const surface &s);

      operator bool () const;

      [[nodiscard]] blend_mode blend () const;
      void blend (blend_mode bm);

      [[nodiscard]] color active_color () const;
      void active_color (const color &c);

      [[nodiscard]] math::rect clip () const;
      void clip (const math::rect &area);
      void disable_clippping ();
      [[nodiscard]] bool clipping_enabled () const;

      [[nodiscard]] bool integer_scaling () const;
      void integer_scaling (bool enabled);

      [[nodiscard]] std::pair<unsigned, unsigned> logical_size () const;
      void logical_size (unsigned x, unsigned y);

      [[nodiscard]] std::pair<float, float> scaling () const;
      void scaling (float x, float y);

      [[nodiscard]] math::rect viewport () const;
      void viewport (const math::rect &area);
      void disable_viewport ();

      /**
       * This is a very slow operation, and should not be used frequently.
       *  pitch specifies the number of bytes between rows in the destination pixels data.
       *  This allows you to write to a submath::rectangle or have padded rows in the destination.
       *  Generally, pitch should equal the number of pixels per row in the pixels data times the number
       *  of bytes per pixel, but it might contain additional padding
       *  (for example, 24bit RGB Windows Bitmap data pads all rows to multiples of 4 bytes).
       */
      void read_pixels (const pixel_format &fmt, void *dst, std::size_t pitch) const;
      void read_pixels (const math::rect &area, const pixel_format &fmt, void *dst, std::size_t pitch) const;

      std::optional<texture> target () const;
      void target (texture &t);
      void restore_default_target ();

      std::pair<unsigned, unsigned> output_size () const;

      void clear ();

      void copy (const texture &t, flip flip_ = flip::NONE);
      void copy (const texture &t, const math::rect &srcrect, flip flip_ = flip::NONE);
      void copy (const texture &t, const math::rect &srcrect, const math::rect &dstrect, flip flip_ = flip::NONE);
      void
      copy (const texture &t, const math::rect &srcrect, const math::rect &dstrect, double angle, flip flip_ = flip::NONE);
      void
      copy (const texture &t, const math::rect &srcrect, const math::rect &dstrect, double angle, const math::point2d &pt, flip flip_);

      void point (int x, int y);
      void point (const math::point2d &p);

      void line (int x1, int y1, int x2, int y2);
      void line (const math::point2d &p1, const math::point2d &p2);
      // horizontal line
      void hline (int x1, int x2, int y);
      // vertical line
      void vline (int x, int y1, int y2);

      void rectangle (const math::rect &rec);
      void rectangle_filled (const math::rect &rec);

      void rounded_rectangle (const math::rect &r, int radius);
      void rounded_rectangle_filled (const math::rect &r, int radius);

      // Anti-alising line
      void aa_line (int x1, int y1, int x2, int y2);
      void aa_line (const math::point2d &a, const math::point2d &b);

      void thick_line (int x1, int y1, int x2, int y2, int width);
      void thick_line (const math::point2d &a, const math::point2d &b, int width);

      void circle (int x, int y, int radius);
      void circle (const math::point2d &p, int radius);

      void aa_circle (int x, int y, int radius);
      void aa_circle (const math::point2d &p, int radius);

      void circle_filled (int x, int y, int radius);
      void circle_filled (const math::point2d &p, int radius);

      void ellipse (int x, int y, int rx, int ry);
      void ellipse (const math::point2d &p, int rx, int ry);

      void aa_ellipse (int x, int y, int rx, int ry);
      void aa_ellipse (const math::point2d &p, int rx, int ry);

      void ellipse_filled (int x, int y, int rx, int ry);
      void ellipse_filled (const math::point2d &p, int rx, int ry);

      void arc (int x, int y, int rad, float start_angle, float end_angle);
      void arc (const math::point2d &p, int rad, float start_angle, float end_angle);

      void pie (int x, int y, int rad, float start_angle, float end_angle);
      void pie (const math::point2d &p, int rad, float start_angle, float end_angle);

      void pie_filled (int x, int y, int rad, float start_angle, float end_angle);
      void pie_filled (const math::point2d &p, int rad, float start_angle, float end_angle);

      void present () noexcept;
    private:
      renderer (std::unique_ptr<detail::renderer_impl> impl);
      spimpl::unique_impl_ptr<detail::renderer_impl> m_pimpl;
  };
}

#endif
