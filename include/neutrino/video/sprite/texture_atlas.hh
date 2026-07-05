//
// Created by igor on 04/07/2026.
//

#pragma once


#include <functional>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/sprite/cpu_texture_atlas.hh>
#include <neutrino/video/sprite/detail/id_strong_type.hh>

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

    class texture_registry;
    namespace details {
        struct gpu_texture_atlas_id_tag;
    }
    /**
     * @brief Opaque handle for a renderer-resident texture atlas.
     *
     * The handle identifies an uploaded atlas stored by @ref texture_registry. Game
     * code should treat it as a value handle and should not infer backend texture
     * details from it.
     */
    class NEUTRINO_EXPORT gpu_texture_atlas_id
        : public details::id_strong_type <details::gpu_texture_atlas_id_tag> {
        friend class texture_registry;

        public:
            /**
             * @brief Construct an invalid atlas handle.
             */
            gpu_texture_atlas_id() = default;

        private:
            explicit gpu_texture_atlas_id(std::uint32_t value)
                : id_strong_type(value) {
            }
    };

    /**
     * @brief Upload a CPU texture atlas to renderer/GPU resources.
     *
     * The upload preserves the CPU atlas frame layout; it does not repack frames.
     *
     * @param atlas CPU atlas containing the source surface and frame metadata.
     * @param format Requested upload format.
     * @return Opaque handle to the uploaded atlas.
     */
    NEUTRINO_EXPORT gpu_texture_atlas_id register_atlas(
        const cpu_texture_atlas& atlas,
        atlas_texture_format format = atlas_texture_format::automatic);
}

/**
 * @brief @c std::hash specialization so @ref neutrino::gpu_texture_atlas_id keys
 *        @c unordered_map / @c unordered_set.
 */
template<>
struct std::hash <neutrino::gpu_texture_atlas_id> {
    [[nodiscard]] std::size_t operator()(const neutrino::gpu_texture_atlas_id& id) const noexcept {
        using base_type = neutrino::details::id_strong_type <neutrino::details::gpu_texture_atlas_id_tag>;
        return std::hash <base_type>{}(static_cast <const base_type&>(id));
    }
};
