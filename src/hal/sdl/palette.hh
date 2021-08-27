//
// Created by igor on 01/06/2020.
//

#ifndef NEUTRINO_SDL_PALETTE_HH
#define NEUTRINO_SDL_PALETTE_HH

#include <hal/sdl/sdl2.hh>
#include <hal/sdl/object.hh>
#include <hal/sdl/color.hh>
#include <hal/sdl/iterator.hh>
#include <hal/sdl/call.hh>
#include <hal/sdl/rtc.hh>

namespace neutrino::sdl {
  class palette : public object<SDL_Palette> {
    public:
      using iterator = detail::raw_iterator<color>;
      using const_iterator = detail::raw_iterator<const color>;
    public:
      palette ();
      explicit palette (std::size_t num_colors);
      palette (const color *first, const color *last);

      explicit palette (const object<SDL_Surface> &surface);
      explicit palette (const object<SDL_PixelFormat> &format);

      [[nodiscard]] iterator begin () noexcept;
      [[nodiscard]] iterator end () noexcept;

      [[nodiscard]] const_iterator begin () const noexcept;
      [[nodiscard]] const_iterator end () const noexcept;

      [[nodiscard]] std::size_t size () const noexcept;
      [[nodiscard]] bool empty () const noexcept;

      [[nodiscard]] color &operator[] (std::size_t n);
      [[nodiscard]] const color &operator[] (std::size_t n) const;

      void fill (const color *first, const color *last);
      void fill (const_iterator first, const_iterator last);
      void fill (iterator first, iterator last);
    private:
      static SDL_Palette *_get_from_surface (const object<SDL_Surface> &s) {
        RTC_IN_PALETTE(s && s->format, "Invalid surface");
        if (!s || !s->format) {
          return nullptr;
        }
        return s->format->palette;
      }
      static SDL_Palette *_get_from_format (const object<SDL_PixelFormat> &s) {
        RTC_IN_PALETTE(s, "Invalid pixel_format");
        if (!s) {
          return nullptr;
        }
        return s->palette;
      }

  };
}
// ================================================================================================
// Implementation
// ================================================================================================
namespace neutrino::sdl {
  inline
  palette::palette ()
      : object<SDL_Palette> (nullptr, false) {

  }
  // -------------------------------------------------------------------------------------------
  inline
  palette::palette (const color *first, const color *last)
      : object<SDL_Palette> (SAFE_SDL_CALL(SDL_AllocPalette, static_cast<int>(std::distance (first, last))), true) {
    if (0 != SDL_SetPaletteColors (handle (), first, 0, static_cast<int>(size ()))) {
      RAISE_SDL_EX();
    }
  }
  // -------------------------------------------------------------------------------------------
  inline
  palette::palette (std::size_t num_colors)
      : object<SDL_Palette> (SAFE_SDL_CALL(SDL_AllocPalette, static_cast<int>(num_colors)), true) {
    auto *colors = reinterpret_cast<color *>(handle ()->colors);
    for (std::size_t i = 0; i < num_colors; i++) {
      colors[i].r = 0;
      colors[i].g = 0;
      colors[i].b = 0;
    }
  }
  // -------------------------------------------------------------------------------------------
  inline
  palette::palette (const object<SDL_Surface> &surface)
      : object<SDL_Palette> (_get_from_surface ((surface)), false) {

  }
  // -------------------------------------------------------------------------------------------
  inline
  palette::palette (const object<SDL_PixelFormat> &format)
      : object<SDL_Palette> (_get_from_format ((format)), false) {

  }
  // -------------------------------------------------------------------------------------------
  inline
  palette::iterator palette::begin () noexcept {
    if (is_null ()) {
      return iterator (nullptr);
    }
    auto *colors = reinterpret_cast<color *>(handle ()->colors);
    return iterator (colors);
  }
  // -------------------------------------------------------------------------------------------
  inline
  palette::iterator palette::end () noexcept {
    if (is_null ()) {
      return iterator (nullptr);
    }
    auto *colors = reinterpret_cast<color *>(handle ()->colors);
    return iterator (colors + handle ()->ncolors);
  }
  // -------------------------------------------------------------------------------------------
  inline
  palette::const_iterator palette::begin () const noexcept {
    if (is_null ()) {
      return const_iterator (nullptr);
    }
    auto *colors = reinterpret_cast<color *>(handle ()->colors);
    return const_iterator (colors);
  }
  // -------------------------------------------------------------------------------------------
  inline
  palette::const_iterator palette::end () const noexcept {
    if (is_null ()) {
      return const_iterator (nullptr);
    }
    auto *colors = reinterpret_cast<color *>(handle ()->colors);
    return const_iterator (colors + handle ()->ncolors);
  }
  // -------------------------------------------------------------------------------------------
  inline
  std::size_t palette::size () const noexcept {
    if (is_null ()) {
      return 0;
    }
    return static_cast<std::size_t>(handle ()->ncolors);
  }
  // -------------------------------------------------------------------------------------------
  inline
  bool palette::empty () const noexcept {
    return size () == 0;
  }
  // -------------------------------------------------------------------------------------------
  inline
  color &palette::operator[] (std::size_t n) {
    RTC_IN_PALETTE(!is_null (), "Empty palette");
    RTC_IN_PALETTE(n < size (), "Index", n, "<", size ());
    RTC_IN_PALETTE(handle ()->colors != nullptr, "Colors are null");

    auto *colors = reinterpret_cast<color *>(handle ()->colors);

    return colors[n];
  }
  // -------------------------------------------------------------------------------------------
  inline
  const color &palette::operator[] (std::size_t n) const {
    RTC_IN_PALETTE(!is_null (), "Empty palette");
    RTC_IN_PALETTE(n < size (), "Index", n, "<", size ());
    RTC_IN_PALETTE(handle ()->colors != nullptr, "Colors are null");

    auto *colors = reinterpret_cast<color *>(handle ()->colors);

    return colors[n];
  }
  // -------------------------------------------------------------------------------------------
  inline
  void palette::fill (const color *first, const color *last) {
    const auto n = std::distance (first, last);
    RTC_IN_PALETTE (
        static_cast<std::size_t>(n) <= size (), "Palette of size ", size (), "is too small to hold ", n, "entries");
    if (0 != SDL_SetPaletteColors (handle (), first, 0, static_cast<int>(n))) {
      RAISE_SDL_EX();
    }
  }
  // -------------------------------------------------------------------------------------------
  inline
  void palette::fill (const_iterator first, const_iterator last) {
    fill (first.getConstPtr (), last.getConstPtr ());
  }
  // -------------------------------------------------------------------------------------------
  inline
  void palette::fill (iterator first, iterator last) {
    fill (first.getConstPtr (), last.getConstPtr ());
  }
}

#endif
