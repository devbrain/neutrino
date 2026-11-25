//
// Created by igor on 25/11/2026.
//

#include <cstdint>
#include <limits>

#include "events.hh"
#include <failsafe/failsafe.hh>

namespace neutrino {
    namespace detail {
        std::uint32_t setup_event_category(std::uint32_t& category, std::string_view cat_name) {
            // SDL never gives a registered type back, so the type is kept for the whole
            // process: every application after the first finds it registered.
            if (category == INVALID_EVENT) {
                auto ids = sdlpp::event_registry::register_events(1);
                ENFORCE(ids, "failed to register events for ", cat_name);
                category = *ids;
            }
            return category;
        }

        sdlpp::event create_event(std::uint32_t category, std::string_view cat_name, int32_t code, void* data1, void* data2) {
            ENFORCE(category != INVALID_EVENT, "events for ", cat_name, "were not registered");
            SDL_Event custom{};
            custom.type = category;
            custom.user.code = code;
            custom.user.data1 = data1;
            custom.user.data2 = data2;
            return sdlpp::event{custom};
        }

        void drain_events(std::uint32_t& category, [[maybe_unused]] std::string_view cat_name) {
            // Never registered — startup failed before on_ready(), which SDL still follows
            // with on_quit() — so none can be queued.
            if (category == INVALID_EVENT) {
                return;
            }
            sdlpp::event_queue::flush(static_cast <sdlpp::event_type>(category));
        }
    }

    void enqueue_event(sdlpp::event&& e) {
        ENFORCE(sdlpp::event_queue::push(e));
    }
}
