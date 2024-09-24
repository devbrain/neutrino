//
// Created by igor on 9/12/24.
//

#include <neutrino/events/hotkey_mapper.hh>
#include <neutrino/events/events.hh>
#include <neutrino/events/detail/keyboard_state.hh>

namespace neutrino {
    hotkey_mapper::hotkey_mapper() : m_max_id(0) {

	}

    void hotkey_mapper::register_hotkey(const config_hotkey& hotkey, int hotkey_id) {
		ENFORCE(hotkey_id >= 0);
        for (auto& item : m_hotkeys) {
            if (item.second == hotkey_id) {
                item.first = hotkey;
                return;
            }
        }
        m_hotkeys.emplace_back(hotkey, hotkey_id);
		if (m_max_id < hotkey_id) {
			m_max_id = hotkey_id;
		}
    }

    void hotkey_mapper::unregister_hotkey(int hotkey_id) {
        for (auto itr = m_hotkeys.begin(); itr != m_hotkeys.end(); ++itr) {
            if (itr->second == hotkey_id) {
                m_hotkeys.erase(itr);
                return;
            }
        }
    }

    void hotkey_mapper::add_to_reactor(events_reactor& reactor) {
        register_event(reactor, [this](const sdl::events::keyboard& kbd, hotkey_pressed_event& ev) {
            if (kbd.pressed) {
                return this->handle(ev);
            }
		  	ev.hotkey_id = -1;
		  	std::fill(ev.hotkeys.begin(), ev.hotkeys.end(), false);
            return false;
        });
    }

    bool hotkey_mapper::handle(hotkey_pressed_event& ev) {
        bool rc = false;
		if (ev.hotkeys.size() < m_max_id + 1) {
			ev.hotkeys.resize(m_max_id + 1, false);
		}
        for (const auto& [key_conf, ev_id] : m_hotkeys) {
            if (key_conf.scancode && key_conf.keymod) {
                auto s = detail::keyboard_state_check(*key_conf.scancode);
                auto m = detail::keyboard_state_check(*key_conf.keymod);
                rc = s && m;
            } else if (key_conf.scancode && !key_conf.keymod) {
                rc = detail::keyboard_state_check(*key_conf.scancode);
            } else if (!key_conf.scancode && key_conf.keymod) {
                rc = detail::keyboard_state_check(*key_conf.keymod);
            }
            if (rc) {
                ev.hotkey_id = ev_id;
				ev.hotkeys[ev_id] = true;
                return true;;
            }
        }
        ev.hotkey_id = -1;
		std::fill(ev.hotkeys.begin(), ev.hotkeys.end(), false);
        return false;
    }
}
