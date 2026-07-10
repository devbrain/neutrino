//
// Created by igor on 04/07/2026.
//

#include <neutrino/video/sprite/texture_atlas.hh>
#include <failsafe/enforce.hh>

#include "services/service_locator.hh"
#include "video/sprite/sprites_manager.hh"
#include "video/sprite/texture_registry.hh"

namespace neutrino {
    gpu_texture_atlas_id register_atlas(
        const cpu_texture_atlas& atlas,
        atlas_texture_format format) {
        auto* registry = service_locator::instance().get_texture_registry();
        ENFORCE(registry != nullptr);
        return registry->create(service_locator::instance().get_renderer(), atlas, format);
    }

    void unregister_atlas(gpu_texture_atlas_id atlas) {
        if (!atlas.valid()) {
            return;
        }

        auto& services = service_locator::instance();
        auto* registry = services.get_texture_registry();
        if (registry == nullptr) {
            return; // services already torn down: the GPU texture is gone, nothing to do
        }

        if (auto* sprites = services.get_sprites_manager()) {
            ENFORCE(!sprites->uses(atlas))("Cannot unregister texture atlas while a sprite sheet still uses it");
        }

        registry->erase(atlas);
    }
}
