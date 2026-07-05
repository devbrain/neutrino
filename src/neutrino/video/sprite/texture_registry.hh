//
// Created by igor on 04/07/2026.
//

#pragma once

#include <map>
#include <vector>

#include <sdlpp/video/texture.hh>

#include <neutrino/video/sprite/sprites_atlas.hh>

namespace neutrino {
    struct gpu_sprites_atlas {
        sdlpp::texture texture;
        std::vector <rect> frames;
        atlas_texture_format format{atlas_texture_format::automatic};
    };

    class texture_registry {
        public:
            gpu_sprites_atlas_id create(const sdlpp::renderer& renderer,
                                        sdlpp::pixel_format_enum format,
                                        sdlpp::texture_access access,
                                        int width, int height);
            gpu_sprites_atlas_id create(const sdlpp::renderer& renderer,
                                        const sdlpp::surface& s);
            gpu_sprites_atlas_id create(const sdlpp::renderer& renderer,
                                        const cpu_sprites_atlas& atlas,
                                        atlas_texture_format format = atlas_texture_format::automatic);

            gpu_sprites_atlas& get(gpu_sprites_atlas_id idx);
            const gpu_sprites_atlas& get(gpu_sprites_atlas_id idx) const;

            void erase(gpu_sprites_atlas_id idx);
        private:
            std::map <gpu_sprites_atlas_id, gpu_sprites_atlas> m_atlases;
            static uint32_t s_counter;
    };
}
