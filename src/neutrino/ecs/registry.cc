//
// Created by igor on 7/28/24.
//
#include <neutrino/ecs/registry.hh>

namespace neutrino::ecs {
    detail::underlying_entity_type entity_builder::s_entity_counter = 0;

    registry::entity_description::entity_description(std::size_t max_components, std::size_t index)
        : components(max_components, false) {
        components[index] = true;
    }

    entity_builder::entity_builder(registry& registry_)
        : m_registry(registry_) {
    }

    entity_id_t entity_builder::build() const {
        return entity_id_t(s_entity_counter++);
    }

    registry::registry(std::size_t max_components)
        : m_max_components(max_components) {

    }

    bool registry::exists(entity_id_t e) const {
        auto itr = m_ents_map.find(e.value_of());
        return itr != m_ents_map.end();
    }

    void registry::remove_entity(entity_id_t e) {
        auto ei = m_ents_map.find(e.value_of());
        if (ei == m_ents_map.end()) {
            return;
        }
        for (std::size_t i=0; i<ei->second.components.size(); i++) {
            if (ei->second.components[i]) {
                auto itr = m_components_names_map.find(i);
                if (itr != m_components_names_map.end()) {
                    auto bucket = m_components.find(itr->second.bucket);
                    if (bucket != m_components.end()) {
                        bucket->second->destruct(e);
                    }
                }

            }
        }
    }

    std::vector<std::string> registry::list_components(entity_id_t e) const {
        auto ei = m_ents_map.find(e.value_of());
        ENFORCE(ei != m_ents_map.end());
        std::vector<std::string> out;
        for (std::size_t i=0; i<ei->second.components.size(); i++) {
            if (ei->second.components[i]) {
                auto itr = m_components_names_map.find(i);
                ENFORCE(itr != m_components_names_map.end());
                out.emplace_back(itr->second.name);
            }
        }
        return out;
    }
}
