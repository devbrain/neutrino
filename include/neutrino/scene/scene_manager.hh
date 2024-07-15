//
// Created by igor on 7/14/24.
//

#ifndef NEUTRINO_SCENE_MANAGER_HH
#define NEUTRINO_SCENE_MANAGER_HH

#include <memory>
#include <vector>
#include <neutrino/scene/scene.hh>
#include <neutrino/neutrino_export.hh>

#include "events/neutrino_events.hh"

namespace neutrino {
	struct scene_timer_event;

	class NEUTRINO_EXPORT scene_manager {
		public:
			scene_manager();
			~scene_manager();

			scene_manager(const scene_manager&) = delete;
			scene_manager& operator =(const scene_manager&) = delete;

			void push(const std::shared_ptr<scene>& scene_);
			std::shared_ptr<scene> pop();
			std::shared_ptr<scene> top();

			[[nodiscard]] bool empty() const;

			void render(sdl::renderer& renderer);
			void handle_input(const sdl::events::event_t& ev);
			void update(std::chrono::milliseconds delta_time);
		private:
			void update_timers(const scene_timer_event& e);
		private:
			std::vector <std::shared_ptr<scene>> m_stack;
	};
}

#endif
