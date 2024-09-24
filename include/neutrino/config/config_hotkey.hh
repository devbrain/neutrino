//
// Created by igor on 9/11/24.
//

#ifndef CONFIG_KEYBOARD_HH
#define CONFIG_KEYBOARD_HH

#include <bsw/s11n/s11n.hh>
#include <sdlpp/events/event_types.hh>
#include <sdlpp/events/system_events.hh>
#include <neutrino/neutrino_export.hh>

namespace neutrino {

    struct NEUTRINO_EXPORT config_hotkey {
        config_hotkey() = default;
        explicit config_hotkey(sdl::scancode scancode_)
            : scancode(scancode_) {}
        config_hotkey(sdl::scancode scancode_, sdl::keymod keymod_)
            :scancode(scancode_), keymod(keymod_) {}
		explicit config_hotkey(sdl::keymod keymod_)
			: keymod(keymod_) {}

        std::optional<sdl::scancode> scancode;
        std::optional<sdl::keymod> keymod;

        SERIALIZATION_SCHEMA(scancode, keymod)
    };

}

#endif
