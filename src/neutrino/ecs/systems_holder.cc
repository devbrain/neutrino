//
// Created by igor on 8/7/24.
//

#include <neutrino/ecs/systems_holder.hh>

namespace neutrino::ecs {
    void systems_holder::update(registry& registry, std::chrono::milliseconds delta_t) {
        for (auto& sys_ptr : m_systems) {
            sys_ptr->update(registry, delta_t);
        }
    }

    void systems_holder::present(registry& registry) {
        for (auto& sys_ptr : m_systems) {
            sys_ptr->present(registry);
        }
    }
}
