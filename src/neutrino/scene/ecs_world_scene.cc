//
// Created by igor on 10/4/24.
//

#include <neutrino/scene/ecs_world_scene.hh>
#include <neutrino/modules/video/components/world_renderer_camera.hh>

namespace neutrino {

	void ecs_world_scene::render(sdl::renderer& renderer) {
		if (!m_world_renderer) {
			m_world_renderer = init_world_renderer(renderer);
			auto viewport = m_world_renderer->get_world_viewport();
			world_renderer_camera camera(viewport.offset(), viewport.area());
			neutrino::ecs::entity_builder builder(get_registry());
			m_camera_ent_id = builder
				.with_component<world_renderer_camera>(camera)
				.build();
			init_with_renderer(renderer);
		}
		auto camera = get_registry().get_component<world_renderer_camera>(m_camera_ent_id);
		(void)m_world_renderer->set_camera(camera.camera_position);
		m_world_renderer->present();
		draw(renderer);
	}

	ecs_world_scene::ecs_world_scene()
	: m_camera_ent_id(ecs::INVALID_ENTITY_VALUE) {
	}

	void ecs_world_scene::update(std::chrono::milliseconds delta_time) {
		ecs_scene::update(delta_time);
		if (m_world_renderer) {
			m_world_renderer->update(delta_time);
		}
	}
}