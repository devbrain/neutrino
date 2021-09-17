//
// Created by igor on 30/06/2021.
//

#include <neutrino/hal/video/renderer.hh>
#include <neutrino/hal/video/window.hh>
#include "hal/video/renderer_impl.hh"
#include "hal/video/texture_impl.hh"
#include "hal/video/surface_impl.hh"
#include "hal/video/windows/window_impl.hh"

#define SDL2_GFXPRIMITIVES_SCOPE extern "C"
#include "hal/video/thirdparty/gfx/SDL2_gfxPrimitives.h"

#include "hal/cast.hh"

namespace neutrino {
  static sdl::renderer::flip cast (hal::renderer::flip f) {
    switch (f) {
      case hal::renderer::flip::NONE:
        return sdl::renderer::flip::NONE;
      case hal::renderer::flip::VERTICAL:
        return sdl::renderer::flip::VERTICAL;
      case hal::renderer::flip::HORIZONTAL:
        return sdl::renderer::flip::HORIZONTAL;
      default:
        RAISE_EX("Should not be here");
    }
  }
}

namespace neutrino::hal {
  template <typename Flag>
  sdl::renderer::flags get_flag (const renderer::flags& flags, Flag f, sdl::renderer::flags x) {
    if (flags & f) {
      return x;
    }
    return sdl::renderer::flags::NONE;
  }

  renderer::renderer (const window& w, flags f)
      : m_pimpl (spimpl::make_unique_impl<detail::renderer_impl> (w.m_pimpl->sdl_window,
                                                                  get_flag (f, flags::SOFTWARE, sdl::renderer::flags::SOFTWARE),
                                                                  get_flag (f, flags::ACCELERATED, sdl::renderer::flags::ACCELERATED),
                                                                  get_flag (f, flags::PRESENTVSYNC, sdl::renderer::flags::PRESENTVSYNC),
                                                                  get_flag (f, flags::TARGETTEXTURE, sdl::renderer::flags::TARGETTEXTURE)
  )) {

  }

  renderer::renderer (const window& w)
      : m_pimpl (spimpl::make_unique_impl<detail::renderer_impl> (w.m_pimpl->sdl_window)) {

  }

  renderer::renderer (const surface& s)
      : m_pimpl (spimpl::make_unique_impl<detail::renderer_impl> (s.m_pimpl->surface)) {

  }

  renderer::renderer (std::unique_ptr<detail::renderer_impl> impl)
      : m_pimpl (std::move (impl)) {

  }

  renderer::operator bool () const {
    return m_pimpl->renderer.const_handle () != nullptr;
  }

  blend_mode renderer::blend () const {
    return cast (m_pimpl->renderer.blend ());
  }

  void renderer::blend (blend_mode bm) {
    m_pimpl->renderer.blend (cast (bm));
  }

  color renderer::active_color () const {
    return cast (m_pimpl->renderer.active_color ());
  }

  void renderer::active_color (const color& c) {
    m_pimpl->renderer.active_color (cast (c));
  }

  math::rect renderer::clip () const {
    return cast (m_pimpl->renderer.clip ());
  }

  void renderer::clip (const math::rect& area) {
    m_pimpl->renderer.clip (cast (area));
  }

  void renderer::disable_clippping () {
    m_pimpl->renderer.disable_clippping ();
  }

  bool renderer::clipping_enabled () const {
    return m_pimpl->renderer.clipping_enabled ();
  }

  bool renderer::integer_scaling () const {
    return m_pimpl->renderer.integer_scaling ();
  }

  void renderer::integer_scaling (bool enabled) {
    m_pimpl->renderer.integer_scaling (enabled);
  }

  std::pair<unsigned, unsigned> renderer::logical_size () const {
    return m_pimpl->renderer.logical_size ();
  }

  void renderer::logical_size (unsigned x, unsigned y) {
    m_pimpl->renderer.logical_size (x, y);
  }

  std::pair<float, float> renderer::scaling () const {
    return m_pimpl->renderer.scaling ();
  }

  void renderer::scaling (float x, float y) {
    m_pimpl->renderer.scaling (x, y);
  }

  math::rect renderer::viewport () const {
    return cast (m_pimpl->renderer.viewport ());
  }

  void renderer::viewport (const math::rect& area) {
    m_pimpl->renderer.viewport (cast (area));
  }

  void renderer::disable_viewport () {
    m_pimpl->renderer.disable_viewport ();
  }

  void renderer::read_pixels (const pixel_format& fmt, void* dst, std::size_t pitch) const {
    m_pimpl->renderer.read_pixels (sdl::pixel_format (fmt.value ()), dst, pitch);
  }

  void renderer::read_pixels (const math::rect& area, const pixel_format& fmt, void* dst, std::size_t pitch) const {
    m_pimpl->renderer.read_pixels (cast (area), sdl::pixel_format (fmt.value ()), dst, pitch);
  }

  std::optional<texture> renderer::target () const {
    if (const auto t = m_pimpl->renderer.target (); t) {
      auto tpr = std::make_unique<detail::texture_impl> (sdl::object<SDL_Texture> (t->const_handle (), false));
      return texture (std::move (tpr));
    }
    return std::nullopt;
  }

  void renderer::target (texture& t) {
    m_pimpl->renderer.target (t.m_pimpl->texture);
  }

  void renderer::restore_default_target () {
    m_pimpl->renderer.restore_default_target ();
  }

  std::pair<unsigned, unsigned> renderer::output_size () const {
    return m_pimpl->renderer.output_size ();
  }

  void renderer::clear () {
    m_pimpl->renderer.clear ();
  }

  void renderer::copy (const texture& t, flip flip_) {
    m_pimpl->renderer.copy (t.m_pimpl->texture, cast (flip_));
  }

  void renderer::copy (const texture& t, const math::rect& srcrect, flip flip_) {
    m_pimpl->renderer.copy (t.m_pimpl->texture, cast (srcrect), cast (flip_));
  }

  void renderer::copy (const texture& t, const math::rect& srcrect, const math::rect& dstrect, flip flip_) {
    m_pimpl->renderer.copy (t.m_pimpl->texture, cast (srcrect), cast (dstrect), cast (flip_));
  }

  void renderer::copy (const texture& t, const math::rect& srcrect, const math::rect& dstrect, double angle,
                       flip flip_) {
    m_pimpl->renderer.copy (t.m_pimpl->texture, cast (srcrect), cast (dstrect), angle, cast (flip_));
  }

  void renderer::copy (const texture& t, const math::rect& srcrect, const math::rect& dstrect, double angle,
                       const math::point2d& pt, flip flip_) {
    m_pimpl->renderer.copy (t.m_pimpl->texture, cast (srcrect), cast (dstrect), angle, cast (pt), cast (flip_));
  }

  void renderer::line (int x1, int y1, int x2, int y2) {
    m_pimpl->renderer.draw (x1, y1, x2, y2);
  }

  void renderer::line (const math::point2d& p1, const math::point2d& p2) {
    m_pimpl->renderer.draw (cast (p1), cast (p2));
  }

  void renderer::point (int x, int y) {
    m_pimpl->renderer.draw (x, y);
  }

  void renderer::point (const math::point2d& p) {
    m_pimpl->renderer.draw (cast (p));
  }

  void renderer::rectangle (const math::rect& rec) {
    m_pimpl->renderer.draw (cast (rec));
  }

  void renderer::rectangle_filled (const math::rect& rec) {
    m_pimpl->renderer.draw_filled (cast (rec));
  }

  void renderer::present () noexcept {
    m_pimpl->renderer.present ();
  }
  // ---------------------------------------------------------------------------
#if defined(_MSC_VER)
#pragma warning ( push )
#pragma warning ( disable : 4244)
#endif

  void renderer::hline (int x1, int x2, int y) {
    auto col = m_pimpl->renderer.active_color ();
    hlineRGBA (m_pimpl->renderer.handle (), x1, x2, y, col.r, col.b, col.b, col.a);
  }

  // ---------------------------------------------------------------------------
  void renderer::vline (int x, int y1, int y2) {
    auto col = m_pimpl->renderer.active_color ();
    vlineRGBA (m_pimpl->renderer.handle (), x, y1, y2, col.r, col.b, col.b, col.a);
  }
  // ---------------------------------------------------------------------------

  void renderer::rounded_rectangle (const math::rect& r, int radius) {
    auto col = m_pimpl->renderer.active_color ();
    roundedRectangleRGBA (m_pimpl->renderer.handle (),
                          r.point[0], r.point[1],
                          r.point[0] + r.dims[0], r.point[1] + r.dims[1], radius, col.r, col.b, col.b, col.a);
  }

  // ---------------------------------------------------------------------------
  void renderer::rounded_rectangle_filled (const math::rect& r, int radius) {
    auto col = m_pimpl->renderer.active_color ();
    roundedBoxRGBA (m_pimpl->renderer.handle (),
                    r.point[0], r.point[1],
                    r.point[0] + r.dims[0], r.point[1] + r.dims[1], radius, col.r, col.b, col.b, col.a);
  }

  // ---------------------------------------------------------------------------
  void renderer::aa_line (int x1, int y1, int x2, int y2) {
    auto col = m_pimpl->renderer.active_color ();
    aalineRGBA (m_pimpl->renderer.handle (), x1, y1, x2, y2, col.r, col.b, col.b, col.a);
  }

  // ---------------------------------------------------------------------------
  void renderer::aa_line (const math::point2d& a, const math::point2d& b) {
    aa_line (a[0], a[1], b[0], b[1]);
  }

  // ---------------------------------------------------------------------------
  void renderer::thick_line (int x1, int y1, int x2, int y2, int width) {
    auto col = m_pimpl->renderer.active_color ();
    thickLineRGBA (m_pimpl->renderer.handle (), x1, y1, x2, y2, width, col.r, col.b, col.b, col.a);
  }

  // ---------------------------------------------------------------------------
  void renderer::thick_line (const math::point2d& a, const math::point2d& b, int width) {
    thick_line (a[0], a[1], b[0], b[1], width);
  }

  // ---------------------------------------------------------------------------
  void renderer::circle (int x, int y, int radius) {
    auto col = m_pimpl->renderer.active_color ();
    circleRGBA (m_pimpl->renderer.handle (), x, y, radius, col.r, col.b, col.b, col.a);
  }

  // ---------------------------------------------------------------------------
  void renderer::circle (const math::point2d& p, int radius) {
    circle (p[0], p[1], radius);
  }

  // ---------------------------------------------------------------------------
  void renderer::aa_circle (int x, int y, int radius) {
    auto col = m_pimpl->renderer.active_color ();
    aacircleRGBA (m_pimpl->renderer.handle (), x, y, radius, col.r, col.b, col.b, col.a);
  }

  // ---------------------------------------------------------------------------
  void renderer::aa_circle (const math::point2d& p, int radius) {
    aa_circle (p[0], p[1], radius);
  }

  // ---------------------------------------------------------------------------
  void renderer::circle_filled (int x, int y, int radius) {
    auto col = m_pimpl->renderer.active_color ();
    filledCircleRGBA (m_pimpl->renderer.handle (), x, y, radius, col.r, col.b, col.b, col.a);
  }

  // ---------------------------------------------------------------------------
  void renderer::circle_filled (const math::point2d& p, int radius) {
    circle_filled (p[0], p[1], radius);
  }

  // ---------------------------------------------------------------------------
  void renderer::ellipse (int x, int y, int rx, int ry) {
    auto col = m_pimpl->renderer.active_color ();
    ellipseRGBA (m_pimpl->renderer.handle (), x, y, rx, ry, col.r, col.b, col.b, col.a);
  }

  // ---------------------------------------------------------------------------
  void renderer::ellipse (const math::point2d& p, int rx, int ry) {
    ellipse (p[0], p[1], rx, ry);
  }

  // ---------------------------------------------------------------------------
  void renderer::aa_ellipse (int x, int y, int rx, int ry) {
    auto col = m_pimpl->renderer.active_color ();
    aaellipseRGBA (m_pimpl->renderer.handle (), x, y, rx, ry, col.r, col.b, col.b, col.a);
  }

  // ---------------------------------------------------------------------------
  void renderer::aa_ellipse (const math::point2d& p, int rx, int ry) {
    aa_ellipse (p[0], p[1], rx, ry);
  }

  // ---------------------------------------------------------------------------
  void renderer::ellipse_filled (int x, int y, int rx, int ry) {
    auto col = m_pimpl->renderer.active_color ();
    filledEllipseRGBA (m_pimpl->renderer.handle (), x, y, rx, ry, col.r, col.b, col.b, col.a);
  }

  // ---------------------------------------------------------------------------
  void renderer::ellipse_filled (const math::point2d& p, int rx, int ry) {
    ellipse_filled (p[0], p[1], rx, ry);
  }

  // ---------------------------------------------------------------------------
  static int deg (float radian) {
    return radian * 180 / M_PI;
  }

  // ---------------------------------------------------------------------------
  void renderer::arc (int x, int y, int rad, float start, float end) {
    auto col = m_pimpl->renderer.active_color ();
    arcRGBA (m_pimpl->renderer.handle (), x, y, rad, deg (start), deg (end), col.r, col.b, col.b, col.a);
  }

  // ---------------------------------------------------------------------------
  void renderer::arc (const math::point2d& p, int rad, float start_angle, float end_angle) {
    arc (p[0], p[1], rad, start_angle, end_angle);
  }

  // ---------------------------------------------------------------------------
  void renderer::pie (int x, int y, int rad, float start_angle, float end_angle) {
    auto col = m_pimpl->renderer.active_color ();
    pieRGBA (m_pimpl->renderer.handle (), x, y, rad, deg (start_angle), deg (end_angle), col.r, col.b, col.b, col.a);
  }

  // ---------------------------------------------------------------------------
  void renderer::pie (const math::point2d& p, int rad, float start_angle, float end_angle) {
    pie (p[0], p[1], rad, start_angle, end_angle);
  }

  // ---------------------------------------------------------------------------
  void renderer::pie_filled (int x, int y, int rad, float start_angle, float end_angle) {
    auto col = m_pimpl->renderer.active_color ();
    filledPieRGBA (m_pimpl->renderer.handle (), x, y, rad, deg (start_angle), deg (end_angle), col.r, col.b, col.b, col.a);
  }

  // ---------------------------------------------------------------------------
  void renderer::pie_filled (const math::point2d& p, int rad, float start_angle, float end_angle) {
    pie_filled (p[0], p[1], rad, start_angle, end_angle);
  }

#if defined(_MSC_VER)
#pragma warning ( pop )
#endif
}