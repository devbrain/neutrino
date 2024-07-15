//
// Created by igor on 7/15/24.
//

#ifndef  NEUTRINO_EVENTS_HH
#define  NEUTRINO_EVENTS_HH

#include <memory>
#include <variant>
#include <neutrino/scene/scene.hh>

namespace neutrino {
	struct push_scene_event {
		std::shared_ptr<scene> scene_obj;
	};

	struct replace_scene_event {
		std::shared_ptr<scene> scene_obj;
	};

	struct pop_scene_event {};

	struct scene_timer_event {
		scene* destination;
		void* data;
	};

	using neutrino_event_t = std::variant<std::monostate, push_scene_event, pop_scene_event, replace_scene_event, scene_timer_event>;

	void generate_event(const push_scene_event& ev);
	void generate_event(const replace_scene_event& ev);
	void generate_event(pop_scene_event ev);
	void generate_event(scene_timer_event ev);

	neutrino_event_t parse_event(const sdl::events::event_t& ev);

}





#endif
