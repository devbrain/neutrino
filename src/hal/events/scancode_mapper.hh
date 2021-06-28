//
// Created by igor on 26/06/2021.
//

#ifndef NEUTRINO_HAL_EVENTS_SCANCODE_MAPPER_HH
#define NEUTRINO_HAL_EVENTS_SCANCODE_MAPPER_HH


#include <neutrino/sdl/events/event_types.hh>
#include <neutrino/hal/events/scancodes.hh>

namespace neutrino::hal::events {
    scan_code_t map_scancode(neutrino::sdl::scancode sc);
}


#endif
