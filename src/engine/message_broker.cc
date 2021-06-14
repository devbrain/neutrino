//
// Created by igor on 09/06/2021.
//

#include <neutrino/engine/message_broker.hh>
#include <algorithm>

namespace neutrino::engine
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
    message_broker::message_broker() = default;

    message_broker::~message_broker() {
        for (auto* m : m_monitors) {
            m->on_unsubscribed();
        }
    }

    void message_broker::on_event(const sdl::events::user& ev) {
        detail::event_handler_holder key {static_cast<uint32_t>(ev.code)};
        auto itr = utils::sorted_array<container_t>::find(m_table, key);
        if (itr != m_table.end())
        {
            itr->value->handle(ev.data1);
        }
        destructor_t destructor = reinterpret_cast<destructor_t>(ev.data2);
        destructor(ev.data1);
    }
}