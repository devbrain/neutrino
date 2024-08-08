//
// Created by igor on 8/7/24.
//

#ifndef NEUTRINO_MODULES_VIDEO_SYSTEMS_SPRITE_SYSTEM_HH
#define NEUTRINO_MODULES_VIDEO_SYSTEMS_SPRITE_SYSTEM_HH

#include <neutrino/modules/video/systems/world_rendering_system.hh>
#include <neutrino/modules/video/components/sprite_component.hh>

namespace neutrino::ecs {
    class NEUTRINO_EXPORT sprite_system : public world_rendering_system {
        public:
            explicit sprite_system(const world_renderer& world_renderer_);

        protected:
            static void _update(animated_sprite& sprite, std::chrono::milliseconds delta_t);
            static void _present(const animated_sprite& sprite, sdl::renderer& r, int x, int y,
                                 const texture_atlas& atlas);
            static void _present(const tile& sprite, sdl::renderer& r, int x, int y, const texture_atlas& atlas);

        private:
            void update(registry& registry, std::chrono::milliseconds delta_t) override;
            void present(registry& registry) override;
    };
}

#endif
