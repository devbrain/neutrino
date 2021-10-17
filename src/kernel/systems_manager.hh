//
// Created by igor on 07/10/2021.
//

#ifndef SRC_KERNEL_SYSTEMS_MANAGER_HH
#define SRC_KERNEL_SYSTEMS_MANAGER_HH

#include <memory>
#include <list>
#include <neutrino/kernel/system.hh>
#include <neutrino/kernel/systems/base_input_system.hh>
#include <neutrino/kernel/systems/video/video_system.hh>

namespace neutrino::kernel {
  class systems_manager {
    public:
      void add(std::unique_ptr<base_input_system> sys);
      void add(std::unique_ptr<video_system> sys);
      void add(std::unique_ptr<system> sys);

      void on_terminating ();
      void on_low_memory ();
      void on_will_enter_background ();
      void on_in_background ();
      void on_in_foreground ();

      void on_input_focus_changed (bool keyboard_focus, bool mouse_focus);
      void on_visibility_change (bool is_visible);
      void on_keyboard_input (const events::keyboard& ev);
      void on_pointer_input (const events::pointer& ev);

      void on_paused(bool paused);
      void update(std::chrono::milliseconds ms);
      void present();

      base_input_system* get_input_system();
      video_system* get_video_system();
    private:
      std::unique_ptr<base_input_system> m_input_system;
      std::unique_ptr<video_system> m_video_system;
      std::list<std::unique_ptr<system>> m_systems;
  };



  systems_manager* get_systems_manager();
  void release_systems_manager();
}

#endif //SRC_KERNEL_SYSTEMS_MANAGER_HH
