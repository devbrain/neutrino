//
// Created by igor on 07/10/2021.
//

#include <neutrino/kernel/systems/base_input_system.hh>

namespace neutrino::kernel {
  base_input_system::~base_input_system () = default;

  void base_input_system::on_terminating () {

  }

  void base_input_system::on_low_memory () {

  }

  void base_input_system::on_will_enter_background () {

  }

  void base_input_system::on_in_background () {

  }

  void base_input_system::on_in_foreground () {

  }

  void base_input_system::on_input_focus_changed ([[maybe_unused]] bool keyboard_focus,[[maybe_unused]]  bool mouse_focus) {

  }

  void base_input_system::on_visibility_change ([[maybe_unused]] bool is_visible) {

  }

  void base_input_system::on_keyboard_input ([[maybe_unused]] const events::keyboard& ev) {

  }

  void base_input_system::on_pointer_input ([[maybe_unused]] const events::pointer& ev) {

  }
}