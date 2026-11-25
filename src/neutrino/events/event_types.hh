//
// Created by igor on 25/11/2026.
//

#pragma once

#include "events/command_concept.hh"
#include "events/events.hh"
#include <sdlpp/events/events.hh>
#include <failsafe/type_name.hh>

namespace neutrino {

    template<command_like Code>
    class custom_event {
        public:
            static void init();
            static sdlpp::event create_event(Code code, void* data1, void* data2 = nullptr);
            static void push(Code code, void* data1, void* data2 = nullptr);
            static void drain();
            static std::uint32_t category();
        private:
            static std::uint32_t s_sdl_event_type;
    };

    template<command_like Code>
    void custom_event<Code>::init() {
        s_sdl_event_type = detail::setup_event_category(s_sdl_event_type, failsafe::type_name_v<Code>);
    }

    template<command_like Code>
    sdlpp::event custom_event<Code>::create_event(Code code, void* data1, void* data2) {
        return detail::create_event(s_sdl_event_type, failsafe::type_name_v<Code>, static_cast<std::int32_t>(code), data1, data2);
    }

    template<command_like Code>
    void custom_event<Code>::push(Code code, void* data1, void* data2) {
        enqueue_event(create_event(code, data1, data2));
    }

    template<command_like Code>
    void custom_event<Code>::drain() {
        detail::drain_events(s_sdl_event_type, failsafe::type_name_v<Code>);
    }

    template<command_like Code>
    std::uint32_t custom_event<Code>::category() {
        return s_sdl_event_type;
    }

    template<command_like Code>
    std::uint32_t custom_event<Code>::s_sdl_event_type = detail::INVALID_EVENT;


}
