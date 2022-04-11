//
// Created by igor on 08/06/2021.
//

#ifndef NEUTRINO_HAL_INPUT_PUBLISHER_HH
#define NEUTRINO_HAL_INPUT_PUBLISHER_HH


#include <list>
#include <memory>

#include <hal/sdl/events/events_dispatcher.hh>
#include <neutrino/utils/observer.hh>
#include <neutrino/hal/sdl_event_hook.hh>

namespace neutrino::hal {
  class input_publisher : private utils::publisher<sdl::events::all_events_t> {
    public:
      using publisher<sdl::events::all_events_t>::attach;
      using publisher<sdl::events::all_events_t>::detach;

      void run () {
        SDL_Event ev;
        while (SDL_PollEvent (&ev) != 0) {
          for (auto& hook : m_hooks) {
            hook->handle (&ev);
          }
          this->notify (sdl::map_event (ev));
        }
      }

      void add_hook(std::unique_ptr<sdl_event_hook> hook) {
        m_hooks.push_back (std::move(hook));
      }
    private:
      std::list<std::unique_ptr<sdl_event_hook>> m_hooks;
  };
}

#endif
