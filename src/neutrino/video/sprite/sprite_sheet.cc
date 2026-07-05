//
// Created by igor on 05/07/2026.
//

#include <utility>

#include <failsafe/enforce.hh>
#include <neutrino/video/sprite/sprite_sheet.hh>

namespace neutrino {
    bool sprite_visual_id::valid() const noexcept {
        return m_value != invalid_value;
    }

    sprite_sheet::sprite_sheet(gpu_texture_atlas_id atlas)
        : m_atlas(atlas) {
    }

    sprite_sheet::sprite_sheet(gpu_texture_atlas_id atlas, const cpu_texture_atlas& source)
        : sprite_sheet(atlas) {
        m_visuals.reserve(source.frame_count());
        for (std::size_t i = 0; i < source.frame_count(); ++i) {
            add_visual(sprite_visual{.texture_rect = source.frame(i).atlas_rect});
        }
    }

    gpu_texture_atlas_id sprite_sheet::atlas() const noexcept {
        return m_atlas;
    }

    std::size_t sprite_sheet::visual_count() const noexcept {
        return m_visuals.size();
    }

    bool sprite_sheet::contains(sprite_visual_id id) const noexcept {
        return id.m_value < m_visuals.size();
    }

    sprite_visual_id sprite_sheet::visual_id(std::size_t index) const {
        ENFORCE(index < m_visuals.size());
        return sprite_visual_id(index);
    }

    sprite_visual_id sprite_sheet::add_visual(sprite_visual visual) {
        const sprite_visual_id id(m_visuals.size());
        m_visuals.push_back(std::move(visual));
        return id;
    }

    sprite_visual_id sprite_sheet::add_visual(std::string name, sprite_visual visual) {
        const auto id = add_visual(std::move(visual));
        bind(std::move(name), id);
        return id;
    }

    const sprite_visual& sprite_sheet::visual(sprite_visual_id id) const {
        ENFORCE(contains(id));
        return m_visuals[id.m_value];
    }

    void sprite_sheet::bind(std::string name, sprite_visual_id id) {
        ENFORCE(contains(id));
        m_names[std::move(name)] = id;
    }

    std::optional <sprite_visual_id> sprite_sheet::find(std::string_view name) const {
        const auto it = m_names.find(std::string(name));
        if (it == m_names.end()) {
            return std::nullopt;
        }

        return it->second;
    }

    sprite_sheet register_sprite_sheet(
        const cpu_texture_atlas& atlas,
        atlas_texture_format format) {
        return sprite_sheet(register_atlas(atlas, format), atlas);
    }
}
