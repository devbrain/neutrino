//
// Created by igor on 7/13/24.
//

#ifndef NEUTRINO_EVENTS_REACTOR_HH
#define NEUTRINO_EVENTS_REACTOR_HH

#include <memory>
#include <unordered_map>

#include <bsw/mp/type_name/type_name.hpp>
#include <bsw/exception.hh>
#include <bsw/logger/logger.hh>

#include <neutrino/neutrino_export.hh>
#include <neutrino/events/detail/event_producer.hh>

namespace neutrino {
	namespace detail {
		class NEUTRINO_EXPORT basic_event_handler {
			public:
				virtual ~basic_event_handler();
				virtual void handle(const sdl::events::event_t& ev) = 0;
				virtual void reset() = 0;
				virtual void* get_internal() = 0;
		};

		template<typename... Callable>
		class event_handler : public basic_event_handler {
			public:
				using tuple_t = producers_tuple <Callable...>;

			public:
				explicit event_handler(Callable... f)
					: m_holder(f...),
					  m_accepted(false) {
				}

				void* get_internal() override {
					if (!m_accepted) {
						return nullptr;
					}
					return &m_holder.get_event();
				}

				void reset() override {
					m_accepted = false;
				}

				void handle(const sdl::events::event_t& ev) override {
					if (m_holder.apply(ev)) {
						m_accepted = true;
					}
				}

			private:
				tuple_t m_holder;
				bool m_accepted;
		};
	}

	class NEUTRINO_EXPORT events_reactor {
		public:
			void reset();

			void handle(const sdl::events::event_t& ev);

			template<typename EventType>
			const EventType* get() const {
				static const std::string key(type_name_v <EventType>);
				const auto itr = m_handlers.find(key);
				if (itr == m_handlers.end()) {
					RAISE_EX("Event type ", type_name_v <EventType>, " was not registered");
				}
				return static_cast <const EventType*>(itr->second->get_internal());
			}

			template<typename... Callable>
			void register_handler(Callable... f) {
				using tuple_t = detail::producers_tuple <Callable...>;
				using event_type = typename tuple_t::event_type;
				static const std::string key(type_name_v <event_type>);
				EVLOG_TRACE(EVLOG_DEBUG, "Registering event handler for ", key);
				m_handlers.insert(std::make_pair(key,
				                                 std::make_unique <detail::event_handler <Callable...>>(
					                                 std::forward <Callable>(f)...)));
			}

		private:
			std::unordered_map <std::string, std::unique_ptr <detail::basic_event_handler>> m_handlers;
	};
}

#endif
