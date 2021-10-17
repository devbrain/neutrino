//
// Created by igor on 07/10/2021.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_SYSTEMS_BASE_INPUT_SYSTEM_HH
#define INCLUDE_NEUTRINO_KERNEL_SYSTEMS_BASE_INPUT_SYSTEM_HH

#include <neutrino/kernel/events.hh>

namespace neutrino::kernel {

  class base_input_system {
    public:
      virtual ~base_input_system();

      virtual void on_terminating ();
      virtual void on_low_memory ();
      virtual void on_will_enter_background ();
      virtual void on_in_background ();
      virtual void on_in_foreground ();

      virtual void on_input_focus_changed (bool keyboard_focus, bool mouse_focus);
      virtual void on_visibility_change (bool is_visible);
      virtual void on_keyboard_input (const events::keyboard& ev);
      virtual void on_pointer_input (const events::pointer& ev);
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_SYSTEMS_BASE_INPUT_SYSTEM_HH
