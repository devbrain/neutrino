//
// Created by igor on 26/06/2021.
//

#include <neutrino/hal/video/surface.hh>
#include <neutrino/hal/video/window.hh>
#include "surface_impl.hh"
#include "palette_impl.hh"
#include "hal/video/windows/window_impl.hh"
#include "hal/cast.hh"


#define SDL2_ROTOZOOM_SCOPE extern "C"
#include "hal/video/thirdparty/gfx/SDL2_rotozoom.h"

namespace neutrino::hal {
  surface::surface (std::unique_ptr<detail::surface_impl>&& impl)
      : m_pimpl (std::move (impl)) {

  }

  surface::surface (const window& w)
      : m_pimpl (spimpl::make_unique_impl<detail::surface_impl> (w.m_pimpl->sdl_window)) {
  }

  surface::surface(SDL_Surface* s)
      : m_pimpl (spimpl::make_unique_impl<detail::surface_impl> (sdl::object<SDL_Surface> (s, true))) {
  }

  surface::surface (unsigned width, unsigned height, pixel_format format)
      : m_pimpl (spimpl::make_unique_impl<detail::surface_impl> (width, height, sdl::pixel_format (format.value ()))) {
  }

  surface::surface (surface&& other) noexcept
      : m_pimpl (spimpl::make_unique_impl<detail::surface_impl> (std::move(other.m_pimpl->surface))) {
  }

  surface& surface::operator = (surface&& other) noexcept {
    m_pimpl->surface = (std::move (other.m_pimpl->surface));
    return *this;
  }

  surface surface::clone() const {
    auto s = m_pimpl->surface.clone();
    auto impl = std::make_unique<detail::surface_impl>(std::move(s));
    return surface(std::move(impl));
  }

  surface surface::make_rgba (unsigned width, unsigned height) {
    return {width, height, pixel_format::make_rgba_32bit ()};
  }

  surface surface::make_8bit (unsigned width, unsigned height) {
    return {width, height, pixel_format::make_8bit ()};
  }

  const SDL_Surface* surface::native() const {
    return m_pimpl->surface.handle();
  }

  SDL_Surface* surface::native() {
    return m_pimpl->surface.handle();
  }

  pixel_format surface::format () const {
    return pixel_format (m_pimpl->surface.get_pixel_format ().value ());
  }

  surface::operator bool () const {
    if (!m_pimpl) {
      return false;
    }
    return m_pimpl->surface.const_handle () != nullptr;
  }

  void surface::lock () noexcept {
    m_pimpl->surface.lock ();
  }

  void surface::unlock () noexcept {
    m_pimpl->surface.unlock ();
  }

  bool surface::must_lock () const noexcept {
    return m_pimpl->surface.must_lock ();
  }

  math::rect surface::clip () const {
    return cast (m_pimpl->surface.clip ());
  }

  bool surface::clip (const math::rect& r) {
    return m_pimpl->surface.clip (cast (r));
  }

  void surface::color_key (uint32_t c) {
    m_pimpl->surface.color_key (c);
  }

  void surface::color_key (const color& c) {
    m_pimpl->surface.color_key (cast (c));
  }

  void surface::disable_color_key () {
    m_pimpl->surface.disable_color_key ();
  }

  std::optional<uint32_t> surface::color_key () const {
    return m_pimpl->surface.color_key ();
  }

  blend_mode surface::blend () const {
    return cast (m_pimpl->surface.blend ());
  }

  void surface::blend (blend_mode v) {
    auto bm = cast (v);
    m_pimpl->surface.blend (bm);
  }

  uint8_t surface::alpha_mod () const {
    return m_pimpl->surface.alpha_mod ();
  }

  void surface::alpha_mod (uint8_t v) {
    m_pimpl->surface.alpha_mod (v);
  }

  void surface::rle_acceleration (bool enabled) {
    m_pimpl->surface.rle_acceleration (enabled);
  }

  void surface::color_mod (uint8_t r, uint8_t g, uint8_t b) {
    m_pimpl->surface.color_mod (r, g, b);
  }

  std::optional<color> surface::color_mod () const {
    return cast<color> (m_pimpl->surface.color_mod ());
  }

  void surface::blit (const math::rect& srect, surface& dst, const math::rect& drect) const {
    m_pimpl->surface.blit (cast (srect), dst.m_pimpl->surface, cast (drect));
  }

  void surface::blit_scaled (const math::rect& srect, surface& dst) const {
    m_pimpl->surface.blit_scaled (cast (srect), dst.m_pimpl->surface);
  }

  void surface::blit_scaled (surface& dst) const {
    m_pimpl->surface.blit_scaled (dst.m_pimpl->surface);
  }

  void surface::blit_scaled (const math::rect& srect, surface& dst, const math::rect& drect) const {
    m_pimpl->surface.blit (cast (srect), dst.m_pimpl->surface, cast (drect));
  }

  void surface::blit (const math::rect& srect, surface& dst, const math::point2d& dpoint) const {
    m_pimpl->surface.blit (cast (srect), dst.m_pimpl->surface, cast (dpoint));
  }

  void surface::blit (const math::rect& srect, surface& dst) const {
    m_pimpl->surface.blit (cast (srect), dst.m_pimpl->surface);
  }

  void surface::blit (surface& dst) const {
    m_pimpl->surface.blit (dst.m_pimpl->surface);
  }

  std::tuple<void*, std::size_t, unsigned, unsigned> surface::pixels_data () const {
    return m_pimpl->surface.pixels_data ();
  }

  std::tuple<unsigned, unsigned> surface::dimensions() const noexcept {
    const auto* s = m_pimpl->surface.handle();
    return {static_cast<unsigned> (s->w), static_cast<unsigned> (s->h)};
  }

  void surface::fill (const math::rect& r, uint32_t c) {
    m_pimpl->surface.fill (cast (r), c);
  }

  void surface::fill (const math::rect& r, const color& c) {
    m_pimpl->surface.fill (cast (r), cast (c));
  }

  void surface::fill (uint32_t c) {
    m_pimpl->surface.fill (c);
  }

  void surface::fill (const color& c) {
    m_pimpl->surface.fill (cast (c));
  }

  surface surface::convert (const pixel_format& fmt) const {
    return surface{std::make_unique<detail::surface_impl> (m_pimpl->surface.convert (sdl::pixel_format (fmt.value ())))};
  }

  void surface::set_palette(const palette& pal) {
    m_pimpl->surface.set_palette (pal.m_pimpl->palette);
  }

  palette surface::get_palette() const {
    return {std::make_unique<detail::palette_impl>(m_pimpl->surface.get_palette())};
  }

  void surface::put_pixel(int x, int y, uint32_t pixel) {
    auto sdl_surface = m_pimpl->surface.handle();
    int bpp = sdl_surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)sdl_surface->pixels + y * sdl_surface->pitch + x * bpp;

    switch(bpp) {
      case 1:
        *p = pixel;
        break;

      case 2:
        *(Uint16 *)p = pixel;
        break;

      case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
          p[0] = (pixel >> 16) & 0xff;
          p[1] = (pixel >> 8) & 0xff;
          p[2] = pixel & 0xff;
        } else {
          p[0] = pixel & 0xff;
          p[1] = (pixel >> 8) & 0xff;
          p[2] = (pixel >> 16) & 0xff;
        }
        break;

      case 4:
        *(Uint32 *)p = pixel;
        break;
    }
  }

  void surface::swap(surface& other) {
    std::swap(m_pimpl, other.m_pimpl);
  }

  uint32_t surface::get_pixel(int x, int y) const {
    auto sdl_surface = m_pimpl->surface.handle();
    int bpp = sdl_surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)sdl_surface->pixels + y * sdl_surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        return *p;
        break;

    case 2:
        return *(Uint16 *)p;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
        break;

    case 4:
        return *(Uint32 *)p;
        break;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
  }

  pixel_format surface::get_pixel_format () const {
    return pixel_format{m_pimpl->surface.get_pixel_format ().value ()};
  }

  uint32_t surface::map_color (const color& c) {
    return m_pimpl->surface.map_color (cast (c));
  }

  surface surface::roto_zoom (double angle, double zoomx, double zoomy, bool smooth) {
    SDL_Surface* s = rotozoomSurfaceXY (m_pimpl->surface.handle (), M_PI * angle / 180, zoomx, zoomy, smooth ? 1 : 0);
    auto ptr = std::make_unique<detail::surface_impl> (sdl::object<SDL_Surface> (s, true));
    return surface{std::move (ptr)};
  }
}