//
// Created by igor on 18/10/2021.
//

#include "main_window.hh"
#include <neutrino/kernel/application.hh>

namespace neutrino {

  static hal::window_flags_t get_flags(const main_window_description& d) {
    hal::window_flags_t f = hal::window_flags_t ::MAXIMIZED;
    if (d.fullscreen()) {
      f |= hal::window_flags_t::FULLSCREEN;
    }
    if (d.resizable()) {
      f |= hal::window_flags_t::RESIZABLE;
    }
    return f;
  }

  main_window::main_window (const main_window_description& descr, application* owner)
      : hal::window(hal::window::window_kind_t::SIMPLE, get_flags (descr)),
        m_descr (descr),
        m_owner (owner) {
  }

  void main_window::show () {
    if (m_descr.position()) {
      auto pos = *m_descr.position();
      open(static_cast<int>(m_descr.width()), static_cast<int>(m_descr.height()), pos[0], pos[1], m_descr.title());
    } else {
      open(static_cast<int>(m_descr.width()), static_cast<int>(m_descr.height()), m_descr.title());
    }
  }

  void main_window::on_window_resized (int w, int h) {
    window::on_window_resized (w, h);
    m_owner->on_window_resized (static_cast<unsigned int>(w), static_cast<unsigned int>(h));
  }

  void main_window::on_input_focus_changed (bool keyboard_focus, bool mouse_focus) {
    window::on_input_focus_changed (keyboard_focus, mouse_focus);
    if (!keyboard_focus && !mouse_focus) {
      m_owner->set_paused (true);
    } else {
      m_owner->set_paused (false);
    }
  }

  void main_window::on_visibility_change (bool is_visible) {
    window::on_visibility_change (is_visible);
    m_owner->set_paused (!is_visible);
  }

  void main_window::on_keyboard_input (const hal::events::keyboard& ev) {
    window::on_keyboard_input (ev);
    m_owner->on_keyboard_input (ev);
  }

  void main_window::on_pointer_input (const hal::events::pointer& ev) {
    window::on_pointer_input (ev);
    m_owner->on_pointer_input (ev);
  }

  void main_window::after_window_opened () {
    m_renderer = std::make_unique<hal::renderer>(*this);
  }

  void main_window::clear () {
    if (!m_owner->is_paused()) {
      auto ac = m_renderer->active_color ();
      m_renderer->active_color ({0x0, 0x0, 0x0, 0xFF});
      m_renderer->clear ();
      m_renderer->active_color (ac);
    }
  }

  void main_window::present () {
    m_owner->do_draw_frame();
    m_renderer->present();
  }

  void main_window::close () {
    this->quit();
  }

  hal::renderer* main_window::renderer () {
    return m_renderer.get();
  }
}