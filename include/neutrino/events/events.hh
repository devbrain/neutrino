//
// Created by igor on 7/13/24.
//

#ifndef NEUTRINO_EVENTS_HH
#define NEUTRINO_EVENTS_HH

#include <functional>
#include <neutrino/events/events_reactor.hh>

namespace neutrino {

	template <typename ...Callable>
	std::enable_if_t<detail::is_producers_v<Callable...>, void> register_event(events_reactor& reactor, Callable ... f) {
		reactor.register_handler(std::forward<Callable>(f)...);
	}

	template<typename EventType>
	const EventType* get_event(events_reactor& reactor) {
		return reactor.get<EventType>();
	}
}

#endif
