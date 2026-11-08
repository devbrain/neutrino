//
// Created by igor on 04/07/2026.
//

#pragma once

#include <functional>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/atlas/image_atlas.hh>
#include <neutrino/video/geometry_types.hh>

#include <neutrino/video/texture.hh>
#include <neutrino/video/text/glyph_atlas.hh>

namespace neutrino {
    /**
     * @brief Requested renderer texture format for an uploaded texture atlas.
     *
     * @ref automatic preserves the source surface format where the backend allows it.
     * Explicit formats express user intent for opaque RGB art, alpha textures, or
     * indexed/palette-driven rendering.
     */
    enum class atlas_texture_format {
        /**
         * @brief Let the upload path/backend choose from the source surface format.
         */
        automatic,

        /**
         * @brief Opaque RGB texture, useful for tiles, backgrounds, and backdrops.
         */
        rgb,

        /**
         * @brief RGBA texture, useful for sprites, glyphs, and alpha assets.
         */
        rgba,

        /**
         * @brief 8-bit indexed texture, useful for palette-based classic rendering.
         */
        indexed8
    };

    /**
     * @brief Upload a CPU texture atlas to renderer/GPU resources.
     *
     * The upload consumes the CPU atlas: its pages become textures and its picture
     * placements move to the registry unchanged (nothing is repacked), so addresses
     * taken from the atlas before the upload stay valid.
     *
     * @param atlas CPU atlas to upload; moved from.
     * @param format Requested upload format.
     * @param filter How every page is sampled when a picture is drawn at a different size.
     *        Defaults to linear, unlike standalone textures (nearest): atlas pages have always
     *        been linear-filtered, so existing atlases keep their look. Pass
     *        texture_filter::nearest for pixel-exact art such as pixel fonts.
     * @return The atlas's id, the same one @ref image_atlas::id returned.
     */
    NEUTRINO_EXPORT atlas_id register_atlas(
        image_atlas&& atlas,
        atlas_texture_format format = atlas_texture_format::automatic,
        texture_filter filter = texture_filter::linear);

    NEUTRINO_EXPORT atlas_id register_atlas(
        glyph_atlas&& atlas,
        atlas_texture_format format = atlas_texture_format::automatic,
        texture_filter filter = texture_filter::linear);
    /**
     * @brief Unregister an uploaded texture atlas.
     *
     * Invalid or already-erased handles are ignored. A registered atlas cannot be
     * unregistered while any registered @ref sprite_sheet still uses it.
     *
     * @pre An application must be initialized.
     */
    NEUTRINO_EXPORT void unregister_atlas(atlas_id atlas);

    /**
     * @brief Size of picture @p picture_idx of an uploaded atlas.
     *
     * Before the upload, use @ref image_atlas::picture_size.
     *
     * @throws std::runtime_error when @p atlas is not registered or @p picture_idx is out of range.
     */
    [[nodiscard]] NEUTRINO_EXPORT dim picture_size(atlas_id atlas, std::size_t picture_idx);
}
