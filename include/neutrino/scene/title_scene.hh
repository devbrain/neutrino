//
// Created by igor on 7/16/24.
//

#ifndef NEUTRINO_SCENE_TITLE_SCENE_HH
#define NEUTRINO_SCENE_TITLE_SCENE_HH

#include <chrono>
#include <neutrino/systems/video/types.hh>
#include <neutrino/scene/scene.hh>
#include <neutrino/neutrino_export.hh>

namespace neutrino {
	class NEUTRINO_EXPORT title_scene : public neutrino::scene {
		struct key_pressed {
		};

		public:
			title_scene(texture_id_t tid, std::chrono::seconds display_time);
			~title_scene() override;
		private:
			void update(std::chrono::milliseconds delta_time) override;

			void render(sdl::renderer& renderer) override;

			void initialize() override;

			virtual void when_done() = 0;

		private:
			texture_id_t m_tid;
			std::chrono::milliseconds m_time_in_screen;
			std::chrono::seconds m_time_to_show;
	};
}

#endif
