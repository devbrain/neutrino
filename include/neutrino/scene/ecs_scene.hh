//
// Created by igor on 10/4/24.
//

#ifndef  NEUTRINO_INCLUDE_NEUTRINO_SCENE_ECS_SCENE_HH_
#define  NEUTRINO_INCLUDE_NEUTRINO_SCENE_ECS_SCENE_HH_

#include <memory>
#include <vector>

#include <neutrino/scene/scene.hh>
#include <neutrino/ecs/registry.hh>
#include <neutrino/ecs/abstract_system.hh>

namespace neutrino {
	class NEUTRINO_EXPORT ecs_scene : public scene {
	 protected:
		virtual void initialize_systems() = 0;
		virtual void initialize_scene() = 0;
		virtual void post_initialize();

		ecs::registry& get_registry();
		const ecs::registry& get_registry() const;

		template <typename System, typename ... Args>
		void add_system(Args&& ... args) {
			m_systems.emplace_back(std::make_unique<System>(std::forward<Args>(args)...));
		}

		void update(std::chrono::milliseconds delta_time) override;
		void initialize() override;
		void start_systems();
	 private:
		ecs::registry m_registry;
		std::vector<std::unique_ptr<ecs::abstract_system>> m_systems;
	};
}


#endif
