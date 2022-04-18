//
// Created by igor on 19/04/2022.
//

#include "pointer_mapper.hh"

namespace neutrino {
  pointer_mapper::pointer_mapper(events_holder& ev)
  : m_events(ev),
  m_ignore_motion(false),
  m_ignore_clicks(false),
  m_ignore_releases(false)
  {

  }

  void pointer_mapper::ignore_motion(bool f) {
    m_ignore_motion = f;
  }

  void pointer_mapper::ignore_clicks(bool f) {
    m_ignore_clicks = f;
  }

  void pointer_mapper::ignore_releases(bool f) {
    m_ignore_releases = f;
  }

  void pointer_mapper::handle_event(const hal::events::pointer& ev) {
    switch (ev.state) {
      case pointer_state_t::MOTION:
        if (!m_ignore_motion) {
          m_events.set (ev);
        }
        break;
      case pointer_state_t::PRESSED:
        if (!m_ignore_clicks) {
          m_events.set (ev);
        }
        break;
      case pointer_state_t::RELEASED:
        if (!m_ignore_releases) {
          m_events.set (ev);
        }
        break;
    }
  }
}