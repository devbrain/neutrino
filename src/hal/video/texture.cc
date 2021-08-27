//
// Created by igor on 30/06/2021.
//

#include <neutrino/hal/video/texture.hh>
#include <neutrino/hal/video/renderer.hh>
#include <neutrino/hal/video/surface.hh>
#include <neutrino/utils/exception.hh>
#include <hal/cast.hh>

#include "texture_impl.hh"
#include "renderer_impl.hh"
#include "surface_impl.hh"

namespace neutrino::hal {

  static sdl::texture::access cast_flags (const texture::access &flags) {
    switch (flags) {
      case texture::access::STREAMING:
        return sdl::texture::access::STREAMING;
      case texture::access::STATIC:
        return sdl::texture::access::STATIC;
      case texture::access::TARGET:
        return sdl::texture::access::TARGET;
      default:
        RAISE_EX("Should not be here");
    }
  }

  texture::texture () = default;

  texture::texture (const renderer &r, const pixel_format &format, unsigned w, unsigned h, access flags)
      : m_pimpl (spimpl::make_unique_impl<detail::texture_impl> (r.m_pimpl->renderer, sdl::pixel_format (format.value ()), w, h, cast_flags (flags))) {

  }

  texture::texture (const renderer &r, const surface &s)
      : m_pimpl (spimpl::make_unique_impl<detail::texture_impl> (r.m_pimpl->renderer, s.m_pimpl->surface)) {

  }

  texture::texture (std::unique_ptr<detail::texture_impl> &&t)
      : m_pimpl (std::move (t)) {

  }

  texture::operator bool () const {
    return m_pimpl->texture.handle () != nullptr;
  }

  // returns: pixel format, texture_access, w, h
  std::tuple<pixel_format, texture::access, unsigned, unsigned> texture::query () const {
    auto[px, acc, w, h] = m_pimpl->texture.query ();
    texture::access a = (acc == sdl::texture::access::STATIC) ? access::STATIC :
                        (acc == sdl::texture::access::STREAMING) ? access::STREAMING : access::TARGET;
    return {pixel_format{px.value ()}, a, w, h};
  }

  uint8_t texture::alpha () const {
    return m_pimpl->texture.alpha ();
  }

  void texture::alpha (uint8_t a) {
    m_pimpl->texture.alpha (a);
  }

  blend_mode texture::blend () const {
    return cast (m_pimpl->texture.blend ());
  }

  void texture::blend (blend_mode bm) {
    m_pimpl->texture.blend (cast (bm));
  }

  std::optional<color> texture::color_mod () const {
    return cast<color> (m_pimpl->texture.color_mod ());
  }

  void texture::color_mod (const color &c) {
    m_pimpl->texture.color_mod (cast (c));
  }

  /*
  Use this function to lock  whole texture for write-only pixel access.
  returns: pointer to pixels and pitch, ie., the length of one row in bytes
  */
  std::pair<void *, std::size_t> texture::lock () const {
    return m_pimpl->texture.lock ();
  }
  /*
  Use this function to lock a portion of the texture for write-only pixel access.
  returns: pointer to pixels and pitch, ie., the length of one row in bytes
  */
  std::pair<void *, std::size_t> texture::lock (const math::rect &r) const {
    return m_pimpl->texture.lock (cast (r));
  }

  void texture::unlock () const {
    m_pimpl->texture.unlock ();
  }

  // slow updates
  void texture::update (const void *pixels, std::size_t pitch) {
    m_pimpl->texture.update (pixels, pitch);
  }

  void texture::update (const math::rect &area, const void *pixels, std::size_t pitch) {
    m_pimpl->texture.update (cast (area), pixels, pitch);
  }

  uint32_t texture::map_rgba (const color &c) const {
    return SDL_MapRGBA (m_pimpl->format.const_handle (), c.r, c.g, c.b, c.a);
  }

  uint32_t texture::map_rgb (const color &c) const {
    return SDL_MapRGB (m_pimpl->format.const_handle (), c.r, c.g, c.b);
  }
}