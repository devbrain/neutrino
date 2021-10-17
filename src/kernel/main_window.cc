//
// Created by igor on 09/10/2021.
//

#include "main_window.hh"
#include "systems_manager.hh"

namespace neutrino::kernel {
  void main_window::on_input_focus_changed (bool keyboard_focus, bool mouse_focus) {
    get_systems_manager()->on_input_focus_changed (keyboard_focus, mouse_focus);
  }

  void main_window::on_visibility_change (bool is_visible) {
    get_systems_manager()->on_visibility_change (is_visible);
  }

  void main_window::on_keyboard_input (const events::keyboard& ev) {
    get_systems_manager()->on_keyboard_input (ev);
  }

  void main_window::on_pointer_input (const events::pointer& ev) {
    get_systems_manager()->on_pointer_input (ev);
  }

  void main_window::on_window_invalidate () {
    // TODO
  }

  main_window::main_window (hal::window_flags_t flags)
      : window_2d (flags) {
  }

  void main_window::set_up(video_system* sys) {
    sys->init (*this);
  }
}