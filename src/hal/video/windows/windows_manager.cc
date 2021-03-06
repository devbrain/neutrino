//
// Created by igor on 09/06/2021.
//

#include "windows_manager.hh"

#include <neutrino/hal/video/window.hh>
#include <hal/sdl/window.hh>
#include "hal/events/events_mapper.hh"

#include <algorithm>

#define d_USER_DATA_ID "neutrino::window"

namespace neutrino::hal::detail {
  neutrino::hal::window* from_event (const neutrino::sdl::events::detail::window_event& ev) {
    try {
      sdl::window sdl_window = sdl::window::by_id (ev.window_id);
      void* p = sdl_window.user_data (d_USER_DATA_ID);
      if (p) {
        return reinterpret_cast<neutrino::hal::window*>(p);
      }
      return nullptr;
    }
    catch ([[maybe_unused]] std::exception& e) {
      return nullptr;
    }
  }

  // ============================================================================================
  void windows_manager::on_event (const neutrino::sdl::events::window_shown& ev) {
    if (auto* w = from_event (ev)) {
      w->_window_shown ();
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::on_event (const neutrino::sdl::events::window_hidden& ev) {
    if (auto* w = from_event (ev)) {
      w->_window_hidden ();
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::on_event (const neutrino::sdl::events::window_exposed& ev) {
    if (auto* w = from_event (ev)) {
      w->_window_exposed ();
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::on_event (const neutrino::sdl::events::window_minimized& ev) {
    if (auto* w = from_event (ev)) {
      w->_window_minimized ();
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::on_event (const neutrino::sdl::events::window_maximized& ev) {
    if (auto* w = from_event (ev)) {
      w->_window_maximized ();
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::on_event (const neutrino::sdl::events::window_restored& ev) {
    if (auto* w = from_event (ev)) {
      w->_window_restored ();
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::on_event (const neutrino::sdl::events::window_mouse_entered& ev) {
    if (auto* w = from_event (ev)) {
      w->_window_mouse_entered ();
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::on_event (const neutrino::sdl::events::window_mouse_leaved& ev) {
    if (auto* w = from_event (ev)) {
      w->_window_mouse_leaved ();
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::on_event (const neutrino::sdl::events::window_focus_gained& ev) {
    if (auto* w = from_event (ev)) {
      w->_window_focus_gained ();
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::on_event (const neutrino::sdl::events::window_focus_lost& ev) {
    if (auto* w = from_event (ev)) {
      w->_window_focus_lost ();
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::on_event (const neutrino::sdl::events::window_close& ev) {
    if (auto* w = from_event (ev)) {
      w->quit ();
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::on_event (const neutrino::sdl::events::window_moved& ev) {
    if (auto* w = from_event (ev)) {
      w->_window_moved (ev.x, ev.y);
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::on_event (const neutrino::sdl::events::window_resized& ev) {
    if (auto* w = from_event (ev)) {
      w->_window_resized (ev.w, ev.h);
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::on_event (const neutrino::sdl::events::keyboard& ev) {
    if (auto* w = from_event (ev)) {
      w->on_keyboard_input (events::map_event (ev));
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::on_event (const sdl::events::mouse_button& ev) {
    if (auto* w = from_event (ev)) {
      w->on_pointer_input (events::map_event (ev));
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::on_event (const sdl::events::mouse_motion& ev) {
    if (auto* w = from_event (ev)) {
      w->on_pointer_input (events::map_event (ev));
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::on_event (const sdl::events::mouse_wheel& ev) {
    if (auto* w = from_event (ev)) {
      w->on_pointer_input (events::map_event (ev));
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::on_event (const sdl::events::touch_device_button& ev) {
    if (auto* w = from_event (ev)) {
      w->on_pointer_input (events::map_event (ev));
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::on_event (const sdl::events::touch_device_motion& ev) {
    if (auto* w = from_event (ev)) {
      w->on_pointer_input (events::map_event (ev));
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::on_event (const sdl::events::touch_device_wheel& ev) {
    if (auto* w = from_event (ev)) {
      w->on_pointer_input (events::map_event (ev));
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::attach (sdl::window& sdl_window, neutrino::hal::window* engine_window) {
    sdl_window.user_data (d_USER_DATA_ID, engine_window);
    m_windows.push_back (engine_window);
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::detach (neutrino::hal::window* engine_window) {
    auto itr = std::find (m_windows.begin (), m_windows.end (), engine_window);
    if (itr != m_windows.end ()) {
      m_windows.erase (itr);
    }
  }

  // -------------------------------------------------------------------------------------------
  bool windows_manager::has_windows () const noexcept {
    return !m_windows.empty ();
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::present () {
    for (auto* w : m_windows) {
      w->present ();
    }
  }

  // -------------------------------------------------------------------------------------------
  void windows_manager::clear () {
    for (auto* w : m_windows) {
      w->clear ();
    }
  }

  // -------------------------------------------------------------------------------------------
  windows_manager::windows_manager () = default;
}