//
// Created by igor on 03/07/2021.
//

#ifndef NEUTRINO_EVENTS_HH
#define NEUTRINO_EVENTS_HH

#include <neutrino/hal/events/events.hh>

namespace neutrino::engine::events {
    using current_fps = hal::events::current_fps;
    using key_mod_t = hal::events::key_mod_t;
    using scan_code_t = hal::events::scan_code_t;
    using pointer_button_t = hal::events::pointer_button_t;
    using keyboard = hal::events::keyboard;
    using pointer = hal::events::pointer;

    struct full_screen{};
    struct quit{};
}

#endif //NEUTRINO_EVENTS_HH
