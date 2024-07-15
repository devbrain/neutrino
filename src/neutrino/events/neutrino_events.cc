//
// Created by igor on 7/15/24.
//

#include "neutrino_events.hh"
#include <sdlpp/events/events.hh>
#include <bsw/mp/type_name/type_name.hpp>

namespace neutrino {

	template <typename T>
	inline constexpr auto code_v = static_cast<int32_t>(type_hash_v<T>);

	void generate_event(const push_scene_event& ev) {
		sdl::events::user u(code_v<push_scene_event>, new push_scene_event(ev));
		sdl::push_event(u);
	}

	void generate_event(const replace_scene_event& ev) {
		sdl::events::user u(code_v<replace_scene_event>, new replace_scene_event(ev));
		sdl::push_event(u);
	}

	void generate_event(pop_scene_event ev) {
		sdl::events::user u(code_v<pop_scene_event>, nullptr);
		sdl::push_event(u);
	}

	void generate_event(scene_timer_event ev) {
		sdl::events::user u(code_v<scene_timer_event>, ev.destination, ev.data);
	}

	neutrino_event_t parse_event(const sdl::events::event_t& ev) {
		if (const auto* e = std::get_if<sdl::events::user>(&ev)) {
			switch (e->code) {
				case code_v<push_scene_event>: {
					auto transmitted = static_cast<push_scene_event*>(e->data1);
					push_scene_event out(*transmitted);
					delete transmitted;
					return out;
				}
				case code_v<replace_scene_event>: {
					auto transmitted = static_cast<replace_scene_event*>(e->data1);
					replace_scene_event out(*transmitted);
					delete transmitted;
					return out;
				}
				case code_v<pop_scene_event>:
					return pop_scene_event{};
				case code_v<scene_timer_event>:
					return scene_timer_event{static_cast<scene*>(e->data1), e->data2};
				default:
					return {};
			}
		}
		return {};
	}
}
