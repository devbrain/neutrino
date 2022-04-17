//
// Created by igor on 12/04/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_CONTROLLER_HH
#define INCLUDE_NEUTRINO_KERNEL_CONTROLLER_HH

#include <neutrino/kernel/events.hh>
#include <neutrino/kernel/application.hh>
#include <neutrino/utils/observer.hh>

namespace neutrino::kernel {
  template <class ... Events>
  class controller : public utils::observer<events::quit, events::full_screen, events::pause, Events...> {
    public:
      void on_event (const events::quit&) override {
        kernel::get_application ()->quit ();
      }

      void on_event (const events::full_screen&) override {
        kernel::get_application ()->toggle_full_screen ();
      }

      void on_event (const events::pause&) override {
        static bool paused_state = false;
        paused_state = !paused_state;
        kernel::get_application ()->pause (paused_state);
      }
  };

}

#endif //INCLUDE_NEUTRINO_KERNEL_CONTROLLER_HH
