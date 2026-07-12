//
// Created by igor on 04/07/2026.
//

#include <neutrino/video/sprite/texture_atlas.hh>
#include <failsafe/enforce.hh>

#include "services/service_access.hh"
#include "video/sprite/sprites_manager.hh"
#include "video/sprite/texture_registry.hh"

namespace neutrino {
    gpu_texture_atlas_id register_atlas(
        const cpu_texture_atlas& atlas,
        atlas_texture_format format) {
        return require_texture_registry().create(
            service_locator::instance().get_renderer(), atlas, format);
    }

    void unregister_atlas(gpu_texture_atlas_id atlas) {
        if (!atlas.valid()) {
            return;
        }
        // Services already torn down: the GPU texture is gone, nothing to do.
        auto* registry = maybe_texture_registry();
        if (registry == nullptr) {
            return;
        }
        if (auto* sprites = maybe_sprites_manager()) {
            ENFORCE(!sprites->uses(atlas))("Cannot unregister texture atlas while a sprite sheet still uses it");
        }
        registry->erase(atlas);
    }
}
