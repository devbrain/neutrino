//
// Created by igor on 09/06/2021.
//

#ifndef NEUTRINO_MESSAGE_BROKER_HH
#define NEUTRINO_MESSAGE_BROKER_HH

#include <neutrino/utils/mp/type_name/type_name.hpp>
#include <neutrino/utils/mp/constexpr_for.hh>
#include <neutrino/utils/mp/typelist.hh>
#include <neutrino/utils/sorted_array.hh>
#include <neutrino/sdl/events/system_events.hh>
#include <neutrino/engine/observer.hh>
#include <vector>
#include <string>
#include <set>

namespace neutrino::engine {
    namespace detail {
        class abstract_dynamic_event_handler {
        public:
            virtual ~abstract_dynamic_event_handler();

            virtual void handle (void* raw_data) const = 0;
            bool remove (void* pointer_to_observer);
            bool empty() const;
            bool add(void* pointer_to_observer);
        protected:
            std::vector<void*> m_handlers;
        };

        template <typename Event>
        class dynamic_event_handler : public abstract_dynamic_event_handler {
        private:
            void handle(void* raw_data) const {
                const auto& ev = *reinterpret_cast<Event*>(raw_data);
                for (void* ptr : m_handlers) {
                    auto* obs = reinterpret_cast<neutrino::engine::detail::observer<Event>*>(ptr);
                    obs->on_event(ev);
                }
            }
        };

        struct event_handler_holder {

            event_handler_holder() = default;

            event_handler_holder(uint32_t k)
            : key(k), value(nullptr) {}

            template<typename Event>
            static auto make() {
                event_handler_holder e;
                e.key = type_hash_v<Event>;
                e.m_type_name = type_name_v<Event>;
                e.value = new detail::dynamic_event_handler<Event>();
                return e;
            }

            uint32_t key;
            std::string_view m_type_name;
            abstract_dynamic_event_handler* value;
        };

        inline
        bool operator < (const event_handler_holder& a, const event_handler_holder& b) {
            return a.key < b.key;
        }

        inline
        bool operator == (const event_handler_holder& a, const event_handler_holder& b) {
            return a.key == b.key;
        }
    }

    class message_broker : public observer<sdl::events::user> {
        using destructor_t = void (*)(void* raw);
        using container_t = std::vector<detail::event_handler_holder>;
    public:
        message_broker();
        ~message_broker();

        template<typename Event, typename ... Args>
        void notify(Args&&... args) {
            constexpr uint32_t key_type = type_hash_v<Event>;
            detail::event_handler_holder key(key_type);

            if (!utils::sorted_array<container_t>::exists(m_table, key)) {
                return;
            }

            static destructor_t destructor = [](void* raw) {
                auto* e = reinterpret_cast<Event*>(raw);
                delete e;
            };

            SDL_Event ev;
            ev.type = SDL_USEREVENT;
            ev.user.code = static_cast<int32_t>(key_type);
            ev.user.data1 = new Event(std::forward<Args>(args)...);
            ev.user.data2 = reinterpret_cast<void*>(destructor);

            SDL_PushEvent(&ev);
        }

        template<typename... Events>
        void attach(observer<Events...>* observer) {
            using events_t = mp::type_list<Events...>;
            bool added = false;
            mp::constexpr_for<0, events_t::size(), 1>([this, observer, &added](auto idx) {
                using event_t = mp::type_list_at_t<idx.value, events_t>;
                using observer_t = neutrino::engine::detail::observer<event_t>;
                auto* specific_observer = dynamic_cast<observer_t *>(observer);
                static detail::event_handler_holder key {type_hash_v<event_t>};

                auto itr = utils::sorted_array<container_t>::find(m_table, key);

                if (itr == m_table.end())
                {
                    auto holder = detail::event_handler_holder::make<event_t>();
                    holder.value->add(specific_observer);
                    utils::sorted_array<container_t>::insert(m_table, holder);
                    added = true;
                } else {
                    if (itr->value->add(specific_observer)) {
                        added = true;
                    }
                }
            });

            if (added) {
                m_monitors.insert(observer);
                observer->on_subscribed();
            }
        }

        template<typename... Events>
        void detach(observer<Events...>* observer) {
            using events_t = mp::type_list<Events...>;
            bool removed = false;
            mp::constexpr_for<0, events_t::size(), 1>([this, observer, &removed](auto idx) {
                using event_t = mp::type_list_at_t<idx.value, events_t>;
                using observer_t = neutrino::engine::detail::observer<event_t>;
                auto* specific_observer = dynamic_cast<observer_t *>(observer);
                static detail::event_handler_holder key {type_hash_v<event_t>};
                auto itr = utils::sorted_array<container_t>::find(m_table, key);
                if (itr != m_table.end()) {
                    if (itr->value->remove(specific_observer)) {
                        removed = true;
                    }
                }
            });

            if (removed) {
                m_monitors.erase(observer);
                observer->on_unsubscribed();
            }
        }
    private:
        void on_event(const sdl::events::user& ev);
    private:
        container_t m_table;
        std::set<detail::observer_monitor*> m_monitors;
    };
}


#endif
