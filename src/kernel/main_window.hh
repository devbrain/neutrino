//
// Created by igor on 09/10/2021.
//

#ifndef SRC_KERNEL_MAIN_WINDOW_HH
#define SRC_KERNEL_MAIN_WINDOW_HH

#include <memory>
#include <neutrino/hal/video/window.hh>
#include <neutrino/hal/video/renderer.hh>
#include <neutrino/kernel/application_description.hh>

namespace neutrino {
  class application;
  class main_window : public hal::window {
    public:
      main_window(const main_window_description& descr, application* owner);
      void show();
      void close();
      hal::renderer* renderer();
    protected:
      void on_window_resized (int w, int h) override;
      void on_input_focus_changed (bool keyboard_focus, bool mouse_focus) override;
      void on_visibility_change (bool is_visible) override;

      void on_keyboard_input (const hal::events::keyboard& ev) override;
      void on_pointer_input (const hal::events::pointer& ev) override;
    private:
      void after_window_opened () override;
      void on_full_screen(bool is_fullscreen) override;
      void clear () override;
      void present () override;
    private:
      void update_renderer();
    private:
      const main_window_description  m_descr;
      application* m_owner;
      std::unique_ptr<hal::renderer> m_renderer;
  };
}
#endif //SRC_KERNEL_MAIN_WINDOW_HH
