//
// Created by igor on 8/7/24.
//

#ifndef NEUTRINO_ECS_ABSTRACT_SYSTEM_HH
#define NEUTRINO_ECS_ABSTRACT_SYSTEM_HH

#include <chrono>
#include <sdlpp/sdlpp.hh>

#include <neutrino/ecs/registry.hh>
#include <neutrino/neutrino_export.hh>
#include <neutrino/modules/video/texture_atlas.hh>

namespace neutrino::ecs {
    class NEUTRINO_EXPORT abstract_system {
        public:
            virtual ~abstract_system();

            virtual void update(registry& registry, std::chrono::milliseconds delta_t) = 0;
            virtual void present(registry& registry) = 0;
    };
}

#endif
