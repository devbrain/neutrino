//
// Created by igor on 05/07/2026.
//

#include "sprites_manager.hh"

#include <failsafe/enforce.hh>

#include <utility>

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
        return sprite_sheet_id(value);
    }

    sprite_animation_id sprites_manager::make_animation_id(std::uint32_t value) {
        return sprite_animation_id(value);
    }

    sprite_state_id sprites_manager::make_state_id(std::uint32_t value) {
        return sprite_state_id(value);
    }

    sprite_sheet_id sprites_manager::create(sprite_sheet sheet) {
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
        if (!id.valid()) {
            return false;
        }

        bool found = false;
        m_sheets.for_each_resource([&found, id](const sprite_sheet& sheet) {
            if (sheet.atlas() == id) {
                found = true;
            }
        });
        return found;
    }

    sprite_animation_id sprites_manager::create(sprite_animation animation) {
        ENFORCE(!animation.empty());
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
        if (!id.valid()) {
            return false;
        }

        bool found = false;
        m_states.for_each_resource([&found, id](const details::sprite_state_record& state) {
            if (state.mode == details::sprite_state_mode::animation && state.animation == id) {
                found = true;
            }
        });
        return found;
    }

    sprite_state_id sprites_manager::create(sprite_appearance appearance) {
        details::sprite_state_record state;
        state.appearance = appearance;
        state.mode = details::sprite_state_mode::appearance;
        return m_states.store(make_state_id, state);
    }

    sprite_state_id sprites_manager::create(sprite_animation_id animation) {
        ENFORCE(!get(animation).empty());

        details::sprite_state_record state;
        state.animation = animation;
        state.mode = details::sprite_state_mode::animation;
        return m_states.store(make_state_id, state);
    }

    void sprites_manager::set_appearance(sprite_state_id id, sprite_appearance appearance) {
        auto& state = m_states.get(id);
        state.appearance = appearance;
        state.animation = sprite_animation_id{};
        state.elapsed = sprite_animation_duration::zero();
        state.mode = details::sprite_state_mode::appearance;
    }

    void sprites_manager::set_animation(sprite_state_id id, sprite_animation_id animation) {
        ENFORCE(!get(animation).empty());

        auto& state = m_states.get(id);
        state.appearance = sprite_appearance{};
        state.animation = animation;
        state.elapsed = sprite_animation_duration::zero();
        state.mode = details::sprite_state_mode::animation;
    }

    sprite_appearance sprites_manager::appearance(sprite_state_id id) const {
        const auto& state = m_states.get(id);
        if (state.mode == details::sprite_state_mode::appearance) {
            return state.appearance;
        }

        return get(state.animation).appearance_at(state.elapsed);
    }

    bool sprites_manager::finished(sprite_state_id id) const {
        const auto& state = m_states.get(id);
        if (state.mode == details::sprite_state_mode::appearance) {
            return false;
        }

        const auto& animation = get(state.animation);
        return !animation.loop() && state.elapsed >= animation.total_duration();
    }

    void sprites_manager::update(sprite_animation_duration dt) {
        if (dt <= sprite_animation_duration::zero()) {
            return;
        }

        m_states.for_each_resource([dt](details::sprite_state_record& state) {
            if (state.mode == details::sprite_state_mode::animation) {
                state.elapsed += dt;
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

        bool found = false;
        m_animations.for_each_resource([&found, id](const sprite_animation& animation) {
            if (uses_sheet(animation, id)) {
                found = true;
            }
        });

        if (found) {
            return true;
        }

        m_states.for_each_resource([&found, id](const details::sprite_state_record& state) {
            if (state.mode == details::sprite_state_mode::appearance && uses_sheet(state.appearance, id)) {
                found = true;
            }
        });
        return found;
    }
}
