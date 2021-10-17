//
// Created by igor on 14/10/2021.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_SYSTEMS_INPUT_SYSTEM_HH
#define INCLUDE_NEUTRINO_KERNEL_SYSTEMS_INPUT_SYSTEM_HH

#include <functional>
#include <optional>
#include <variant>
#include <map>

#include <neutrino/utils/observer.hh>
#include <neutrino/utils/exception.hh>
#include <neutrino/utils/override.hh>
#include <neutrino/hal/events/events_s11n.hh>
#include <neutrino/kernel/systems/base_input_system.hh>


namespace neutrino::kernel {
  template <typename ... Events>
  class input_system : public base_input_system, public utils::publisher<Events...> {
    public:
      template <typename Observer>
      explicit input_system (Observer& controller) {
        this->attach (&controller);
      }

    public:
      template <typename Ev>
      void when_pressed (events::scan_code_t scan_code, const Ev& event) {
        add_handler (m_pressed_handler, scan_code, [this, event] {
          this->notify (event);
        });
      }

      template <typename Ev>
      void when_pressed (events::scan_code_t scan_code, std::function<Ev (const events::keyboard& ev)> event_factory) {
        add_handler (m_pressed_handler, scan_code, [this, event_factory](const events::keyboard& ev) {
          this->notify (event_factory(ev));
        });
      }

      template <typename Ev>
      void when_pressed (events::scan_code_t scan_code, events::key_mod_t mod, const Ev& event) {
        add_handler (m_pressed_handler, scan_code, mod, [this, event] {
          this->notify (event);
        });
      }

      template <typename Ev>
      void when_pressed (events::scan_code_t scan_code, events::key_mod_t mod,
                         std::function<Ev (const events::keyboard& ev)> event_factory) {
        add_handler (m_pressed_handler, scan_code, mod, [this, event_factory](const events::keyboard& ev) {
          this->notify (event_factory(ev));
        });
      }

      template <typename Ev>
      void when_released (events::scan_code_t scan_code, const Ev& event) {
        add_handler (m_released_handler, scan_code, [this, event] {
          this->notify (event);
        });
      }

      template <typename Ev>
      void when_released (events::scan_code_t scan_code, std::function<Ev (const events::keyboard& ev)> event_factory) {
        add_handler (m_released_handler, scan_code, [this, event_factory](const events::keyboard& ev) {
          this->notify (event_factory(ev));
        });
      }

      template <typename Ev>
      void when_released (events::scan_code_t scan_code, events::key_mod_t mod, const Ev& event) {
        add_handler (m_released_handler, scan_code, mod, [this, event] {
          this->notify (event);
        });
      }

      template <typename Ev>
      void when_released (events::scan_code_t scan_code, events::key_mod_t mod,
                          std::function<Ev (const events::keyboard& ev)> event_factory) {
        add_handler (m_released_handler, scan_code, mod, [this, event_factory](const events::keyboard& ev) {
          this->notify (event_factory(ev));
        });
      }

    protected:
      void on_keyboard_input (const kernel::events::keyboard& ev) override {
        if (ev.pressed) {
          react_to_keyboard_input (m_pressed_handler, ev);
        } else {
          react_to_keyboard_input (m_released_handler, ev);
        }
      }

    private:

      using fire_event_t = std::function<void ()>;
      using construct_and_fire_event_t = std::function<void (const events::keyboard&)>;
      using fire_functor_t = std::variant<fire_event_t, construct_and_fire_event_t>;

      struct keyboard_mod_handler {

        keyboard_mod_handler(events::key_mod_t mod, fire_event_t f)
        : m_mod(mod), m_handler(f) {}

        keyboard_mod_handler(events::key_mod_t mod, construct_and_fire_event_t f)
            : m_mod(mod), m_handler(f) {}

        events::key_mod_t m_mod;
        fire_functor_t m_handler;
      };

      struct keyboard_handler {
        explicit keyboard_handler (fire_event_t f)
        : m_handler(f) {}

        explicit keyboard_handler (construct_and_fire_event_t f)
        : m_handler(f) {}

        keyboard_handler(events::key_mod_t mod, fire_event_t f)
        : m_mod_handler({mod, f}){}

        keyboard_handler(events::key_mod_t mod, construct_and_fire_event_t f)
        : m_mod_handler({mod, f}){}

        std::optional<fire_functor_t> m_handler;
        std::optional<keyboard_mod_handler> m_mod_handler;
      };

      using keyboard_dispatcher_t = std::map<events::scan_code_t, keyboard_handler>;
      keyboard_dispatcher_t m_pressed_handler;
      keyboard_dispatcher_t m_released_handler;
    private:
      template <typename Fn>
      static void add_handler (keyboard_dispatcher_t & map,
                               events::scan_code_t scan_code,
                               Fn handler) {
        auto itr = map.find(scan_code);
        if (itr == map.end()) {
          map.insert ({scan_code, keyboard_handler{handler}});
        } else {
          if (itr->second.m_handler) {
            RAISE_EX("handler is already defined for", neutrino::hal::events::s11n<events::scan_code_t>::to_string (scan_code));
          }
          itr->second.m_handler = handler;
        }
      }

      template <typename Fn>
      static void add_handler (keyboard_dispatcher_t & map,
                               events::scan_code_t scan_code,
                               events::key_mod_t mod,
                               Fn handler) {

        auto itr = map.find(scan_code);
        if (itr == map.end()) {
          map.insert ({scan_code, keyboard_handler{mod, handler}});
        } else {
          if (itr->second.m_mod_handler) {
            RAISE_EX("Handler already is defined for",
                     neutrino::hal::events::s11n<events::key_mod_t>::to_string (mod),
                     "+",
                     neutrino::hal::events::s11n<events::scan_code_t>::to_string (scan_code)
                     );
          } else {
            itr->second.m_mod_handler = keyboard_mod_handler{mod, handler};
          }
        }
      }

      static void react_to_keyboard_input (const keyboard_dispatcher_t & map, const kernel::events::keyboard& ev) {
          auto itr = map.find (ev.code);
          if (itr == map.end()) {
            return;
          }
          const fire_functor_t* functor = nullptr;
          if (itr->second.m_mod_handler) {
            if (ev.mod & itr->second.m_mod_handler->m_mod) {
              functor = &itr->second.m_mod_handler->m_handler;
            }
          } else {
            if (itr->second.m_handler) {
              functor = &itr->second.m_handler.value();
            }
          }
          if (functor) {
            std::visit (
                neutrino::utils::overload (
                    [] (const fire_event_t& f) {
                      f ();
                    },
                    [&ev] (const construct_and_fire_event_t& f) {
                      f (ev);
                    }
                ),
                *functor
            );
          }
      }
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_SYSTEMS_INPUT_SYSTEM_HH
