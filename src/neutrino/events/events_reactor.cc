//
// Created by igor on 7/13/24.
//

#include <neutrino/events/events_reactor.hh>
#include <neutrino/events/detail/keyboard_state.hh>


namespace neutrino {
	detail::basic_event_handler::~basic_event_handler() = default;

	void events_reactor::reset() {
		detail::keyboard_state_reset();
		for (auto& [_, v] : m_handlers) {
			v->reset();
		}
	}

	void events_reactor::handle(const sdl::events::event_t& ev) {
		if (const auto* kbev = std::get_if<sdl::events::keyboard>(&ev)) {
			detail::keyboard_state_update(*kbev);
		}
		for (auto& [_, v] : m_handlers) {
			v->handle(ev);
		}
	}
}
