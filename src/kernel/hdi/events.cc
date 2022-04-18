//
// Created by igor on 18/04/2022.
//

#include <unordered_set>
#include <neutrino/kernel/hdi/events.hh>
#include <neutrino/utils/mp/type_name/type_name.hpp>
#include "kbd_actions.hh"

namespace neutrino {
  struct events_holder::impl {
    impl() = default;

    void clear() {
      std::unordered_set<kbd_actions, kbd_actions_hash> tmp_keys;
      m_keys.swap (tmp_keys);

      std::unordered_set<uint32_t> tmp_events;
      m_events.swap (tmp_events);

      m_pointer_event = std::nullopt;
    }

    mouse_t get_mouse() const {
      mouse_data m{m_pointer_event->button, m_pointer_event->state, m_pointer_event->point[0], m_pointer_event->point[1]};
      return m;
    }

    std::unordered_set<kbd_actions, kbd_actions_hash> m_keys;
    std::unordered_set<uint32_t> m_events;
    std::optional<hal::events::pointer> m_pointer_event;
  };

  events_holder::events_holder () {
    m_pimpl = spimpl::make_unique_impl<impl>();
  }

  events_holder::~events_holder () = default;

  bool events_holder::operator [] (std::string_view x) const {
    return (m_pimpl->m_events.find (neutrino::utils::detail::crc32 (x)) != m_pimpl->m_events.end());
  }

  bool events_holder::pressed (scan_code_t btn) const {
    return m_pimpl->m_keys.find (kbd_actions(true, key_mod_t::NONE, btn)) != m_pimpl->m_keys.end();
  }

  bool events_holder::pressed (key_mod_t mod, scan_code_t btn) const {
    return m_pimpl->m_keys.find (kbd_actions(true, mod, btn)) != m_pimpl->m_keys.end();
  }

  bool events_holder::released (scan_code_t btn) const {
    return m_pimpl->m_keys.find (kbd_actions(false, key_mod_t::NONE, btn)) != m_pimpl->m_keys.end();
  }

  bool events_holder::released (key_mod_t mod, scan_code_t btn) const {
    return m_pimpl->m_keys.find (kbd_actions(false, mod, btn)) != m_pimpl->m_keys.end();
  }

  void events_holder::reset() {
    m_pimpl->clear();
  }

  void events_holder::set(std::string_view x) {
    m_pimpl->m_events.insert(neutrino::utils::detail::crc32 (x));
  }

  void events_holder::set(bool is_pressed, key_mod_t mod, scan_code_t btn) {
    m_pimpl->m_keys.insert(kbd_actions(is_pressed, mod, btn));
  }

  void events_holder::set (const hal::events::pointer& ev) {
    m_pimpl->m_pointer_event = ev;
  }

  mouse_t events_holder::operator [] (pointer_state_t x) const {
    if (!m_pimpl->m_pointer_event) {
      return {};
    }
    if (m_pimpl->m_pointer_event->state == x) {
      return m_pimpl->get_mouse();
    }
    return {};
  }

  mouse_t events_holder::operator [] (pointer_button_t x) const {
    if (!m_pimpl->m_pointer_event) {
      return {};
    }
    if (m_pimpl->m_pointer_event->button == x) {
      return m_pimpl->get_mouse();
    }
    return {};
  }

  mouse_t events_holder::pressed (pointer_button_t btn) const {
    if (!m_pimpl->m_pointer_event) {
      return {};
    }
    if (m_pimpl->m_pointer_event->state == pointer_state_t::PRESSED && m_pimpl->m_pointer_event->button == btn) {
      return m_pimpl->get_mouse();
    }
    return {};
  }

  mouse_t events_holder::moved (pointer_button_t btn) const {
    if (!m_pimpl->m_pointer_event) {
      return {};
    }
    if (m_pimpl->m_pointer_event->state == pointer_state_t::MOTION && m_pimpl->m_pointer_event->button == btn) {
      return m_pimpl->get_mouse();
    }
    return {};
  }

  mouse_t events_holder::moved () const {
    if (!m_pimpl->m_pointer_event) {
      return {};
    }
    if (m_pimpl->m_pointer_event->state == pointer_state_t::MOTION) {
      return m_pimpl->get_mouse();
    }
    return {};
  }

  mouse_t events_holder::released (pointer_button_t btn) const {
    if (!m_pimpl->m_pointer_event) {
      return {};
    }
    if (m_pimpl->m_pointer_event->state == pointer_state_t::RELEASED && m_pimpl->m_pointer_event->button == btn) {
      return m_pimpl->get_mouse();
    }
    return {};
  }
}