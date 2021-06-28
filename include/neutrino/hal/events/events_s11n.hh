//
// Created by igor on 12/06/2021.
//

#ifndef NEUTRINO_HAL_EVENTS_EVENTS_S11N_HH
#define NEUTRINO_HAL_EVENTS_EVENTS_S11N_HH

#include <string>
#include <neutrino/hal/events/events.hh>

namespace neutrino::hal::events {
    template <typename T>
    struct s11n;

    template<>
    struct s11n<key_mod_t> {
        static const char* to_string(const key_mod_t& x);
        static key_mod_t from_string(const std::string& x);
    };

    template<>
    struct s11n<scan_code_t> {
        static const char* to_string(const scan_code_t& x);
        static scan_code_t from_string(const std::string& x);
    };

    template<>
    struct s11n<pointer_button_t> {
        static const char* to_string(const pointer_button_t& x);
        static pointer_button_t from_string(const std::string& x);
    };
}

#endif
