//
// Created by igor on 20/10/2021.
//

#include <neutrino/kernel/systems/video/video2d/video2d.hh>


namespace neutrino::kernel {


  video2d_system::blend_mode_t video2d_system::blend () const {
    return m_renderer.blend();
  }

  void video2d_system::blend (video2d_system::blend_mode_t bm) {
    m_renderer.blend(bm);
  }

  hal::color video2d_system::active_color () const {
    return m_renderer.active_color();
  }

  void video2d_system::active_color (const hal::color& c) {
    m_renderer.active_color(c);
  }

  void video2d_system::point (int x, int y) {
    m_renderer.point (x, y);
  }

  void video2d_system::point (const math::point2d& p) {
    m_renderer.point (p);
  }

  void video2d_system::line (int x1, int y1, int x2, int y2) {
    m_renderer.line (x1, y1, x2, y2);
  }

  void video2d_system::line (const math::point2d& p1, const math::point2d& p2) {
    m_renderer.line(p1, p2);
  }

  void video2d_system::hline (int x1, int x2, int y) {
    m_renderer.hline (x1, x2, y);
  }

  void video2d_system::vline (int x, int y1, int y2) {
    m_renderer.vline (x, y1, y2);
  }

  void video2d_system::rectangle (const math::rect& rec) {
    m_renderer.rectangle (rec);
  }

  void video2d_system::rectangle_filled (const math::rect& rec) {
    m_renderer.rectangle_filled (rec);
  }

  void video2d_system::rounded_rectangle (const math::rect& r, int radius) {
    m_renderer.rounded_rectangle (r, radius);
  }

  void video2d_system::rounded_rectangle_filled (const math::rect& r, int radius) {
    m_renderer.rounded_rectangle_filled (r, radius);
  }

  void video2d_system::aa_line (int x1, int y1, int x2, int y2) {
    m_renderer.aa_line (x1, y1, x2, y2);
  }

  void video2d_system::aa_line (const math::point2d& a, const math::point2d& b) {
    m_renderer.aa_line (a, b);
  }

  void video2d_system::thick_line (int x1, int y1, int x2, int y2, int width) {
    m_renderer.thick_line (x1, y1, x2, y2, width);
  }

  void video2d_system::thick_line (const math::point2d& a, const math::point2d& b, int width) {
    m_renderer.thick_line (a, b, width);
  }

  void video2d_system::circle (int x, int y, int radius) {
    m_renderer.circle (x, y, radius);
  }

  void video2d_system::circle (const math::point2d& p, int radius) {
    m_renderer.circle (p, radius);
  }

  void video2d_system::aa_circle (int x, int y, int radius) {
    m_renderer.aa_circle (x, y, radius);
  }

  void video2d_system::aa_circle (const math::point2d& p, int radius) {
    m_renderer.aa_circle (p, radius);
  }

  void video2d_system::circle_filled (int x, int y, int radius) {
    m_renderer.circle_filled (x, y, radius);
  }

  void video2d_system::circle_filled (const math::point2d& p, int radius) {
    m_renderer.circle_filled (p, radius);
  }

  void video2d_system::ellipse (int x, int y, int rx, int ry) {
    m_renderer.ellipse (x, y, rx, ry);
  }

  void video2d_system::ellipse (const math::point2d& p, int rx, int ry) {
    m_renderer.ellipse (p, rx, ry);
  }

  void video2d_system::aa_ellipse (int x, int y, int rx, int ry) {
    m_renderer.ellipse (x, y, rx, ry);
  }

  void video2d_system::aa_ellipse (const math::point2d& p, int rx, int ry) {
    m_renderer.aa_ellipse (p, rx, ry);
  }

  void video2d_system::ellipse_filled (int x, int y, int rx, int ry) {
    m_renderer.ellipse_filled (x, y, rx, ry);
  }

  void video2d_system::ellipse_filled (const math::point2d& p, int rx, int ry) {
    m_renderer.ellipse_filled (p, rx, ry);
  }

  void video2d_system::arc (int x, int y, int rad, float start_angle, float end_angle) {
    m_renderer.arc(x, y, rad, start_angle, end_angle);
  }

  void video2d_system::arc (const math::point2d& p, int rad, float start_angle, float end_angle) {
    m_renderer.arc(p, rad, start_angle, end_angle);
  }

  void video2d_system::pie (int x, int y, int rad, float start_angle, float end_angle) {
    m_renderer.pie (x, y, rad, start_angle, end_angle);
  }

  void video2d_system::pie (const math::point2d& p, int rad, float start_angle, float end_angle) {
    m_renderer.pie(p, rad, start_angle, end_angle);
  }

  void video2d_system::pie_filled (int x, int y, int rad, float start_angle, float end_angle) {
    m_renderer.pie_filled (x, y, rad, start_angle, end_angle);
  }

  void video2d_system::pie_filled (const math::point2d& p, int rad, float start_angle, float end_angle) {
    m_renderer.pie_filled (p, rad, start_angle, end_angle);
  }

  void video2d_system::copy (const hal::texture& t, flip_t flip) {
    m_renderer.copy (t, flip);
  }
  void video2d_system::copy (const hal::texture& t, const math::rect& srcrect, flip_t flip) {
    m_renderer.copy (t, srcrect, flip);
  }
  void video2d_system::copy (const hal::texture& t, const math::rect& srcrect, const math::rect& dstrect, flip_t flip) {
    m_renderer.copy (t, srcrect, dstrect, flip);
  }
  void video2d_system::copy (const hal::texture& t, const math::rect& srcrect, const math::rect& dstrect, double angle, flip_t flip) {
    m_renderer.copy (t, srcrect, dstrect, angle, flip);
  }
  void video2d_system::copy (const hal::texture& t, const math::rect& srcrect, const math::rect& dstrect, double angle,
             const math::point2d& pt, flip_t flip) {
    m_renderer.copy (t, srcrect, dstrect, angle, pt, flip);
  }

  void video2d_system::update() {

  }
}
