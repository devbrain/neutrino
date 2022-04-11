//
// Created by igor on 20/10/2021.
//

#include <neutrino/kernel/systems/video/video2d/video2d.hh>
#include <imgui.h>

namespace neutrino::kernel {
  video2d_system::video2d_system(hal::pixel_format texture_format)
  : m_texture_format (texture_format) {}

  hal::texture video2d_system::create_primary_texture () {
    return {get_renderer(),
            m_texture_format,
            (unsigned)width(),
            (unsigned)height(),
            hal::texture::access::TARGET};
  }

  void video2d_system::point (int x, int y) {
    get_renderer().point (x, y);
  }

  void video2d_system::point (const math::point2d& p) {
    get_renderer().point (p);
  }

  void video2d_system::line (int x1, int y1, int x2, int y2) {
    get_renderer().line (x1, y1, x2, y2);
  }

  void video2d_system::line (const math::point2d& p1, const math::point2d& p2) {
    get_renderer().line(p1, p2);
  }

  void video2d_system::hline (int x1, int x2, int y) {
    get_renderer().hline (x1, x2, y);
  }

  void video2d_system::vline (int x, int y1, int y2) {
    get_renderer().vline (x, y1, y2);
  }

  void video2d_system::rectangle (const math::rect& rec) {
    get_renderer().rectangle (rec);
  }

  void video2d_system::rectangle_filled (const math::rect& rec) {
    get_renderer().rectangle_filled (rec);
  }

  void video2d_system::rounded_rectangle (const math::rect& r, int radius) {
    get_renderer().rounded_rectangle (r, radius);
  }

  void video2d_system::rounded_rectangle_filled (const math::rect& r, int radius) {
    get_renderer().rounded_rectangle_filled (r, radius);
  }

  void video2d_system::aa_line (int x1, int y1, int x2, int y2) {
    get_renderer().aa_line (x1, y1, x2, y2);
  }

  void video2d_system::aa_line (const math::point2d& a, const math::point2d& b) {
    get_renderer().aa_line (a, b);
  }

  void video2d_system::thick_line (int x1, int y1, int x2, int y2, int width) {
    get_renderer().thick_line (x1, y1, x2, y2, width);
  }

  void video2d_system::thick_line (const math::point2d& a, const math::point2d& b, int width) {
    get_renderer().thick_line (a, b, width);
  }

  void video2d_system::circle (int x, int y, int radius) {
    get_renderer().circle (x, y, radius);
  }

  void video2d_system::circle (const math::point2d& p, int radius) {
    get_renderer().circle (p, radius);
  }

  void video2d_system::aa_circle (int x, int y, int radius) {
    get_renderer().aa_circle (x, y, radius);
  }

  void video2d_system::aa_circle (const math::point2d& p, int radius) {
    get_renderer().aa_circle (p, radius);
  }

  void video2d_system::circle_filled (int x, int y, int radius) {
    get_renderer().circle_filled (x, y, radius);
  }

  void video2d_system::circle_filled (const math::point2d& p, int radius) {
    get_renderer().circle_filled (p, radius);
  }

  void video2d_system::ellipse (int x, int y, int rx, int ry) {
    get_renderer().ellipse (x, y, rx, ry);
  }

  void video2d_system::ellipse (const math::point2d& p, int rx, int ry) {
    get_renderer().ellipse (p, rx, ry);
  }

  void video2d_system::aa_ellipse (int x, int y, int rx, int ry) {
    get_renderer().ellipse (x, y, rx, ry);
  }

  void video2d_system::aa_ellipse (const math::point2d& p, int rx, int ry) {
    get_renderer().aa_ellipse (p, rx, ry);
  }

  void video2d_system::ellipse_filled (int x, int y, int rx, int ry) {
    get_renderer().ellipse_filled (x, y, rx, ry);
  }

  void video2d_system::ellipse_filled (const math::point2d& p, int rx, int ry) {
    get_renderer().ellipse_filled (p, rx, ry);
  }

  void video2d_system::arc (int x, int y, int rad, float start_angle, float end_angle) {
    get_renderer().arc(x, y, rad, start_angle, end_angle);
  }

  void video2d_system::arc (const math::point2d& p, int rad, float start_angle, float end_angle) {
    get_renderer().arc(p, rad, start_angle, end_angle);
  }

  void video2d_system::pie (int x, int y, int rad, float start_angle, float end_angle) {
    get_renderer().pie (x, y, rad, start_angle, end_angle);
  }

  void video2d_system::pie (const math::point2d& p, int rad, float start_angle, float end_angle) {
    get_renderer().pie(p, rad, start_angle, end_angle);
  }

  void video2d_system::pie_filled (int x, int y, int rad, float start_angle, float end_angle) {
    get_renderer().pie_filled (x, y, rad, start_angle, end_angle);
  }

  void video2d_system::pie_filled (const math::point2d& p, int rad, float start_angle, float end_angle) {
    get_renderer().pie_filled (p, rad, start_angle, end_angle);
  }

}
