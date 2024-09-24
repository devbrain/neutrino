//
// Created by igor on 8/25/24.
//

#include "ecs_registry.hh"



ecs_registry::ecs_registry()
    : m_registry{}, m_player(0) {
}

neutrino::ecs::entity_id_t ecs_registry::get_player() const {
    return m_player;
}

void ecs_registry::set_player(neutrino::ecs::entity_id_t v) {
    m_player = v;
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
