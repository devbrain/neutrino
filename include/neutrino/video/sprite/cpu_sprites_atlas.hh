//
// Created by igor on 04/07/2026.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
#include <sdlpp/video/surface.hh>
#include <neutrino/neutrino_export.h>

#include <neutrino/video/sprite/bitmask.hh>
#include <neutrino/video/geometry_types.hh>

namespace neutrino {
    struct cpu_sprite_frame_mask_options {
        std::optional <sdlpp::color> key_color;
        std::optional <uint8_t> key_color_index;
    };

    struct cpu_sprite_frame {
        explicit cpu_sprite_frame(const rect& atlas_rect)
            : atlas_rect(atlas_rect) {
        }

        cpu_sprite_frame(const rect& atlas_rect, const bitmask& mask)
            : atlas_rect(atlas_rect),
              mask(mask) {
        }

        rect atlas_rect;
        std::optional <bitmask> mask;

        static std::optional <bitmask> evaluate_bitmask(
            const sdlpp::surface& s,
            const rect& atlas_rect,
            const cpu_sprite_frame_mask_options& options = {});
    };



    class NEUTRINO_EXPORT cpu_sprites_atlas {
        public:
            explicit cpu_sprites_atlas(sdlpp::surface&& s);

            explicit cpu_sprites_atlas(sdlpp::surface&& s,
                                       std::vector <cpu_sprite_frame>&& frames);

            [[nodiscard]] const sdlpp::surface& surface() const noexcept;
            [[nodiscard]] std::size_t frame_count() const noexcept;
            [[nodiscard]] const cpu_sprite_frame& frame(std::size_t index) const;
            void add_frame (const cpu_sprite_frame& f);

        private:
            sdlpp::surface m_surface;
            std::vector <cpu_sprite_frame> m_frames;
    };


}