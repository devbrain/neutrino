//
// Created by igor on 18/04/2022.
//

#include <neutrino/kernel/application_description.hh>

#include <utility>

namespace neutrino {
  unsigned main_window_description::width () const noexcept {
    return m_width;
  }

  unsigned main_window_description::height () const noexcept {
    return m_height;
  }

  std::string main_window_description::title () const noexcept {
    return m_title;
  }

  std::optional<math::point2d> main_window_description::position () const noexcept {
    return m_pos;
  }

  main_window_description::main_window_description (unsigned int w, unsigned int h)
      : m_width(w),
        m_height(h),
        m_resizable(false),
        m_fullscreen(false) {
  }

  void main_window_description::title (std::string ttl) {
    m_title = std::move (ttl);
  }

  void main_window_description::position (int x, int y) {
    m_pos = {x, y};
  }

  void main_window_description::resizable (bool f) {
    m_resizable = f;
  }

  bool main_window_description::resizable () const noexcept {
    return m_resizable;
  }

  void main_window_description::fullscreen (bool f) {
    m_fullscreen = f;
  }

  bool main_window_description::fullscreen () const noexcept {
    return m_fullscreen;
  }

  main_window_description application_description::get_main_window_description () const noexcept {
    return m_main_window_descr;
  }

  application_description::application_description (main_window_description mw)
      : m_main_window_descr (std::move(mw)),
        m_fps(60) {

  }

  application_description::application_description (main_window_description mw, unsigned int fps)
      : m_main_window_descr (std::move(mw)),
        m_fps(fps) {
  }

  unsigned application_description::fps () const noexcept {
    return m_fps;
  }
}