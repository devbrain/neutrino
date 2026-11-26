//
// Created by igor on 25/11/2026.
//

#pragma once

#include <cstdint>
#include <type_traits>

#include "events/detail/events.hh"
#include <failsafe/type_name.hh>

namespace neutrino {
    template<typename T>
    concept command_like = std::is_enum_v <T> &&
                           std::is_same_v <std::underlying_type_t <T>, std::int32_t>;

    /**
     * @brief The engine's own SDL user events, one registered SDL type per code enum.
     *
     * @p Code is an enum over int32_t naming one subsystem's events; a type of its own keeps
     * its codes apart from every other subsystem's, and messages name the enum. Payloads are
     * the two pointers of an SDL user event.
     *
     *   - init() registers the type. Every engine event type is registered in on_ready(),
     *     before game code runs, so a failure surfaces at startup. A type is kept for the whole
     *     process — SDL never gives one back — so an init() in every application after the
     *     first finds it registered and does nothing.
     *   - push() posts an event; create_event() builds one without posting. Both ENFORCE that
     *     the type is registered.
     *   - drain() removes the queued events of this type; it does nothing for a type never
     *     registered (startup failed before on_ready(), which SDL still follows with on_quit()).
     *   - category() is the SDL type, to recognise the events in handle_event(); the invalid
     *     value until init().
     *
     * Engine-internal: games do not post engine events (P3.11, "Engine events").
     */
    template<command_like Code>
    class custom_event {
        public:
            static void init();
            static sdlpp::event create_event(Code code, void* data1, void* data2 = nullptr);
            static void push(Code code, void* data1, void* data2 = nullptr);
            static void drain();
            static std::uint32_t category();
        private:
            static std::string_view s_name;
            static std::uint32_t s_sdl_event_type;
    };


    template<command_like Code>
    void custom_event<Code>::init() {
        s_sdl_event_type = detail::setup_event_category(s_sdl_event_type, s_name);
    }

    template<command_like Code>
    sdlpp::event custom_event<Code>::create_event(Code code, void* data1, void* data2) {
        return detail::create_event(s_sdl_event_type, s_name, static_cast<std::int32_t>(code), data1, data2);
    }

    template<command_like Code>
    void custom_event<Code>::push(Code code, void* data1, void* data2) {
        enqueue_event(create_event(code, data1, data2));
    }

    template<command_like Code>
    void custom_event<Code>::drain() {
        detail::drain_events(s_sdl_event_type, s_name);
    }

    template<command_like Code>
    std::uint32_t custom_event<Code>::category() {
        return s_sdl_event_type;
    }

    template<command_like Code>
    std::uint32_t custom_event<Code>::s_sdl_event_type = detail::INVALID_EVENT;

    template<command_like Code>
    std::string_view custom_event<Code>::s_name = failsafe::type_name_v<Code>;

}
