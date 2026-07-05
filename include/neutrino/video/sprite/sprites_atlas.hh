//
// Created by igor on 04/07/2026.
//

#pragma once

#include <compare>
#include <cstdint>
#include <limits>
#include <neutrino/neutrino_export.h>
#include <neutrino/video/sprite/cpu_sprites_atlas.hh>

namespace neutrino {
    enum class atlas_texture_format {
        automatic,
        rgb,
        rgba,
        indexed8
    };

    class texture_registry;
    class NEUTRINO_EXPORT gpu_sprites_atlas_id {
        friend class texture_registry;
        public:
            gpu_sprites_atlas_id() = default;
            gpu_sprites_atlas_id(const gpu_sprites_atlas_id&) = default;
            gpu_sprites_atlas_id& operator = (const gpu_sprites_atlas_id&) = default;
            gpu_sprites_atlas_id(gpu_sprites_atlas_id&&) = default;
            gpu_sprites_atlas_id& operator = (gpu_sprites_atlas_id&&) = default;

            std::strong_ordering operator <=> (const gpu_sprites_atlas_id& other) const = default;
        private:
            explicit gpu_sprites_atlas_id(uint32_t x)
                : m_value(x) {}
            uint32_t m_value{std::numeric_limits <uint32_t>::max()};
    };

    NEUTRINO_EXPORT gpu_sprites_atlas_id register_atlas(
        const cpu_sprites_atlas& atlas,
        atlas_texture_format = atlas_texture_format::automatic);
}
