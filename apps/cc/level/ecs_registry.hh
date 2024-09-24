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

        neutrino::ecs::registry& get_registry();
        [[nodiscard]] const neutrino::ecs::registry& get_registry() const;
        void clear();
    private:
        neutrino::ecs::registry m_registry;
        neutrino::ecs::entity_id_t m_player;
};

#endif
