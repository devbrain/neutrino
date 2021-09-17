//
// Created by igor on 26/06/2021.
//

#ifndef NEUTRINO_HAL_EVENTS_BROKER_HH
#define NEUTRINO_HAL_EVENTS_BROKER_HH

#include <vector>
#include <string>
#include <set>
#include <cstdint>

#include <neutrino/utils/mp/type_name/type_name.hpp>
#include <neutrino/utils/mp/constexpr_for.hh>
#include <neutrino/utils/mp/typelist.hh>
#include <neutrino/utils/sorted_array.hh>
#include <neutrino/utils/observer.hh>

namespace neutrino::hal {
  namespace detail {
    class abstract_dynamic_event_handler {
      public:
        virtual ~abstract_dynamic_event_handler ();

        virtual void handle (void* raw_data) const = 0;
        bool remove (void* pointer_to_observer);
        [[nodiscard]] bool empty () const;
        bool add (void* pointer_to_observer);
      protected:
        std::vector<void*> m_handlers;
    };

    template <typename Event>
    class dynamic_event_handler : public abstract_dynamic_event_handler {
      private:
        void handle (void* raw_data) const override {
          const auto& ev = *reinterpret_cast<Event*>(raw_data);
          for (void* ptr : m_handlers) {
            auto* obs = reinterpret_cast<neutrino::utils::detail::observer<Event>*>(ptr);
            obs->on_event (ev);
          }
        }
    };

    struct event_handler_holder {

      event_handler_holder () = default;

      explicit event_handler_holder (uint32_t k)
          : key (k), value (nullptr) {
      }

      template <typename Event>
      static auto make () {
        event_handler_holder e;
        e.key = type_hash_v<Event>;
        e.m_type_name = type_name_v<Event>;
        e.value = new detail::dynamic_event_handler<Event> ();
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

  class events_broker {
    protected:
      using destructor_t = void (*) (void* raw);
      using container_t = std::vector<detail::event_handler_holder>;
    public:
      events_broker ();
      virtual ~events_broker ();

      template <typename Event, typename ... Args>
      void notify (Args&& ... args) {
        constexpr uint32_t key_type = type_hash_v<Event>;
        detail::event_handler_holder key (key_type);

        if (!utils::sorted_array<container_t>::exists (m_table, key)) {
          return;
        }

        static destructor_t destructor = [] (void* raw) {
          auto* e = reinterpret_cast<Event*>(raw);
          delete e;
        };

        _pos_event (static_cast<int32_t>(key_type), new Event (std::forward<Args> (args)...), reinterpret_cast<void*>(destructor));
      }

      template <typename... Events>
      void attach (utils::observer<Events...>* observer) {
        using events_t = mp::type_list<Events...>;
        bool added = false;
        mp::constexpr_for<0, events_t::size (), 1> ([this, observer, &added] (auto idx) {
          using event_t = mp::type_list_at_t<idx.value, events_t>;
          using observer_t = neutrino::utils::detail::observer<event_t>;
          auto* specific_observer = dynamic_cast<observer_t*>(observer);
          static detail::event_handler_holder key{type_hash_v<event_t>};

          auto itr = utils::sorted_array<container_t>::find (m_table, key);

          if (itr == m_table.end ()) {
            auto holder = detail::event_handler_holder::make<event_t> ();
            holder.value->add (specific_observer);
            utils::sorted_array<container_t>::insert (m_table, holder);
            added = true;
          }
          else {
            if (itr->value->add (specific_observer)) {
              added = true;
            }
          }
        });

        if (added) {
          m_monitors.insert (observer);
          observer->on_subscribed ();
        }
      }

      template <typename... Events>
      void detach (utils::observer<Events...>* observer) {
        using events_t = mp::type_list<Events...>;
        bool removed = false;
        mp::constexpr_for<0, events_t::size (), 1> ([this, observer, &removed] (auto idx) {
          using event_t = mp::type_list_at_t<idx.value, events_t>;
          using observer_t = neutrino::utils::detail::observer<event_t>;
          auto* specific_observer = dynamic_cast<observer_t*>(observer);
          static detail::event_handler_holder key{type_hash_v<event_t>};
          auto itr = utils::sorted_array<container_t>::find (m_table, key);
          if (itr != m_table.end ()) {
            if (itr->value->remove (specific_observer)) {
              removed = true;
            }
          }
        });

        if (removed) {
          m_monitors.erase (observer);
          observer->on_unsubscribed ();
        }
      }

    protected:
      container_t m_table;
      std::set<utils::detail::observer_monitor*> m_monitors;
    private:
      static void _pos_event (uint32_t code, void* data1, void* data2);
  };
}
#endif
