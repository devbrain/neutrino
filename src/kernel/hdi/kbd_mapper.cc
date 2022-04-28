//
// Created by igor on 18/04/2022.
//

#include "kbd_mapper.hh"
#include <iostream>
#include <neutrino/hal/events/events_s11n.hh>

namespace neutrino {
  kbd_mapper::kbd_mapper(events_holder& ev)
  : m_events(ev) {
  }

  void kbd_mapper::when_pressed(key_mod_t mod, scan_code_t code, std::string_view event) {
    m_data.insert (std::make_pair(kbd_actions(true, mod, code), std::string(event)));
  }

  void kbd_mapper::when_pressed(scan_code_t code, std::string_view event) {
    when_pressed (key_mod_t::NONE, code, event);
  }

  void kbd_mapper::when_released(key_mod_t mod, scan_code_t code, std::string_view event) {
    m_data.insert (std::make_pair(kbd_actions(false, mod, code), std::string(event)));
  }

  void kbd_mapper::when_released(scan_code_t code, std::string_view event) {
    when_released (key_mod_t::NONE, code, event);
  }

  void kbd_mapper::handle_event(const hal::events::keyboard& ev) {
    kbd_actions a(ev);

    if (ev.pressed && (ev.mod & hal::events::key_mod_t ::LALT) && (ev.code == hal::events::scan_code_t::F) ) {
      std::cout << ev.pressed << ":"
                << neutrino::hal::events::s11n<key_mod_t>::to_string (ev.mod) << ":"
                << neutrino::hal::events::s11n<scan_code_t>::to_string (ev.code) << std::endl;
    }

    auto itr = m_data.find (a);
    if (itr != m_data.end()) {
      m_events.set (std::string_view (itr->second.c_str(), itr->second.size()));
    }
    m_events.set(a.pressed, a.mod, a.scan_code);
  }

  // visitor(is_pressed, mod, code, event)
  void kbd_mapper::visit(const std::function<void(bool, const std::string&, const std::string&, const std::string&)>& visitor) {
    for (const auto& [a, ev] : m_data) {
      auto k = hal::events::s11n<scan_code_t>::to_string (a.scan_code);
      if (a.mod == key_mod_t::NONE) {
        visitor (a.pressed, "", k, ev);
      } else {
        visitor (a.pressed, hal::events::s11n<key_mod_t>::to_string (a.mod), k, ev);
      }
    }
  }
}