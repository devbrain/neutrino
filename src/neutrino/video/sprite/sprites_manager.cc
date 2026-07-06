//
// Created by igor on 05/07/2026.
//

#include "sprites_manager.hh"

#include <failsafe/enforce.hh>

#include <cmath>
#include <utility>

#include "services/service_locator.hh"
#include "video/sprite/texture_registry.hh"

namespace neutrino {
    namespace {
        [[nodiscard]] bool uses_sheet(const sprite_appearance& appearance, sprite_sheet_id sheet) noexcept {
            return sheet.valid() && appearance.visual.sheet == sheet;
        }

        [[nodiscard]] bool uses_sheet(const sprite_animation& animation, sprite_sheet_id sheet) {
            if (!sheet.valid()) {
                return false;
            }

            for (std::size_t i = 0; i < animation.frame_count(); ++i) {
                if (uses_sheet(animation.frame(i).appearance, sheet)) {
                    return true;
                }
            }
            return false;
        }
    }

    sprite_sheet_id sprites_manager::make_sheet_id(std::uint32_t value) {
        return details::id_access::make <sprite_sheet_id>(value);
    }

    sprite_animation_id sprites_manager::make_animation_id(std::uint32_t value) {
        return details::id_access::make <sprite_animation_id>(value);
    }

    sprite_state_id sprites_manager::make_state_id(std::uint32_t value) {
        return details::id_access::make <sprite_state_id>(value);
    }

    void sprites_manager::validate_sheet(const sprite_sheet& sheet) const {
        auto* textures = service_locator::instance().get_texture_registry();
        ENFORCE(textures != nullptr);
        ENFORCE(sheet.atlas().valid())("Sprite sheet must reference a valid texture atlas");
        ENFORCE(textures->contains(sheet.atlas()))("Sprite sheet texture atlas is not registered");
    }

    void sprites_manager::validate_appearance(const sprite_appearance& appearance) const {
        if (!appearance.visual.valid()) {
            return;
        }

        ENFORCE(m_sheets.contains(appearance.visual.sheet))("Sprite visual sheet is not registered");
        ENFORCE(m_sheets.get(appearance.visual.sheet).contains(appearance.visual.visual))(
            "Sprite visual id is not part of the referenced sheet");
    }

    void sprites_manager::validate_animation(const sprite_animation& animation) const {
        ENFORCE(!animation.empty());
        for (std::size_t i = 0; i < animation.frame_count(); ++i) {
            validate_appearance(animation.frame(i).appearance);
        }
    }

    sprite_sheet_id sprites_manager::create(sprite_sheet sheet) {
        validate_sheet(sheet);
        return m_sheets.store(make_sheet_id, std::move(sheet));
    }

    sprite_sheet& sprites_manager::get(sprite_sheet_id id) {
        return m_sheets.get(id);
    }

    const sprite_sheet& sprites_manager::get(sprite_sheet_id id) const {
        return m_sheets.get(id);
    }

    void sprites_manager::erase(sprite_sheet_id id) {
        if (!id.valid() || !m_sheets.contains(id)) {
            return;
        }

        ENFORCE(!uses(id))("Cannot unregister sprite sheet while it is still used");
        m_sheets.erase(id);
    }

    bool sprites_manager::uses(gpu_texture_atlas_id id) const {
        return id.valid() && m_sheets.any_of([id](const sprite_sheet& sheet) {
            return sheet.atlas() == id;
        });
    }

    sprite_animation_id sprites_manager::create(sprite_animation animation) {
        validate_animation(animation);
        return m_animations.store(make_animation_id, std::move(animation));
    }

    const sprite_animation& sprites_manager::get(sprite_animation_id id) const {
        return m_animations.get(id);
    }

    void sprites_manager::erase(sprite_animation_id id) {
        if (!id.valid() || !m_animations.contains(id)) {
            return;
        }

        ENFORCE(!uses(id))("Cannot unregister sprite animation while it is still used");
        m_animations.erase(id);
    }

    bool sprites_manager::uses(sprite_animation_id id) const {
        return id.valid() && m_states.any_of([id](const details::sprite_state_record& state) {
            return state.animation == id;
        });
    }

    sprite_state_id sprites_manager::create(sprite_appearance appearance) {
        validate_appearance(appearance);

        details::sprite_state_record state;
        state.appearance = appearance;
        return m_states.store(make_state_id, state);
    }

    sprite_state_id sprites_manager::create(sprite_animation_id animation) {
        ENFORCE(!get(animation).empty());

        details::sprite_state_record state;
        state.animation = animation;
        return m_states.store(make_state_id, state);
    }

    void sprites_manager::set_appearance(sprite_state_id id, sprite_appearance appearance) {
        validate_appearance(appearance);

        auto& state = m_states.get(id);
        state.appearance = appearance;
        state.animation = sprite_animation_id{};
        state.elapsed = sprite_animation_duration::zero();
    }

    void sprites_manager::set_animation(sprite_state_id id, sprite_animation_id animation) {
        ENFORCE(!get(animation).empty());

        auto& state = m_states.get(id);
        state.appearance = sprite_appearance{};
        state.animation = animation;
        state.elapsed = sprite_animation_duration::zero();
    }

    bool sprites_manager::switch_animation(sprite_state_id id, sprite_animation_id animation) {
        ENFORCE(!get(animation).empty());

        if (m_states.get(id).animation == animation) {
            return false;
        }

        set_animation(id, animation);
        return true;
    }

    sprite_appearance sprites_manager::appearance(sprite_state_id id) const {
        const auto& state = m_states.get(id);
        if (!state.animation.valid()) {
            return state.appearance;
        }

        return get(state.animation).appearance_at(state.elapsed);
    }

    bool sprites_manager::finished(sprite_state_id id) const {
        const auto& state = m_states.get(id);
        if (!state.animation.valid()) {
            return false;
        }

        const auto& animation = get(state.animation);
        return !animation.loop() && state.elapsed >= animation.total_duration();
    }

    void sprites_manager::update(sprite_animation_duration dt) {
        if (dt <= sprite_animation_duration::zero()) {
            return;
        }

        m_states.for_each_resource([this, dt](details::sprite_state_record& state) {
            if (!state.animation.valid()) {
                return;
            }

            state.elapsed += dt;
            const auto& animation = get(state.animation);
            const auto total = animation.total_duration();
            if (total <= sprite_animation_duration::zero()) {
                state.elapsed = sprite_animation_duration::zero();
            } else if (animation.loop()) {
                state.elapsed = sprite_animation_duration{std::fmod(state.elapsed.count(), total.count())};
            } else if (state.elapsed > total) {
                state.elapsed = total;
            }
        });
    }

    void sprites_manager::erase(sprite_state_id id) {
        m_states.erase(id);
    }

    bool sprites_manager::uses(sprite_sheet_id id) const {
        if (!id.valid()) {
            return false;
        }

        return m_animations.any_of([id](const sprite_animation& animation) {
                return uses_sheet(animation, id);
            })
            || m_states.any_of([id](const details::sprite_state_record& state) {
                return !state.animation.valid() && uses_sheet(state.appearance, id);
            });
    }
}
