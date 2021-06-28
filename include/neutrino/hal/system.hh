//
// Created by igor on 26/06/2021.
//

#ifndef NEUTRINO_HAL_SYSTEM_HH
#define NEUTRINO_HAL_SYSTEM_HH

#include <bitflags/bitflags.hpp>

namespace neutrino::hal {
    BEGIN_BITFLAGS(system_flags_t)
        FLAG(AUDIO)
        FLAG(JOYSTICK)
        FLAG(HAPTIC)
        FLAG(GAMECONTROLLER)
        FLAG(SENSOR)
    END_BITFLAGS(system_flags_t)

    class system final {
    public:
        system();
        explicit system(system_flags_t flags);

        ~system();
    };
}

#endif
