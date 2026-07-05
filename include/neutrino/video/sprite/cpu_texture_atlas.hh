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
    /**
     * @brief Options used to derive a CPU collision/picking bitmask from atlas pixels.
     *
     * If @ref key_color_index is set, the source surface must be INDEX8 and matching
     * palette indices are considered transparent. Otherwise @ref key_color is compared
     * against source RGB values. If no key is supplied and the surface has an alpha
     * channel, zero-alpha pixels are transparent. Pixels with no transparency signal are
     * considered solid.
     */
    struct cpu_texture_atlas_mask_options {
        /**
         * @brief RGB key color treated as transparent when evaluating a bitmask.
         *
         * Alpha is ignored for the key comparison.
         */
        std::optional <sdlpp::color> key_color;

        /**
         * @brief INDEX8 palette index treated as transparent when evaluating a bitmask.
         */
        std::optional <uint8_t> key_color_index;
    };

    /**
     * @brief Metadata for one frame/region inside a CPU-resident texture atlas.
     *
     * The frame does not own pixels. @ref atlas_rect identifies a rectangle inside the
     * owning @ref cpu_texture_atlas surface. The optional @ref mask is CPU-only metadata
     * for per-pixel picking or collision.
     */
    struct cpu_texture_atlas_frame {
        /**
         * @brief Construct a frame that references a rectangle in the atlas surface.
         */
        explicit cpu_texture_atlas_frame(const rect& atlas_rect)
            : atlas_rect(atlas_rect) {
        }

        /**
         * @brief Construct a frame with an already-evaluated CPU bitmask.
         */
        cpu_texture_atlas_frame(const rect& atlas_rect, const bitmask& mask)
            : atlas_rect(atlas_rect),
              mask(mask) {
        }

        /**
         * @brief Source rectangle of this frame in atlas-surface coordinates.
         */
        rect atlas_rect;

        /**
         * @brief Optional frame-local solid/transparent bitmask.
         */
        std::optional <bitmask> mask;

        /**
         * @brief Evaluate a frame-local bitmask from a surface rectangle.
         *
         * @param s CPU surface containing the atlas pixels.
         * @param atlas_rect Source rectangle in @p s to evaluate.
         * @param options Transparency policy used while evaluating the mask.
         * @return A bitmask with dimensions matching @p atlas_rect, or std::nullopt
         *         when the surface/rectangle/options combination is invalid.
         */
        static std::optional <bitmask> evaluate_bitmask(
            const sdlpp::surface& s,
            const rect& atlas_rect,
            const cpu_texture_atlas_mask_options& options = {});
    };


    /**
     * @brief CPU-resident texture atlas: one loaded surface plus frame metadata.
     *
     * This is the asset-side representation used before uploading to renderer/GPU
     * resources. It is neutral enough to back sprites, bitmap fonts, tiles, and
     * backdrops. It owns CPU pixels but no GPU texture.
     */
    class NEUTRINO_EXPORT cpu_texture_atlas {
        public:
            /**
             * @brief Construct an atlas with no frame metadata.
             */
            explicit cpu_texture_atlas(sdlpp::surface&& s);

            /**
             * @brief Construct an atlas with precomputed frame metadata.
             */
            explicit cpu_texture_atlas(sdlpp::surface&& s,
                                       std::vector <cpu_texture_atlas_frame>&& frames);

            /**
             * @brief Access the owned CPU surface.
             */
            [[nodiscard]] const sdlpp::surface& surface() const noexcept;

            /**
             * @brief Number of frame rectangles stored in this atlas.
             */
            [[nodiscard]] std::size_t frame_count() const noexcept;

            /**
             * @brief Get frame metadata by zero-based index.
             *
             * @pre @p index must be less than @ref frame_count.
             */
            [[nodiscard]] const cpu_texture_atlas_frame& frame(std::size_t index) const;

            /**
             * @brief Append frame metadata to the atlas.
             */
            void add_frame (const cpu_texture_atlas_frame& f);

        private:
            sdlpp::surface m_surface;
            std::vector <cpu_texture_atlas_frame> m_frames;
    };


}
