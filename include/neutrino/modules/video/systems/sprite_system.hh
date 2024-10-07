//
// Created by igor on 8/7/24.
//

#ifndef NEUTRINO_MODULES_VIDEO_SYSTEMS_SPRITE_SYSTEM_HH
#define NEUTRINO_MODULES_VIDEO_SYSTEMS_SPRITE_SYSTEM_HH

#include <neutrino/modules/video/systems/video_system.hh>
#include <neutrino/modules/video/components/sprite_component.hh>

namespace neutrino::ecs::video {
    class NEUTRINO_EXPORT sprite_system : public video_system {
        protected:
            static void _update(animated_sprite& sprite, std::chrono::milliseconds delta_t);
			static void _update(sprite_bank& sprite, std::chrono::milliseconds delta_t);
        private:
            void update(registry& registry, std::chrono::milliseconds delta_t) override;
            void present(registry& registry) override;
    };
}

#endif
