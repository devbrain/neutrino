//
// Created by igor on 10/4/24.
//

#include <neutrino/scene/ecs_scene.hh>

namespace neutrino {

	ecs::registry& ecs_scene::get_registry() {
		return m_registry;
	}

	const ecs::registry& ecs_scene::get_registry() const {
		return m_registry;
	}

	void ecs_scene::update(std::chrono::milliseconds delta_time) {
		for (auto& sys_ptr : m_systems) {
			sys_ptr->update(m_registry, delta_time);
		}
	}

	void ecs_scene::initialize() {
		initialize_scene();
		initialize_systems();
		start_systems();
		post_initialize();
	}

	void ecs_scene::post_initialize() {

	}

	void ecs_scene::start_systems() {
		for (auto& sys_ptr : m_systems) {
			sys_ptr->start(m_registry);
		}
	}
}