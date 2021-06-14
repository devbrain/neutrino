//
// Created by igor on 08/06/2021.
//

#ifndef NEUTRINO_ENGINE_INPUT_PUBLISHER_HH
#define NEUTRINO_ENGINE_INPUT_PUBLISHER_HH

#include <neutrino/sdl/events/events_dispatcher.hh>
#include <neutrino/engine/observer.hh>

namespace neutrino::engine {
    class input_publisher : private publisher<sdl::events::all_events_t>
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
