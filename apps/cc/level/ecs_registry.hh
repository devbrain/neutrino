//
// Created by igor on 8/25/24.
//

#ifndef  ECS_REGISTRY_HH
#define  ECS_REGISTRY_HH

#include "neutrino/ecs/registry.hh"


class ecs_registry {
    public:
        ecs_registry();
        [[nodiscard]] neutrino::ecs::entity_id_t get_player() const;
        void set_player(neutrino::ecs::entity_id_t v);

        neutrino::ecs::registry& get_world();
        [[nodiscard]] const neutrino::ecs::registry& get_world() const;
        void clear();

		template <typename Component>
		Component& get_component();

		template <typename Component>
		const Component& get_component() const;
    private:
        neutrino::ecs::registry m_registry;
        neutrino::ecs::entity_id_t m_player;
};

template <typename Component>
Component& ecs_registry::get_component() {
	return m_registry.get_component<Component>(m_player);
}

template <typename Component>
const Component& ecs_registry::get_component() const {
	return m_registry.get_component<Component>(m_player);
}

#endif
