//
// Created by igor on 8/7/24.
//

#ifndef NEUTRINO_ECS_SYSTEMS_HOLDER_HH
#define NEUTRINO_ECS_SYSTEMS_HOLDER_HH

#include <vector>
#include <memory>
#include <neutrino/ecs/abstract_system.hh>

namespace neutrino::ecs {
    class NEUTRINO_EXPORT systems_holder {
        public:
            systems_holder() = default;
            systems_holder(systems_holder&& ) = default;

            template <typename System, typename ... Args>
            void register_system(Args&& ... args) {
                m_systems.emplace_back(std::make_unique<System>(std::forward<Args>(args)...));
            }

            void update(registry& registry, std::chrono::milliseconds delta_t);
            void present(registry& registry);
        private:
            std::vector <std::unique_ptr <abstract_system>> m_systems;
    };
}

#endif
