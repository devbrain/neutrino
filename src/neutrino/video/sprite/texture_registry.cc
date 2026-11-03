//
// Created by igor on 04/07/2026.
//

#include "texture_registry.hh"
#include "video/atlas/atlas_id_gen.hh"

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
    }

    gpu_texture_atlas::gpu_texture_atlas(const sdlpp::renderer& renderer,
                                         image_atlas&& atlas,
                                         atlas_texture_format format)
        : m_atlas(atlas.id()),
          m_rects(std::move(atlas.m_rects)) {
        m_pages.reserve(atlas.m_pages.size());

        for (auto & page : atlas.m_pages) {
            sdlpp::expected <sdlpp::texture, std::string> tex;
            if (format == atlas_texture_format::automatic) {
                tex = sdlpp::texture::create(renderer, page);
            } else {
                const auto requested_format = to_sdl_format(format, page);
                auto converted = page.format() == requested_format
                                     ? page.duplicate()
                                     : page.convert(requested_format);
                if (!converted) {
                    THROW_RUNTIME(converted.error());
                }
                tex = sdlpp::texture::create(renderer, *converted);
            }
            if (tex) {
                m_pages.emplace_back(texture::adopt(std::move(*tex), texture_access::fixed));
            } else {
                THROW_RUNTIME(tex.error());
            }
        }
    }

    // ================================================================================================================
    gpu_texture_atlas::gpu_texture_atlas(gpu_texture_atlas&&) noexcept = default;
    gpu_texture_atlas& gpu_texture_atlas::operator=(gpu_texture_atlas&&) noexcept = default;

    std::size_t gpu_texture_atlas::picture_count() const noexcept {
        return m_rects.size();
    }

    dim gpu_texture_atlas::picture_size(std::size_t i) const {
        ENFORCE(i < m_rects.size(), "Picture index", i, "is out of range. Number of pictures for atlas", m_atlas,
                "is", m_rects.size());
        return m_rects[i].picture.dimensions();
    }

    std::pair<const texture&, rect> gpu_texture_atlas::get(std::size_t i) const {
        ENFORCE(i < m_rects.size(), "Picture index", i, "is out of range. Number of pictures for atlas", m_atlas,
                        "is", m_rects.size());
        const auto& placement = m_rects[i];
        ENFORCE(placement.page < m_pages.size(), "GPU atlas is corrupted");
        return {m_pages[placement.page], placement.picture};
    }

    std::pair<texture&, rect> gpu_texture_atlas::get(std::size_t i) {
        ENFORCE(i < m_rects.size(), "Picture index", i, "is out of range. Number of pictures for atlas", m_atlas,
                        "is", m_rects.size());
        const auto& placement = m_rects[i];
        ENFORCE(placement.page < m_pages.size(), "GPU atlas is corrupted");
        return {m_pages[placement.page], placement.picture};
    }

    atlas_id gpu_texture_atlas::id() const noexcept {
        return m_atlas;
    }

    atlas_id texture_registry::add(const sdlpp::renderer& renderer, image_atlas&& atlas, atlas_texture_format format) {
        ENFORCE (!m_atlas.contains(atlas.id()), "Atlas with id", atlas.id(), "is already registered");
        auto [itr, _] = m_atlas.emplace(atlas.id(), gpu_texture_atlas(renderer, std::move(atlas), format));
        return itr->second.id();
    }

    bool texture_registry::contains(atlas_id idx) const {
        auto itr = m_atlas.find(idx);
        return itr != m_atlas.end();
    }

    bool texture_registry::contains(atlas_id idx, std::size_t picture_idx) const {
        auto itr = m_atlas.find(idx);
        if (itr == m_atlas.end()) {
            return false;
        }
        return (picture_idx < itr->second.picture_count());
    }

    std::size_t texture_registry::picture_count(atlas_id idx) const {
        auto itr = m_atlas.find(idx);
        ENFORCE(itr != m_atlas.end(), "Atlas with id", idx, "does not exist");
        return itr->second.picture_count();
    }

    dim texture_registry::picture_size(atlas_id idx, std::size_t picture_idx) const {
        auto itr = m_atlas.find(idx);
        ENFORCE(itr != m_atlas.end(), "Atlas with id", idx, "does not exist");
        return itr->second.picture_size(picture_idx);
    }

    std::pair<texture&, rect> texture_registry::get(atlas_id idx, std::size_t picture_idx) {
        auto itr = m_atlas.find(idx);
        ENFORCE(itr != m_atlas.end(), "Atlas with id", idx, "does not exist");
        return itr->second.get(picture_idx);
    }

    std::pair<const texture&, rect> texture_registry::get(atlas_id idx, std::size_t picture_idx) const {
        auto itr = m_atlas.find(idx);
        ENFORCE(itr != m_atlas.end(), "Atlas with id", idx, "does not exist");
        return itr->second.get(picture_idx);
    }


    void texture_registry::erase(atlas_id idx) {
        auto itr = m_atlas.find(idx);
        if (itr != m_atlas.end()) {
            m_atlas.erase(itr);
        }
    }
}
