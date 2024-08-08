//
// Created by igor on 7/30/24.
//

#include <neutrino/modules/video/world/objects_layer.hh>
#include <neutrino/modules/video/systems/sprite_system.hh>

namespace neutrino::tiled {

    objects_layer::objects_layer(ecs::registry& ecs_registry)
        : m_registry(ecs_registry) {
    }

    objects_layer::~objects_layer() = default;

    void objects_layer::update(std::chrono::milliseconds delta_t) {
        m_holder.update(m_registry, delta_t);
    }

    void objects_layer::present() {
        m_holder.present(m_registry);
    }


}
