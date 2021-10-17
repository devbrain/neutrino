//
// Created by igor on 17/10/2021.
//

#ifndef APPS_SDL_DEMO_DEMOSCENE_HH
#define APPS_SDL_DEMO_DEMOSCENE_HH

#include <iostream>
#include <neutrino/kernel/application.hh>
#include <neutrino/kernel/systems/input_system.hh>
#include <neutrino/kernel/systems/video/vga256/vga256.hh>
#include <neutrino/utils/observer.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::kernel {

  namespace detail {
    namespace events {
      struct quit{};
      struct full_screen{};
      struct pause{};
    }

    class controller : public utils::observer<events::quit, events::full_screen, events::pause> {
      public:
        void on_event(const events::quit&) override {
          kernel::get_application()->quit();
        }

        void on_event(const events::full_screen&) override {
          kernel::get_application()->toggle_full_screen();
        }

        void on_event(const events::pause&) override {
          static bool paused_state = false;
          paused_state = !paused_state;
          kernel::get_application()->pause(paused_state);
        }
    };

    using input_def = input_system<events::quit, events::full_screen, events::pause>;
    class input : public input_def {
      public:
        explicit input(controller& ctrl)
            : input_def (ctrl){
          when_pressed (kernel::events::scan_code_t::ESCAPE, events::quit{});
          when_pressed (kernel::events::scan_code_t::RETURN, kernel::events::key_mod_t::ALT, events::full_screen{});
          when_pressed (kernel::events::scan_code_t::SPACE, events::pause{});
        }
    };
  } // ns detail


    struct demo_effect {
      template <typename Effect>
      static void run (int w, int h, int desired_fps = 60) {
        try {
          detail::controller the_controller;
          kernel::vga256 screen (w, h);
          kernel::application app (std::make_unique<neutrino::kernel::detail::input> (the_controller),
                                   screen.create_system (),
                                   std::make_unique<Effect> (screen));
          app.show (screen.width (), screen.height ());
          app.run (desired_fps);
        }
        catch (std::exception& e) {
          std::cerr << "ERROR: " << e.what () << std::endl;
        }
      }
    };
}

#endif //APPS_SDL_DEMO_DEMOSCENE_HH
