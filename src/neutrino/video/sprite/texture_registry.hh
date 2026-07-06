//
// Created by igor on 04/07/2026.
//

#pragma once

#include <cstdint>

#include <sdlpp/video/texture.hh>

#include <neutrino/video/sprite/texture_atlas.hh>

#include "resource_registry.hh"

namespace neutrino {
    /**
     * @brief Renderer-resident texture atlas record.
     *
     * This stores the backend texture for renderer submission. Frame rectangles
     * live in the sprite sheet's visuals; the texture dimensions are cached here
     * so draw paths never query the backend per draw.
     */
    struct gpu_texture_atlas {
        /**
         * @brief Uploaded renderer texture.
         */
        sdlpp::texture texture;

        /**
         * @brief Format requested when the atlas was registered.
         */
        atlas_texture_format format{atlas_texture_format::automatic};

        /**
         * @brief Texture width in pixels, cached at upload time.
         */
        int width{0};

        /**
         * @brief Texture height in pixels, cached at upload time.
         */
        int height{0};
    };

    /**
     * @brief Owns uploaded texture atlases for the active renderer.
     *
     * The registry is an internal service used by the public texture-atlas upload API
     * and by renderers that need to resolve @ref gpu_texture_atlas_id handles.
     */
    class texture_registry {
        public:
            /**
             * @brief Create an empty texture record without frame metadata.
             */
            gpu_texture_atlas_id create(const sdlpp::renderer& renderer,
                                        sdlpp::pixel_format_enum format,
                                        sdlpp::texture_access access,
                                        int width, int height);

            /**
             * @brief Create a texture record from a surface without frame metadata.
             */
            gpu_texture_atlas_id create(const sdlpp::renderer& renderer,
                                        const sdlpp::surface& s);

            /**
             * @brief Upload a CPU texture atlas and copy its frame metadata.
             */
            gpu_texture_atlas_id create(const sdlpp::renderer& renderer,
                                        const cpu_texture_atlas& atlas,
                                        atlas_texture_format format = atlas_texture_format::automatic);

            /**
             * @brief Resolve a mutable uploaded atlas record.
             *
             * @pre @p idx must identify an atlas stored in this registry.
             */
            gpu_texture_atlas& get(gpu_texture_atlas_id idx);

            /**
             * @brief Resolve a const uploaded atlas record.
             *
             * @pre @p idx must identify an atlas stored in this registry.
             */
            const gpu_texture_atlas& get(gpu_texture_atlas_id idx) const;

            /**
             * @brief Does this registry currently store @p idx?
             */
            [[nodiscard]] bool contains(gpu_texture_atlas_id idx) const;

            /**
             * @brief Remove an uploaded atlas if present.
             */
            void erase(gpu_texture_atlas_id idx);
        private:
            static gpu_texture_atlas_id make_id(std::uint32_t value);

            details::resource_registry <gpu_texture_atlas_id, gpu_texture_atlas> m_atlases;
    };
}
