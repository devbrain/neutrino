//
// Created by igor on 06/07/2021.
//

#ifndef NEUTRINO_SCENE_WINDOW_HH
#define NEUTRINO_SCENE_WINDOW_HH

#include <neutrino/engine/scene_manager.hh>
#include <neutrino/hal/video/window.hh>
#include <neutrino/utils/observer.hh>
#include <neutrino/engine/events.hh>

namespace neutrino::engine {
  template <typename Base>
  class scene_window : public Base, public utils::observer<events::full_screen> {
    public:
      scene_window (int w, int h);
      scene_window (int w, int h, hal::window_flags_t flags);
      void show ();
      void show (int x, int y);
      void on_event (const events::full_screen&) override;

    private:
      void on_input_focus_changed (bool keyboard_focus, bool mouse_focus) override;
      void on_visibility_change (bool is_visible) override;
      void on_keyboard_input (const events::keyboard& ev) override;
      void on_pointer_input (const events::pointer& ev) override;
      void on_window_invalidate () override;
    private:
      int m_width;
      int m_height;
  };

  template <typename Base>
  scene_window<Base>::scene_window (int w, int h)
      : m_width (w), m_height (h) {
    scene_manager::instance ().attach (this);
  }

  // -----------------------------------------------------------------------------------------------------------
  template <typename Base>
  scene_window<Base>::scene_window (int w, int h, hal::window_flags_t flags)
      : Base (flags), m_width (w), m_height (h) {
    scene_manager::instance ().attach (this);
  }

  // -----------------------------------------------------------------------------------------------------------
  template <typename Base>
  void scene_window<Base>::show () {
    this->open (m_width, m_height);
  }

  // -----------------------------------------------------------------------------------------------------------
  template <typename Base>
  void scene_window<Base>::show (int x, int y) {
    this->open (m_width, m_height, x, y);
  }

  // -----------------------------------------------------------------------------------------------------------
  template <typename Base>
  void scene_window<Base>::on_event (const events::full_screen&) {
    this->toggle_fullscreen ();
  }

  // -----------------------------------------------------------------------------------------------------------
  template <typename Base>
  void scene_window<Base>::on_input_focus_changed (bool keyboard_focus, bool mouse_focus) {
    scene_manager::instance ().on_input_focus_changed (keyboard_focus, mouse_focus);
  }

  // -----------------------------------------------------------------------------------------------------------
  template <typename Base>
  void scene_window<Base>::on_visibility_change (bool is_visible) {
    scene_manager::instance ().on_visibility_change (is_visible);
  }

  // -----------------------------------------------------------------------------------------------------------
  template <typename Base>
  void scene_window<Base>::on_keyboard_input (const events::keyboard& ev) {
    scene_manager::instance ().on_keyboard_input (ev);
  }

  // -----------------------------------------------------------------------------------------------------------
  template <typename Base>
  void scene_window<Base>::on_pointer_input (const events::pointer& ev) {
    scene_manager::instance ().on_pointer_input (ev);
  }

  // -----------------------------------------------------------------------------------------------------------
  template <typename Base>
  void scene_window<Base>::on_window_invalidate () {
    // TODO
    // m_graphics_context->invalidate(*this);
  }

}

#endif //NEUTRINO_SCENE_WINDOW_HH
