//
// Created by igor on 26/06/2021.
//

#include <neutrino/hal/system.hh>
#include <neutrino/sdl/system.hh>

namespace neutrino::hal {

    system::system() {
        SAFE_SDL_CALL(SDL_Init, SDL_INIT_EVERYTHING);
    }

    system::system(system_flags_t flags) {
        uint32_t f = SDL_INIT_EVENTS | SDL_INIT_TIMER | SDL_INIT_VIDEO;
        if (flags & system_flags_t::AUDIO) {
            f |= SDL_INIT_AUDIO;
        }
        if (flags & system_flags_t::JOYSTICK) {
            f |= SDL_INIT_JOYSTICK;
        }
        if (flags & system_flags_t::HAPTIC) {
            f |= SDL_INIT_HAPTIC;
        }
        if (flags & system_flags_t::GAMECONTROLLER) {
            f |= SDL_INIT_GAMECONTROLLER;
        }
        if (flags & system_flags_t::SENSOR) {
            f |= SDL_INIT_SENSOR;
        }
        SAFE_SDL_CALL(SDL_Init, f);
    }

    system::~system() {
        SDL_Quit();
    }
}