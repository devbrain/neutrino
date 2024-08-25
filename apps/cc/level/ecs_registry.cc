//
// Created by igor on 8/25/24.
//

#include "ecs_registry.hh"
#include "components/component_player.hh"

static auto create_player(neutrino::ecs::registry& reg) {
    neutrino::ecs::entity_builder builder(reg);
    return builder.with_component <component_player>().build();
}

ecs_registry::ecs_registry()
    : m_registry{}, m_player(create_player(m_registry)) {
}

component_player& ecs_registry::get_player() {
    return m_registry.get_component<component_player>(m_player);
}

const component_player& ecs_registry::get_player() const {
    return m_registry.get_component<component_player>(m_player);
}

neutrino::ecs::registry& ecs_registry::get_registry() {
    return m_registry;
}

const neutrino::ecs::registry& ecs_registry::get_registry() const {
    return m_registry;
}

void ecs_registry::clear() {
    m_registry.remove_if([this](const neutrino::ecs::entity_id_t& e) {
        return e != m_player;
    });
}
