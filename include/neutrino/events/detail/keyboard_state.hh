//
// Created by igor on 9/12/24.
//

#ifndef NEUTRINO_EVENTS_DETAIL_KEYBOARD_STATE_HH
#define NEUTRINO_EVENTS_DETAIL_KEYBOARD_STATE_HH

#include <sdlpp/events/events.hh>
#include <neutrino/neutrino_export.hh>

namespace neutrino::detail {
	NEUTRINO_EXPORT void keyboard_state_reset();
    NEUTRINO_EXPORT void keyboard_state_update(const sdl::events::keyboard& kbev);
    NEUTRINO_EXPORT bool keyboard_state_check(const sdl::scancode& sc);
    NEUTRINO_EXPORT bool keyboard_state_check(const sdl::keymod& mod);
}

#endif
