//
// Created by igor on 8/7/24.
//

#ifndef NEUTRINO_MODULES_VIDEO_SYSTEMS_SPRITE_SYSTEM_HH
#define NEUTRINO_MODULES_VIDEO_SYSTEMS_SPRITE_SYSTEM_HH

#include <neutrino/ecs/abstract_system.hh>
#include <neutrino/modules/video/components/sprite_component.hh>

namespace neutrino {
    class NEUTRINO_EXPORT sprite_system : public ecs::abstract_system {
        protected:
            static void _update(animated_sprite& sprite, std::chrono::milliseconds delta_t);
            static void _present(const animated_sprite& sprite, sdl::renderer& r, int x, int y,
                                 const texture_atlas& atlas);
            static void _present(const tile& sprite, sdl::renderer& r, int x, int y, const texture_atlas& atlas);

        private:
            void update(ecs::registry& registry, std::chrono::milliseconds delta_t, const sdl::rect& viewport) override;
            void present(ecs::registry& registry, sdl::renderer& renderer, const sdl::rect& viewport,
                         const texture_atlas& atlas) override;
    };
}

#endif
