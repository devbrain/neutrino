//
// Created by igor on 04/07/2026.
//

#include "texture_registry.hh"
#include <failsafe/enforce.hh>

#include <utility>

namespace neutrino {
    namespace {
        sdlpp::pixel_format_enum to_sdl_format(atlas_texture_format format, const sdlpp::surface& surface) {
            switch (format) {
                case atlas_texture_format::automatic:
                    return surface.format();
                case atlas_texture_format::rgb:
                    return sdlpp::pixel_format_enum::RGB24;
                case atlas_texture_format::rgba:
                    return sdlpp::pixel_format_enum::RGBA8888;
                case atlas_texture_format::indexed8:
                    return sdlpp::pixel_format_enum::INDEX8;
            }
            return surface.format();
        }

        std::vector <rect> copy_frame_rects(const cpu_sprites_atlas& atlas) {
            std::vector <rect> frames;
            frames.reserve(atlas.frame_count());
            for (std::size_t i = 0; i < atlas.frame_count(); ++i) {
                frames.push_back(atlas.frame(i).atlas_rect);
            }
            return frames;
        }

    }

    uint32_t texture_registry::s_counter = 0;

    gpu_sprites_atlas_id texture_registry::create(const sdlpp::renderer& renderer, sdlpp::pixel_format_enum format,
        sdlpp::texture_access access, int width, int height) {
        gpu_sprites_atlas_id key(s_counter);
        auto tex = sdlpp::texture::create(renderer, format, access, width, height);
        if (tex) {
            m_atlases.emplace(key, gpu_sprites_atlas{std::move(*tex), {}, atlas_texture_format::automatic});
            ++s_counter;
            return key;
        }
        THROW_RUNTIME(tex.error());
    }

    gpu_sprites_atlas_id texture_registry::create(const sdlpp::renderer& renderer, const sdlpp::surface& s) {
        gpu_sprites_atlas_id key(s_counter);
        auto tex = sdlpp::texture::create(renderer, s);
        if (tex) {
            m_atlases.emplace(key, gpu_sprites_atlas{std::move(*tex), {}, atlas_texture_format::automatic});
            ++s_counter;
            return key;
        }
        THROW_RUNTIME(tex.error());
    }

    gpu_sprites_atlas_id texture_registry::create(
        const sdlpp::renderer& renderer,
        const cpu_sprites_atlas& atlas,
        atlas_texture_format format) {
        gpu_sprites_atlas_id key(s_counter);
        auto frames = copy_frame_rects(atlas);

        sdlpp::expected <sdlpp::texture, std::string> tex;
        if (format == atlas_texture_format::automatic) {
            tex = sdlpp::texture::create(renderer, atlas.surface());
        } else {
            const auto requested_format = to_sdl_format(format, atlas.surface());
            auto converted = atlas.surface().format() == requested_format
                                 ? atlas.surface().duplicate()
                                 : atlas.surface().convert(requested_format);
            if (!converted) {
                THROW_RUNTIME(converted.error());
            }
            tex = sdlpp::texture::create(renderer, *converted);
        }

        if (tex) {
            m_atlases.emplace(key, gpu_sprites_atlas{std::move(*tex), std::move(frames), format});
            ++s_counter;
            return key;
        }
        THROW_RUNTIME(tex.error());
    }

    gpu_sprites_atlas& texture_registry::get(gpu_sprites_atlas_id idx) {
        auto itr = m_atlases.find(idx);
        ENFORCE(itr != m_atlases.end());
        return itr->second;
    }

    const gpu_sprites_atlas& texture_registry::get(gpu_sprites_atlas_id idx) const {
        auto itr = m_atlases.find(idx);
        ENFORCE(itr != m_atlases.end());
        return itr->second;
    }

    void texture_registry::erase(gpu_sprites_atlas_id idx) {
        auto itr = m_atlases.find(idx);
        if (itr != m_atlases.end()) {
            m_atlases.erase(itr);
        }
    }
}
