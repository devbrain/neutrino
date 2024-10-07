//
// Created by igor on 10/4/24.
//

#ifndef NEUTRINO_INCLUDE_NEUTRINO_SCENE_ECS_WORLD_SCENE_HH_
#define NEUTRINO_INCLUDE_NEUTRINO_SCENE_ECS_WORLD_SCENE_HH_

#include <neutrino/scene/ecs_scene.hh>
#include <neutrino/modules/video/world_renderer.hh>

namespace neutrino {
	class NEUTRINO_EXPORT ecs_world_scene : public ecs_scene {
	 public:
		ecs_world_scene();
	 protected:
		virtual void draw(neutrino::sdl::renderer& renderer) = 0;
		virtual std::unique_ptr<world_renderer> init_world_renderer(neutrino::sdl::renderer& renderer) = 0;
		virtual void init_with_renderer(sdl::renderer& renderer) = 0;

		void update(std::chrono::milliseconds delta_time) override;
		void render(neutrino::sdl::renderer& renderer) override;
	 private:
		ecs::entity_id_t m_camera_ent_id;
		std::unique_ptr<world_renderer> m_world_renderer;
	};
}

#endif
