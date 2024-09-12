//
// Created by igor on 9/12/24.
//
#include <vector>
#include <algorithm>
#include <neutrino/events/detail/keyboard_state.hh>
#include <sdlpp/events/event_types.hh>

namespace {
    template <typename T>
    std::vector<bool> keyboard_state_init() {
        auto all_codes = neutrino::sdl::values<T>();
        auto max_sc_val = (std::size_t)*std::max_element(all_codes.begin(), all_codes.end(), [](auto a, auto b) {
            return static_cast<uint16_t>(a) < static_cast<uint16_t>(b);
        });
        return std::vector<bool>(max_sc_val+1, false);
    }
}

namespace neutrino::detail {
    static std::vector<bool> key_state = keyboard_state_init<sdl::scancode>();
    static std::vector<bool> mod_state = keyboard_state_init<sdl::keymod>();

    void keyboard_state_update(const sdl::events::keyboard& kbev) {
        key_state[kbev.scan_code] = kbev.pressed;
        static const auto all_mods = neutrino::sdl::values<sdl::keymod>();
        auto mask = static_cast<uint16_t>(kbev.key_mod);
        for (const auto&m : all_mods) {
            auto k = static_cast<uint16_t>(m);
            if (k & mask) {
                mod_state[k] = kbev.pressed;
            }
        }
    }

    bool keyboard_state_check(const sdl::scancode& sc) {
        return key_state[sc];
    }

    bool keyboard_state_check(const sdl::keymod& mod) {
        static const auto all_mods = neutrino::sdl::values<sdl::keymod>();
        auto mask = static_cast<uint16_t>(mod);
        for (const auto&m : all_mods) {
            auto k = static_cast<uint16_t>(m);
            if (k & mask) {
                return mod_state[k];
            }
        }
        return false;
    }
}


