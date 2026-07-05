//
// Created by igor on 04/07/2026.
//

#include <neutrino/video/sprite/texture_atlas.hh>
#include <failsafe/enforce.hh>

#include "services/service_locator.hh"
#include "video/sprite/texture_registry.hh"

namespace neutrino {
    gpu_texture_atlas_id register_atlas(
        const cpu_texture_atlas& atlas,
        atlas_texture_format format) {
        auto* registry = service_locator::instance().get_texture_registry();
        ENFORCE(registry != nullptr);
        return registry->create(service_locator::instance().get_renderer(), atlas, format);
    }
}
