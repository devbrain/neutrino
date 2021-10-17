//
// Created by igor on 09/10/2021.
//

#ifndef SRC_KERNEL_MAIN_WINDOW_HH
#define SRC_KERNEL_MAIN_WINDOW_HH

#include <neutrino/hal/video/window.hh>
#include <neutrino/kernel/events.hh>
#include <neutrino/kernel/systems/video/video_system.hh>

namespace neutrino::kernel {
  class main_window : public hal::window_2d {
    public:
      main_window() = default;
      explicit main_window(hal::window_flags_t flags);

      void set_up(video_system* sys);
    private:
      void on_input_focus_changed (bool keyboard_focus, bool mouse_focus) override;
      void on_visibility_change (bool is_visible) override;
      void on_keyboard_input (const events::keyboard& ev) override;
      void on_pointer_input (const events::pointer& ev) override;
      void on_window_invalidate () override;
  };
}
#endif //SRC_KERNEL_MAIN_WINDOW_HH
