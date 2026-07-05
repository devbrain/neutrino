//
// Created by igor on 04/07/2026.
//

#pragma once

#include <map>
#include <vector>

#include <sdlpp/video/texture.hh>

#include <neutrino/video/sprite/texture_atlas.hh>

namespace neutrino {
    /**
     * @brief Renderer-resident texture atlas record.
     *
     * This stores the backend texture together with the copied frame rectangles from
     * the CPU atlas. Higher-level systems resolve frame indices through @ref frames
     * and use @ref texture for actual renderer submission.
     */
    struct gpu_texture_atlas {
        /**
         * @brief Uploaded renderer texture.
         */
        sdlpp::texture texture;

        /**
         * @brief Source rectangles in texture coordinates, copied from the CPU atlas.
         */
        std::vector <rect> frames;

        /**
         * @brief Format requested when the atlas was registered.
         */
        atlas_texture_format format{atlas_texture_format::automatic};
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
             * @brief Remove an uploaded atlas if present.
             */
            void erase(gpu_texture_atlas_id idx);
        private:
            std::map <gpu_texture_atlas_id, gpu_texture_atlas> m_atlases;
            static uint32_t s_counter;
    };
}
