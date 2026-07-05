//
// Created by igor on 04/07/2026.
//

#include "texture_registry.hh"

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

        std::vector <rect> copy_frame_rects(const cpu_texture_atlas& atlas) {
            std::vector <rect> frames;
            frames.reserve(atlas.frame_count());
            for (std::size_t i = 0; i < atlas.frame_count(); ++i) {
                frames.push_back(atlas.frame(i).atlas_rect);
            }
            return frames;
        }

    }

    gpu_texture_atlas_id texture_registry::make_id(std::uint32_t value) {
        return gpu_texture_atlas_id(value);
    }

    gpu_texture_atlas_id texture_registry::create(const sdlpp::renderer& renderer, sdlpp::pixel_format_enum format,
        sdlpp::texture_access access, int width, int height) {
        auto tex = sdlpp::texture::create(renderer, format, access, width, height);
        if (tex) {
            return m_atlases.store(make_id, gpu_texture_atlas{std::move(*tex), {}, atlas_texture_format::automatic});
        }
        THROW_RUNTIME(tex.error());
    }

    gpu_texture_atlas_id texture_registry::create(const sdlpp::renderer& renderer, const sdlpp::surface& s) {
        auto tex = sdlpp::texture::create(renderer, s);
        if (tex) {
            return m_atlases.store(make_id, gpu_texture_atlas{std::move(*tex), {}, atlas_texture_format::automatic});
        }
        THROW_RUNTIME(tex.error());
    }

    gpu_texture_atlas_id texture_registry::create(
        const sdlpp::renderer& renderer,
        const cpu_texture_atlas& atlas,
        atlas_texture_format format) {
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
            return m_atlases.store(make_id, gpu_texture_atlas{std::move(*tex), std::move(frames), format});
        }
        THROW_RUNTIME(tex.error());
    }

    gpu_texture_atlas& texture_registry::get(gpu_texture_atlas_id idx) {
        return m_atlases.get(idx);
    }

    const gpu_texture_atlas& texture_registry::get(gpu_texture_atlas_id idx) const {
        return m_atlases.get(idx);
    }

    void texture_registry::erase(gpu_texture_atlas_id idx) {
        m_atlases.erase(idx);
    }
}
