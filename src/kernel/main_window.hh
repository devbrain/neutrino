//
// Created by igor on 09/10/2021.
//

#ifndef SRC_KERNEL_MAIN_WINDOW_HH
#define SRC_KERNEL_MAIN_WINDOW_HH

#include <memory>
#include <optional>

#include <neutrino/hal/video/window.hh>
#include <neutrino/kernel/events.hh>
#include <neutrino/kernel/systems/video/video_system.hh>
#include <neutrino/kernel/systems/video/accel_renderer_video_system.hh>
#include <neutrino/kernel/systems/video/opengl_video_system.hh>
#include "systems_manager.hh"

namespace neutrino::kernel {
  template <typename Base>
  class main_window : public Base {
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

  std::unique_ptr<hal::window> window_factory(std::optional<hal::window_flags_t> flags, video_system* sys);
  void set_up_system(hal::window* win, video_system* sys);

  // =========================================================================================
  template <typename Base>
  void main_window<Base>::on_input_focus_changed (bool keyboard_focus, bool mouse_focus) {
    get_systems_manager()->on_input_focus_changed (keyboard_focus, mouse_focus);
  }

  template <typename Base>
  void main_window<Base>::on_visibility_change (bool is_visible) {
    get_systems_manager()->on_visibility_change (is_visible);
  }

  template <typename Base>
  void main_window<Base>::on_keyboard_input (const events::keyboard& ev) {
    get_systems_manager()->on_keyboard_input (ev);
  }

  template <typename Base>
  void main_window<Base>::on_pointer_input (const events::pointer& ev) {
    get_systems_manager()->on_pointer_input (ev);
  }

  template <typename Base>
  void main_window<Base>::on_window_invalidate () {
    // TODO
  }

  template <typename Base>
  main_window<Base>::main_window (hal::window_flags_t flags)
      : Base (flags) {
  }

  template <typename Base>
  void main_window<Base>::set_up(video_system* sys) {
    sys->init (*this);
  }
}
#endif //SRC_KERNEL_MAIN_WINDOW_HH
