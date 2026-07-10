//
// Created by igor on 05/07/2026.
//

#include <utility>

#include <failsafe/enforce.hh>
#include <neutrino/video/sprite/sprite_sheet.hh>

#include "services/service_locator.hh"
#include "video/sprite/sprites_manager.hh"

namespace neutrino {
    namespace {
        [[nodiscard]] std::uint64_t make_visual_owner() noexcept {
            static std::uint64_t next_owner = 0;
            const auto owner = next_owner;
            ++next_owner;
            return owner;
        }
    }

    std::uint64_t sprite_sheet::next_visual_owner() noexcept {
        return make_visual_owner();
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
        return id.valid() && id.m_owner == m_visual_owner && id.m_value < m_visuals.size();
    }

    sprite_visual_id sprite_sheet::visual_id(std::size_t index) const {
        ENFORCE(index < m_visuals.size());
        return sprite_visual_id(static_cast <std::uint32_t>(index), m_visual_owner);
    }

    sprite_visual_id sprite_sheet::add_visual(sprite_visual visual) {
        const sprite_visual_id id(static_cast <std::uint32_t>(m_visuals.size()), m_visual_owner);
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
        const auto it = m_names.find(name);
        if (it == m_names.end()) {
            return std::nullopt;
        }

        return it->second;
    }

    sprite_sheet_id register_sprite_sheet(sprite_sheet sheet) {
        auto* manager = service_locator::instance().get_sprites_manager();
        ENFORCE(manager != nullptr);
        return manager->create(std::move(sheet));
    }

    sprite_sheet_id register_sprite_sheet(
        const cpu_texture_atlas& atlas,
        atlas_texture_format format) {
        return register_sprite_sheet(sprite_sheet(register_atlas(atlas, format), atlas));
    }

    void unregister_sprite_sheet(sprite_sheet_id sheet) {
        if (!sheet.valid()) {
            return;
        }

        auto* manager = service_locator::instance().get_sprites_manager();
        if (manager == nullptr) {
            return; // services already torn down: the resource is gone, nothing to do
        }
        manager->erase(sheet);
    }

    sprite_visual_ref visual_ref(sprite_sheet_id sheet, std::size_t index) {
        auto* manager = service_locator::instance().get_sprites_manager();
        ENFORCE(manager != nullptr);

        const auto& registered = manager->get(sheet);
        return sprite_visual_ref{sheet, registered.visual_id(index)};
    }

    std::optional <sprite_visual_ref> find_visual_ref(sprite_sheet_id sheet, std::string_view name) {
        auto* manager = service_locator::instance().get_sprites_manager();
        ENFORCE(manager != nullptr);

        const auto& registered = manager->get(sheet);
        const auto visual = registered.find(name);
        if (!visual) {
            return std::nullopt;
        }

        return sprite_visual_ref{sheet, *visual};
    }
}
