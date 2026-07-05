//
// Created by igor on 05/07/2026.
//

#pragma once

#include <cstdint>

#include <neutrino/video/sprite/sprite_animation.hh>
#include <neutrino/video/sprite/sprite_sheet.hh>
#include <neutrino/video/sprite/sprite_state.hh>

#include "resource_registry.hh"

namespace neutrino {
    namespace details {
        enum class sprite_state_mode {
            appearance,
            animation
        };

        struct sprite_state_record {
            sprite_appearance appearance;
            sprite_animation_id animation;
            sprite_animation_duration elapsed{0.0f};
            sprite_state_mode mode{sprite_state_mode::appearance};
        };
    }

    /**
     * @brief Internal owner of sprite resources and render states.
     *
     * The manager stores sprite sheets, animation definitions, and lightweight
     * playback states. It does not own world objects, positions, or physics state.
     */
    class sprites_manager {
        public:
            /**
             * @brief Store a sheet and return its opaque handle.
             */
            sprite_sheet_id create(sprite_sheet sheet);

            /**
             * @brief Resolve a mutable registered sheet.
             *
             * @pre @p id must identify a sheet stored in this manager.
             */
            sprite_sheet& get(sprite_sheet_id id);

            /**
             * @brief Resolve a const registered sheet.
             *
             * @pre @p id must identify a sheet stored in this manager.
             */
            const sprite_sheet& get(sprite_sheet_id id) const;

            /**
             * @brief Remove a registered sheet if present.
             */
            void erase(sprite_sheet_id id);

            /**
             * @brief Store an animation definition and return its opaque handle.
             */
            sprite_animation_id create(sprite_animation animation);

            /**
             * @brief Resolve a registered animation definition.
             *
             * @pre @p id must identify an animation stored in this manager.
             */
            const sprite_animation& get(sprite_animation_id id) const;

            /**
             * @brief Remove a registered animation if present.
             */
            void erase(sprite_animation_id id);

            /**
             * @brief Create a state that resolves to a fixed appearance.
             */
            sprite_state_id create(sprite_appearance appearance);

            /**
             * @brief Create a state that plays a registered animation from zero.
             *
             * @pre @p animation must identify an animation stored in this manager.
             */
            sprite_state_id create(sprite_animation_id animation);

            /**
             * @brief Replace a state with a fixed appearance.
             *
             * @pre @p id must identify a state stored in this manager.
             */
            void set_appearance(sprite_state_id id, sprite_appearance appearance);

            /**
             * @brief Replace a state with animation playback from zero.
             *
             * @pre @p id must identify a state stored in this manager.
             * @pre @p animation must identify an animation stored in this manager.
             */
            void set_animation(sprite_state_id id, sprite_animation_id animation);

            /**
             * @brief Resolve the appearance currently represented by a state.
             *
             * @pre @p id must identify a state stored in this manager.
             */
            [[nodiscard]] sprite_appearance appearance(sprite_state_id id) const;

            /**
             * @brief Advance every animated sprite state.
             */
            void update(sprite_animation_duration dt);

            /**
             * @brief Remove a state if present.
             */
            void erase(sprite_state_id id);

        private:
            static sprite_sheet_id make_sheet_id(std::uint32_t value);
            static sprite_animation_id make_animation_id(std::uint32_t value);
            static sprite_state_id make_state_id(std::uint32_t value);

            details::resource_registry <sprite_sheet_id, sprite_sheet> m_sheets;
            details::resource_registry <sprite_animation_id, sprite_animation> m_animations;
            details::resource_registry <sprite_state_id, details::sprite_state_record> m_states;
    };
}
