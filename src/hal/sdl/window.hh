//
// Created by igor on 08/06/2021.
//

#ifndef NEUTRINO_SDL_WINDOW_HH
#define NEUTRINO_SDL_WINDOW_HH

#include <string>
#include <cstdint>
#include <tuple>

#include <hal/sdl/sdl2.hh>
#include <hal/sdl/call.hh>
#include <hal/sdl/object.hh>

namespace neutrino::sdl {
  class window : public object<SDL_Window> {
    public:
      enum class flags_t : uint32_t {
          FULL_SCREEN = SDL_WINDOW_FULLSCREEN, // fullscreen window
          FULL_SCREEN_DESKTOP = SDL_WINDOW_FULLSCREEN_DESKTOP, // fullscreen window at the current desktop resolution
          OPENGL = SDL_WINDOW_OPENGL, // window usable with OpenGL context
          VULKAN = SDL_WINDOW_VULKAN, // window usable with a Vulkan instance
          SHOWN = SDL_WINDOW_SHOWN, //  window is visible
          HIDDEN = SDL_WINDOW_HIDDEN, // window is not visible
          BORDERLESS = SDL_WINDOW_BORDERLESS, // no window decoration
          RESIZABLE = SDL_WINDOW_RESIZABLE, // window can be resized
          MINIMIZED = SDL_WINDOW_MINIMIZED, // window is minimized
          MAXIMIZED = SDL_WINDOW_MAXIMIZED, // window is maximized
          INPUT_GRABBED = SDL_WINDOW_INPUT_GRABBED, // window has grabbed input focus
          INPUT_FOCUS = SDL_WINDOW_INPUT_FOCUS, // window has input focus
          MOUSE_FOCUS = SDL_WINDOW_MOUSE_FOCUS, // window has mouse focus
          HIGHDPI = SDL_WINDOW_ALLOW_HIGHDPI, // window should be created in high-DPI mode if supported (>= SDL 2.0.1)
          MOUSE_CAPTURE = SDL_WINDOW_MOUSE_CAPTURE, // window has mouse captured (unrelated to INPUT_GRABBED, >= SDL 2.0.4)

          NONE = 0
      };
    public:
      static window by_id (uint32_t window_id) {
        return window (object<SDL_Window> (SAFE_SDL_CALL(SDL_GetWindowFromID, window_id), false));
      }

      window () = default;

      template <typename ... Args,
          typename std::enable_if<(std::is_same_v<flags_t, Args> && ...), int>::type = 0>
      window (int w, int h, Args...args);

      template <typename ... Args,
          typename std::enable_if<(std::is_same_v<flags_t, Args> && ...), int>::type = 0>
      window (const std::string &title, int w, int h, Args...args);

      template <typename ... Args,
          typename std::enable_if<(std::is_same_v<flags_t, Args> && ...), int>::type = 0>
      window (int x, int y, int w, int h, Args...args);

      template <typename ... Args,
          typename std::enable_if<(std::is_same_v<flags_t, Args> && ...), int>::type = 0>
      window (const std::string &title, int x, int y, int w, int h, Args...args);

      window (object <SDL_Window> &&other);
      window &operator= (object <SDL_Window> &&other);

      [[nodiscard]] uint32_t id () const;

      // w, h
      [[nodiscard]] std::tuple<int, int> size () const noexcept;
      void size (int w, int h) noexcept;

      [[nodiscard]] std::tuple<int, int> position () const noexcept;
      void position (int x, int y) noexcept;

      // top, left, bottom, right
      [[nodiscard]] std::tuple<int, int, int, int> borders_size () const noexcept;

      [[nodiscard]] float brightness () const noexcept;
      void brightness (float v) noexcept;

      [[nodiscard]] int display_index () const;

      void user_data (const char *name, void *data) noexcept;
      [[nodiscard]] void *user_data (const char *name) const noexcept;

      void minimize () noexcept;
      void maximize () noexcept;
      void raise () noexcept;
      void show () noexcept;
      void hide () noexcept;

      [[nodiscard]] std::string title () const;
      void title (const std::string &v) noexcept;

      void update_surface ();
      void swap_opengl_window () noexcept;
  };
}
// ======================================================================================================
// Implementation
// ======================================================================================================
namespace neutrino::sdl {
  template <typename ... Args,
      typename std::enable_if<(std::is_same_v<window::flags_t, Args> && ...), int>::type>
  window::window (int w, int h, Args...args)
      :object<SDL_Window> (SAFE_SDL_CALL(
                               SDL_CreateWindow,
                               "",
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               w,
                               h,
                               (static_cast<std::uint32_t>(args) | ... | 0u)
                           )) {
  }
  // --------------------------------------------------------------------------------------------
  template <typename ... Args,
      typename std::enable_if<(std::is_same_v<window::flags_t, Args> && ...), int>::type>
  window::window (const std::string &title, int w, int h, Args...args)
      :object<SDL_Window> (SAFE_SDL_CALL(
                               SDL_CreateWindow,
                               title.c_str (),
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               w,
                               h,
                               (static_cast<std::uint32_t>(args) | ... | 0u)
                           )) {
  }
  // --------------------------------------------------------------------------------------------
  template <typename ... Args,
      typename std::enable_if<(std::is_same_v<window::flags_t, Args> && ...), int>::type>
  window::window (int x, int y, int w, int h, Args...args)
      :object<SDL_Window> (SAFE_SDL_CALL(
                               SDL_CreateWindow,
                               "",
                               x,
                               y,
                               w,
                               h,
                               (static_cast<std::uint32_t>(args) | ... | 0u)
                           )) {
  }
  // --------------------------------------------------------------------------------------------
  template <typename ... Args,
      typename std::enable_if<(std::is_same_v<window::flags_t, Args> && ...), int>::type>
  window::window (const std::string &title, int x, int y, int w, int h, Args...args)
      :object<SDL_Window> (SAFE_SDL_CALL(
                               SDL_CreateWindow,
                               title.c_str (),
                               x,
                               y,
                               w,
                               h,
                               (static_cast<std::uint32_t>(args) | ... | 0u)
                           ), true) {
  }
  // --------------------------------------------------------------------------------------------
  inline
  window::window (object <SDL_Window> &&other)
      : object<SDL_Window> (std::move (other)) {

  }
  // --------------------------------------------------------------------------------------------
  inline
  window &window::operator= (object <SDL_Window> &&other) {
    object<SDL_Window>::operator= (std::move (other));
    return *this;
  }
  // --------------------------------------------------------------------------------------------
  inline
  uint32_t window::id () const {
    auto ret = SDL_GetWindowID (const_handle ());
    if (ret == 0) {
      RAISE_SDL_EX("SDL_GetWindowID() == 0");
    }
    return ret;
  }
  // --------------------------------------------------------------------------------------------
  inline
  std::tuple<int, int> window::size () const noexcept {
    int w, h;
    SDL_GetWindowSize (const_handle (), &w, &h);
    return {w, h};
  }
  // --------------------------------------------------------------------------------------------
  inline
  void window::size (int w, int h) noexcept {
    SDL_SetWindowSize (handle (), w, h);
  }
  // --------------------------------------------------------------------------------------------
  inline
  std::tuple<int, int, int, int> window::borders_size () const noexcept {
    int top, left, bottom, right;
    SDL_GetWindowBordersSize (const_handle (), &top, &left, &bottom, &right);
    return {top, left, bottom, right};
  }
  // --------------------------------------------------------------------------------------------
  inline
  float window::brightness () const noexcept {
    return SDL_GetWindowBrightness (const_handle ());
  }
  // --------------------------------------------------------------------------------------------
  inline
  void window::brightness (float v) noexcept {
    SDL_SetWindowBrightness (handle (), v);
  }
  // --------------------------------------------------------------------------------------------
  inline
  int window::display_index () const {
    return SAFE_SDL_CALL(SDL_GetWindowDisplayIndex, const_handle ());
  }
  // --------------------------------------------------------------------------------------------
  inline
  void window::user_data (const char *name, void *data) noexcept {
    SDL_SetWindowData (handle (), name, data);
  }
  // --------------------------------------------------------------------------------------------
  inline
  void *window::user_data (const char *name) const noexcept {
    return SDL_GetWindowData (const_handle (), name);
  }
  // --------------------------------------------------------------------------------------------
  inline
  void window::minimize () noexcept {
    SDL_MinimizeWindow (handle ());
  }
  // --------------------------------------------------------------------------------------------
  inline
  void window::maximize () noexcept {
    SDL_MaximizeWindow (handle ());
  }
  // --------------------------------------------------------------------------------------------
  inline
  void window::raise () noexcept {
    SDL_RaiseWindow (handle ());
  }
  // --------------------------------------------------------------------------------------------
  inline
  std::tuple<int, int> window::position () const noexcept {
    int x, y;
    SDL_GetWindowPosition (const_handle (), &x, &y);
    return {x, y};
  }
  // --------------------------------------------------------------------------------------------
  inline
  void window::position (int x, int y) noexcept {
    SDL_SetWindowPosition (handle (), x, y);
  }
  // --------------------------------------------------------------------------------------------
  inline
  void window::show () noexcept {
    SDL_ShowWindow (handle ());
  }
  // --------------------------------------------------------------------------------------------
  inline
  void window::hide () noexcept {
    SDL_HideWindow (handle ());
  }
  // --------------------------------------------------------------------------------------------
  inline
  std::string window::title () const {
    const char *v = SDL_GetWindowTitle (const_handle ());
    if (v) {
      return v;
    }
    return {};
  }
  // --------------------------------------------------------------------------------------------
  inline
  void window::title (const std::string &v) noexcept {
    SDL_SetWindowTitle (handle (), v.c_str ());
  }
  // --------------------------------------------------------------------------------------------
  inline
  void window::update_surface () {
    SAFE_SDL_CALL(SDL_UpdateWindowSurface, handle ());
  }
  // --------------------------------------------------------------------------------------------
  inline
  void window::swap_opengl_window () noexcept {
    SDL_GL_SwapWindow (handle ());
  }
}
#endif
