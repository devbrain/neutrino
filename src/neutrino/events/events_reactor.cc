//
// Created by igor on 7/13/24.
//

#include <neutrino/events/events_reactor.hh>

namespace neutrino {
	detail::basic_event_handler::~basic_event_handler() = default;

	void events_reactor::reset() {
		for (auto& [_, v] : m_handlers) {
			v->reset();
		}
	}

	void events_reactor::handle(const sdl::events::event_t& ev) {
		for (auto& [_, v] : m_handlers) {
			v->handle(ev);
		}
	}
}
