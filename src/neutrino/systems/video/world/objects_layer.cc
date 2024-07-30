//
// Created by igor on 7/30/24.
//

#include <neutrino/systems/video/world/objects_layer.hh>

namespace neutrino::tiled {
    objects_layer::objects_layer(ecs::registry& ecs_registry)
        : m_registry(ecs_registry) {
    }

    objects_layer::~objects_layer() = default;

    ecs::registry& objects_layer::get_registry() {
        return m_registry;
    }

    const ecs::registry& objects_layer::get_registry() const {
        return m_registry;
    }
}
