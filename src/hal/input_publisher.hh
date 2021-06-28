//
// Created by igor on 08/06/2021.
//

#ifndef NEUTRINO_HAL_INPUT_PUBLISHER_HH
#define NEUTRINO_HAL_INPUT_PUBLISHER_HH

#include <neutrino/sdl/events/events_dispatcher.hh>
#include <neutrino/utils/observer.hh>

namespace neutrino::hal {
    class input_publisher : private utils::publisher<sdl::events::all_events_t>
    {
    public:
        using publisher<sdl::events::all_events_t>::attach;
        using publisher<sdl::events::all_events_t>::detach;

        void run() {
            SDL_Event ev;
            while( SDL_PollEvent( &ev ) != 0 ) {
                this->notify(sdl::map_event(ev));
            }
       }
    };
}

#endif
