//
// Created by igor on 06/06/2020.
//

#ifndef NEUTRINO_SDL_TEXTURE_HH
#define NEUTRINO_SDL_TEXTURE_HH

#include <optional>

#include <hal/sdl/sdl2.hh>
#include <hal/sdl/object.hh>
#include <hal/sdl/pixel_format.hh>
#include <hal/sdl/call.hh>
#include <hal/sdl/surface.hh>

namespace neutrino::sdl {
  class texture : public object<SDL_Texture> {
    public:
      enum class access : uint32_t {
          STATIC = SDL_TEXTUREACCESS_STATIC,
          STREAMING = SDL_TEXTUREACCESS_STREAMING,
          TARGET = SDL_TEXTUREACCESS_TARGET
      };
    public:
      texture () = default;

      template <typename ... Args,
          typename std::enable_if<(std::is_same_v<access, Args> && ...), int>::type = 0>
      texture (const object <SDL_Renderer>& r, const pixel_format& format, unsigned w, unsigned h, Args&& ... flags);

      texture (const object <SDL_Renderer>& r, const object <SDL_Surface>& s);

      explicit texture (object <SDL_Texture>&& other);
      texture& operator = (object <SDL_Texture>&& other) noexcept;

      // returns: pixel format, texture_access, w, h
      [[nodiscard]] std::tuple<pixel_format, access, unsigned, unsigned> query () const;

      [[nodiscard]] uint8_t alpha () const;
      void alpha (uint8_t a);

      [[nodiscard]] blend_mode blend () const;
      void blend (blend_mode bm);

      [[nodiscard]] std::optional<color> color_mod () const;
      void color_mod (const color& c);

      /*
      Use this function to lock  whole texture for write-only pixel access.
      returns: pointer to pixels and pitch, ie., the length of one row in bytes
      */
      [[nodiscard]] std::pair<void*, std::size_t> lock () const;
      /*
      Use this function to lock a portion of the texture for write-only pixel access.
      returns: pointer to pixels and pitch, ie., the length of one row in bytes
      */
      [[nodiscard]] std::pair<void*, std::size_t> lock (const rect& r) const;

      void unlock () const;

      // slow updates
      void update (const void* pixels, std::size_t pitch);
      void update (const rect& area, const void* pixels, std::size_t pitch);
  };
} // ns sdl
// =================================================================================================================
// Implementation
// =================================================================================================================
namespace neutrino::sdl {
  template <typename ... Args,
      typename std::enable_if<(std::is_same_v<texture::access, Args> && ...), int>::type>
  texture::texture (const object <SDL_Renderer>& r,
                    const pixel_format& format,
                    unsigned w, unsigned h, Args&& ... flags)
      : object<SDL_Texture> (SAFE_SDL_CALL(SDL_CreateTexture,
                                           const_cast<SDL_Renderer*>(r.handle ()),
                                           format.value (),
                                           (static_cast<std::uint32_t>(flags) | ... | 0u),
                                           static_cast<int>(w),
                                           static_cast<int>(h)), true) {

  }

  // ---------------------------------------------------------------------------------------------------------------
  inline
  texture::texture (const object <SDL_Renderer>& r, const object <SDL_Surface>& s)
      : object<SDL_Texture> (SAFE_SDL_CALL(SDL_CreateTextureFromSurface,
                                           const_cast<SDL_Renderer*>(r.handle ()),
                                           const_cast<SDL_Surface*>(s.handle ())
                             ), true) {

  }

  // ---------------------------------------------------------------------------------------------------------------
  inline
  texture::texture (object <SDL_Texture>&& other)
      : object<SDL_Texture> (std::move (other)) {

  }

  // ---------------------------------------------------------------------------------------------------------------
  inline
  texture& texture::operator = (object <SDL_Texture>&& other) noexcept {
    object<SDL_Texture>::operator = (std::move (other));
    return *this;
  }

  // ---------------------------------------------------------------------------------------------------------------
  inline
  std::tuple<pixel_format, texture::access, unsigned, unsigned> texture::query () const {
    uint32_t format;
    int w;
    int h;
    int acc;
    SAFE_SDL_CALL(SDL_QueryTexture, const_handle (), &format, &acc, &w, &h);
    return {pixel_format (format), (access) acc, static_cast<unsigned>(w), static_cast<unsigned>(h)};
  }

  // ---------------------------------------------------------------------------------------------------------------
  inline
  uint8_t texture::alpha () const {
    uint8_t a;
    SAFE_SDL_CALL(SDL_GetTextureAlphaMod, const_handle (), &a);
    return a;
  }

  // ---------------------------------------------------------------------------------------------------------------
  inline
  void texture::alpha (uint8_t a) {
    SAFE_SDL_CALL(SDL_SetTextureAlphaMod, handle (), a);
  }

  // ---------------------------------------------------------------------------------------------------------------
  inline
  blend_mode texture::blend () const {
    SDL_BlendMode x;
    SAFE_SDL_CALL(SDL_GetTextureBlendMode, const_handle (), &x);
    return static_cast<blend_mode>(x);
  }

  // ---------------------------------------------------------------------------------------------------------------
  inline
  void texture::blend (blend_mode bm) {
    SAFE_SDL_CALL(SDL_SetTextureBlendMode, handle (), static_cast<SDL_BlendMode>(bm));
  }

  // ---------------------------------------------------------------------------------------------------------------
  inline
  std::optional<color> texture::color_mod () const {
    color c{0, 0, 0, 0};
    if (0 == SDL_GetTextureColorMod (const_handle (), &c.r, &c.g, &c.b)) {
      return c;
    }
    return std::nullopt;
  }

  // ---------------------------------------------------------------------------------------------------------------
  inline
  void texture::color_mod (const color& c) {
    SAFE_SDL_CALL(SDL_SetTextureColorMod, handle (), c.r, c.g, c.b);
  }

  // ---------------------------------------------------------------------------------------------------------------
  inline
  std::pair<void*, std::size_t> texture::lock () const {
    void* pixels;
    int pitch;
    SAFE_SDL_CALL(SDL_LockTexture, const_cast<SDL_Texture*>(handle ()), nullptr, &pixels, &pitch);
    return {pixels, static_cast<std::size_t>(pitch)};
  }

  // ---------------------------------------------------------------------------------------------------------------
  inline
  std::pair<void*, std::size_t> texture::lock (const rect& r) const {
    void* pixels;
    int pitch;
    SAFE_SDL_CALL(SDL_LockTexture, const_cast<SDL_Texture*>(handle ()), &r, &pixels, &pitch);
    return {pixels, static_cast<std::size_t>(pitch)};
  }

  // ---------------------------------------------------------------------------------------------------------------
  inline
  void texture::unlock () const {
    SAFE_SDL_CALL(SDL_UnlockTexture, const_handle ());
  }

  // ---------------------------------------------------------------------------------------------------------------
  inline
  void texture::update (const void* pixels, std::size_t pitch) {
    SAFE_SDL_CALL(SDL_UpdateTexture, handle (), nullptr, pixels, static_cast<int>(pitch));
  }

  // ---------------------------------------------------------------------------------------------------------------
  inline
  void texture::update (const rect& area, const void* pixels, std::size_t pitch) {
    SAFE_SDL_CALL(SDL_UpdateTexture, handle (), &area, pixels, static_cast<int>(pitch));
  }
}
#endif
