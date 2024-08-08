//
// Created by igor on 7/4/24.
//

#ifndef  NEUTRINO_MODULES_VIDEO_WORLD_OBJECTS_LAYER_HH
#define  NEUTRINO_MODULES_VIDEO_WORLD_OBJECTS_LAYER_HH

#include <chrono>
#include <sdlpp/video/geometry.hh>
#include <sdlpp/video/render.hh>

#include <neutrino/ecs/registry.hh>
#include <neutrino/ecs/systems_holder.hh>
#include <neutrino/modules/video/texture_atlas.hh>
#include <neutrino/neutrino_export.hh>

namespace neutrino {
    class NEUTRINO_EXPORT world_renderer;
}

namespace neutrino::tiled {
    class NEUTRINO_EXPORT objects_layer {
        friend class neutrino::world_renderer;

        public:
            explicit objects_layer(ecs::registry& ecs_registry);
            objects_layer(objects_layer&&) = default;
            ~objects_layer();

            template<typename System, typename... Args>
            void register_system(Args&&... args) {
                m_holder.register_system <System>(std::forward <Args>(args)...);
            }

        private:
            void update(std::chrono::milliseconds delta_t);
            void present();

        private:
            ecs::registry& m_registry;
            ecs::systems_holder m_holder;
    };
}

#endif
