//
// Created by igor on 04/07/2026.
//

#include <neutrino/video/sprite/texture_atlas.hh>
#include <failsafe/enforce.hh>

#include "services/service_access.hh"
#include "video/sprite/sprites_manager.hh"
#include "video/sprite/texture_registry.hh"

namespace neutrino {
    atlas_id register_atlas(
        image_atlas&& atlas,
        atlas_texture_format format) {
        return require_texture_registry().add(
            service_locator::instance().get_renderer(), std::move(atlas), format);
    }

    void unregister_atlas(atlas_id atlas) {
        if (!atlas.valid()) {
            return;
        }
        // Services already torn down: the GPU texture is gone, nothing to do.
        auto* registry = maybe_texture_registry();
        if (registry == nullptr) {
            return;
        }
        if (auto* sprites = maybe_sprites_manager()) {
            ENFORCE(!sprites->uses(atlas), "Cannot unregister texture atlas while a sprite sheet still uses it");
        }
        registry->erase(atlas);
    }

    dim picture_size(atlas_id atlas, std::size_t picture_idx) {
        return require_texture_registry().picture_size(atlas, picture_idx);
    }
}
