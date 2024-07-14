//
// Created by igor on 7/14/24.
//

#ifndef NEUTRINO_EVENTS_DETAILS_EVENT_PRODUCER_HH
#define NEUTRINO_EVENTS_DETAILS_EVENT_PRODUCER_HH

#include <functional>
#include <utility>
#include <tuple>
#include <sdlpp/events/system_events.hh>
#include <boost/callable_traits.hpp>
#include <bsw/mp/all_same.hh>


namespace neutrino::detail {
	namespace priv {
		template<typename A, typename B>
		struct belongs {
			static constexpr bool value = std::is_same_v <A, B>;
		};

		template<typename A, typename B>
		struct belongs <A, std::variant <B>> {
			static constexpr bool value = std::is_same_v <A, B>;
		};

		template<typename A, typename T0, typename... Tn>
		struct belongs <A, std::variant <T0, Tn...>> {
			static constexpr bool value = std::is_same_v <A, T0> || belongs <A, std::variant <Tn...>>::value;
		};
	}

	template<typename A>
	inline constexpr bool is_sdl_event_v = priv::belongs <A, sdl::events::event_t>::value;

	// ===============================================================
	namespace priv {
		template<typename T>
		struct is_const_ref {
			static constexpr bool value = false;
		};

		template<typename T>
		struct is_const_ref <const T&> {
			static constexpr bool value = true;
		};

		template<typename T>
		struct is_ref {
			static constexpr bool value = false;
		};

		template<typename T>
		struct is_ref <T&> {
			static constexpr bool value = true;
		};

		template<typename Args, std::size_t ArgsSize, bool RetTypeIsBool>
		struct is_event_creator_traits_helper {
			static constexpr bool value = false;
		};

		template<typename Args>
		struct is_event_creator_traits_helper <Args, 2, true> {
			using sdl_event_type = std::tuple_element_t <0, Args>;
			using event_type = std::tuple_element_t <1, Args>;

			static constexpr bool value =
				is_sdl_event_v <std::decay_t <sdl_event_type>> &&
				is_const_ref <sdl_event_type>::value && is_ref <event_type>::value;
		};

		template<typename Args, std::size_t ArgsSize, bool RetTypeIsBool>
		struct event_creator_traits_helper;

		template<typename Args>
		struct event_creator_traits_helper <Args, 2, true> {
			using sdl_event_type = std::tuple_element_t <0, Args>;
			using event_type = std::tuple_element_t <1, Args>;
		};
	}

	template<typename Callable>
	struct event_creator_traits {
		using args_t = boost::callable_traits::args_t <Callable>;
		static constexpr std::size_t num_args = std::tuple_size_v <args_t>;
		using retval_t = boost::callable_traits::return_type_t <Callable>;

		using helper = priv::event_creator_traits_helper <args_t, num_args, std::is_same_v <retval_t, bool>>;
		using sdl_event_type = std::decay_t <typename helper::sdl_event_type>;
		using event_type = std::decay_t <typename helper::event_type>;

		using fn_t = std::function <bool (const sdl_event_type&, event_type&)>;
	};

	template<typename Callable>
	struct is_event_creator_traits {
		using args_t = boost::callable_traits::args_t <Callable>;
		static constexpr std::size_t num_args = std::tuple_size_v <args_t>;
		using retval_t = boost::callable_traits::return_type_t <Callable>;

		constexpr static bool value = priv::is_event_creator_traits_helper <
			args_t, num_args, std::is_same_v <retval_t, bool>>::value;
	};

	namespace priv {
		template<typename... Callable>
		struct is_event_producer_helper {
			static constexpr auto all_producers = (is_event_creator_traits <Callable>::value && ...);
		};

		template<bool AllAreProducers, typename... Callable>
		struct is_same_event_helper {
			static constexpr auto value = false;
		};

		template<typename... Callable>
		struct is_same_event_helper <true, Callable...> {
			static constexpr bool is_same() {
				if constexpr (sizeof...(Callable) == 1) {
					return true;
				} else {
					return bsw::mp::all_same_v <typename event_creator_traits <Callable>::event_type...>;
				}
			}

			static constexpr auto value = is_same();
		};

		template<typename Tuple, typename Event, typename SdlEvents, std::size_t Index>
		struct apply {
			static bool call(const sdl::events::event_t& ev, Tuple& tuple, Event& my_event) {
				using sdl_event_t = bsw::mp::type_list_at_t <Index, SdlEvents>;
				if (auto* e = std::get_if <sdl_event_t>(&ev)) {
					return std::get <Index>(tuple)(*e, my_event);
				}
				return false;
			}
		};
	}

	template<typename... Callable>
	inline constexpr bool is_producers_v = priv::is_same_event_helper <
		priv::is_event_producer_helper <Callable...>::all_producers, Callable...>::value;

	template<typename... Callable>
	class producers_tuple {
		public:
			using holder_t = std::tuple <typename event_creator_traits <Callable>::fn_t...>;
			using sdl_events_t = bsw::mp::type_list <typename event_creator_traits <Callable>::sdl_event_type...>;
			using event_type = bsw::mp::type_list_at_t <
				0, bsw::mp::type_list <typename event_creator_traits <Callable>::event_type...>>;

			explicit producers_tuple(Callable... f)
				: m_producers(typename event_creator_traits <Callable>::fn_t{std::forward<Callable>(f)}...) {
			}

			bool apply(const sdl::events::event_t& ev) {
				return apply_tuple_impl(ev, std::make_integer_sequence <std::size_t, sizeof...(Callable)>{});
			}

			event_type& get_event() {
				return m_event;
			}

		private:
			template<std::size_t ... Is>
			bool apply_tuple_impl(const sdl::events::event_t& ev, std::index_sequence <Is...>) {
				return (priv::apply <holder_t, event_type, sdl_events_t, Is>::call(ev, m_producers, m_event) || ...);
			}

			holder_t m_producers;
			event_type m_event;
	};
}

#endif
