//
// Created by igor on 20/10/2021.
//

#ifndef INCLUDE_NEUTRINO_HAL_SDL_EVENT_HOOK_HH
#define INCLUDE_NEUTRINO_HAL_SDL_EVENT_HOOK_HH

union SDL_Event;

namespace neutrino::hal {
  class sdl_event_hook {
    public:
      virtual ~sdl_event_hook();
      virtual void handle(SDL_Event* ev) = 0;
  };
}

#endif //INCLUDE_NEUTRINO_HAL_SDL_EVENT_HOOK_HH
