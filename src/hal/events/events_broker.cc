//
// Created by igor on 26/06/2021.
//

#include <neutrino/hal/events/events_broker.hh>
#include <neutrino/sdl/events/system_events.hh>
#include <algorithm>

namespace neutrino::hal
{
    namespace detail
    {
        abstract_dynamic_event_handler::~abstract_dynamic_event_handler() = default;

        bool abstract_dynamic_event_handler::remove(void* pointer_to_observer)
        {
            auto itr = std::find(m_handlers.begin(), m_handlers.end(), pointer_to_observer);
            if (itr != m_handlers.end())
            {
                m_handlers.erase(itr);
                return true;
            }
            return false;
        }

        bool abstract_dynamic_event_handler::empty() const
        {
            return m_handlers.empty();
        }

        bool abstract_dynamic_event_handler::add(void* pointer_to_observer)
        {
            auto itr = std::find(m_handlers.begin(), m_handlers.end(), pointer_to_observer);
            if (itr != m_handlers.end())
            {
                m_handlers.push_back(pointer_to_observer);
                return true;
            }
            return false;
        }
    } // ns detail
    // ===========================================================================================
    events_broker::events_broker() = default;

    events_broker::~events_broker() {
        for (auto* m : m_monitors) {
            m->on_unsubscribed();
        }
    }
    // -------------------------------------------------------------------------------------------
    void events_broker::_pos_event(uint32_t code, void *data1, void* data2) {
        SDL_Event ev;
        ev.type = SDL_USEREVENT;
        ev.user.code = static_cast<int32_t>(code);
        ev.user.data1 = data1;
        ev.user.data2 = data2;

        SDL_PushEvent(&ev);
    }
}