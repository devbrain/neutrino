//
// Created by igor on 25/11/2026.
//

#pragma once

#include <string_view>

#include <sdlpp/events/events.hh>


namespace neutrino {
    namespace detail {

        inline constexpr auto INVALID_EVENT = std::numeric_limits <std::uint32_t>::max();
        std::uint32_t setup_event_category(std::uint32_t& category, std::string_view cat_name);

        sdlpp::event create_event(std::uint32_t category, std::string_view cat_name, int32_t code, void* data1, void* data2);
        void drain_events(std::uint32_t& category, std::string_view cat_name);
    }

    void enqueue_event(sdlpp::event&& e);
}