//
// Created by igor on 04/07/2026.
//

#pragma once

#include <cstdint>
#include <vector>
#include <utility>
#include <unordered_map>

#include <neutrino/video/atlas/image_atlas.hh>
#include <neutrino/video/texture.hh>
#include <neutrino/video/sprite/texture_atlas.hh>



namespace neutrino {
    class gpu_texture_atlas {
        public:
            explicit gpu_texture_atlas(const sdlpp::renderer& renderer,
                                       image_atlas&& atlas,
                                       atlas_texture_format format);

            gpu_texture_atlas(const gpu_texture_atlas&) = delete;
            gpu_texture_atlas& operator =(const gpu_texture_atlas&) = delete;

            gpu_texture_atlas(gpu_texture_atlas&&) noexcept;
            gpu_texture_atlas& operator =(gpu_texture_atlas&&) noexcept;


            [[nodiscard]] std::size_t picture_count() const noexcept;
            [[nodiscard]] dim picture_size(std::size_t i) const;

            [[nodiscard]] std::pair<const texture&, rect> get(std::size_t i) const;
            [[nodiscard]] std::pair<texture&, rect> get(std::size_t i);

            [[nodiscard]] atlas_id id() const noexcept;
        private:
            atlas_id m_atlas;
            std::vector <texture> m_pages;
            std::vector <image_atlas::atlas_placement> m_rects;
    };

    /**
     * @brief Owns uploaded texture atlases for the active renderer.
     *
     * The registry is an internal service used by the public texture-atlas upload API
     * and by renderers that need to resolve @ref atlas_id handles.
     */
    class texture_registry {
        public:
            /**
             * @brief Upload a CPU texture atlas and copy its frame metadata.
             */
            atlas_id add(const sdlpp::renderer& renderer,
                     image_atlas&& atlas,
                     atlas_texture_format format = atlas_texture_format::automatic);


            [[nodiscard]] bool contains(atlas_id idx) const;
            [[nodiscard]] bool contains(atlas_id idx, std::size_t picture_idx) const;

            [[nodiscard]] std::size_t picture_count(atlas_id idx) const;

            [[nodiscard]] dim picture_size(atlas_id idx, std::size_t picture_idx) const;
            std::pair<texture&, rect> get(atlas_id idx, std::size_t picture_idx);
            [[nodiscard]] std::pair<const texture&, rect> get(atlas_id idx, std::size_t picture_idx) const;

            void erase(atlas_id idx);
        private:
            std::unordered_map<atlas_id, gpu_texture_atlas> m_atlas;
    };
}
