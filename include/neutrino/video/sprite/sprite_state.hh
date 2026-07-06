//
// Created by igor on 05/07/2026.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/sprite/detail/id_strong_type.hh>
#include <neutrino/video/sprite/sprite_animation.hh>
#include <neutrino/video/sprite/sprite_appearance.hh>

namespace neutrino {
    namespace details {
        struct sprite_state_id_tag;
    }

    /**
     * @brief Opaque handle to a runtime sprite render state.
     *
     * A state is either a fixed @ref sprite_appearance or a playback cursor over a
     * registered @ref sprite_animation. It still has no world position; callers
     * provide position when drawing.
     */
    class NEUTRINO_EXPORT sprite_state_id
        : public details::id_strong_type <details::sprite_state_id_tag> {
        friend struct details::id_access;

        public:
            /**
             * @brief Construct an invalid sprite-state handle.
             */
            sprite_state_id() = default;

        private:
            explicit sprite_state_id(std::uint32_t value)
                : id_strong_type(value) {
            }
    };

    /**
     * @brief Create a runtime state that always resolves to @p appearance.
     *
     * @pre An application must be initialized.
     */
    NEUTRINO_EXPORT sprite_state_id create_sprite_state(sprite_appearance appearance);

    /**
     * @brief Create a runtime state that plays a registered animation from zero.
     *
     * The internal sprite manager advances this state from the application update
     * loop. Drawing the state resolves the active animation frame automatically.
     *
     * @pre An application must be initialized.
     * @pre @p animation must identify a registered animation definition.
     */
    NEUTRINO_EXPORT sprite_state_id create_sprite_state(sprite_animation_id animation);

    /**
     * @brief Replace a state with a fixed appearance and clear its animation clock.
     *
     * @pre @p state must identify a live sprite state.
     */
    NEUTRINO_EXPORT void set_sprite_state_appearance(sprite_state_id state, sprite_appearance appearance);

    /**
     * @brief Replace a state with animation playback starting at elapsed zero.
     *
     * Use this for one-shot actions that must restart even if the same animation is
     * already active, such as a fresh jump or attack.
     *
     * @pre @p state must identify a live sprite state.
     * @pre @p animation must identify a registered animation definition.
     */
    NEUTRINO_EXPORT void restart_sprite_animation(sprite_state_id state, sprite_animation_id animation);

    /**
     * @brief Switch a state to an animation only when it is not already active.
     *
     * If @p state is already playing @p animation, elapsed time is preserved and the
     * function returns false. Otherwise the state switches to @p animation, elapsed
     * time is reset to zero, and the function returns true.
     *
     * @pre @p state must identify a live sprite state.
     * @pre @p animation must identify a registered animation definition.
     */
    NEUTRINO_EXPORT bool switch_sprite_animation(sprite_state_id state, sprite_animation_id animation);

    /**
     * @brief Resolve the current appearance of a runtime state.
     *
     * This is the same resolution used by @ref draw_sprite(point, sprite_state_id).
     *
     * @pre @p state must identify a live sprite state.
     */
    NEUTRINO_EXPORT sprite_appearance sprite_state_appearance(sprite_state_id state);

    /**
     * @brief Has the state's current non-looping animation reached its end?
     *
     * Fixed appearances and looping animations return false. Replacing a state with
     * @ref restart_sprite_animation resets the completion state to false.
     *
     * @pre @p state must identify a live sprite state.
     */
    NEUTRINO_EXPORT bool sprite_state_finished(sprite_state_id state);

    /**
     * @brief Unregister a runtime sprite state.
     *
     * Invalid or already-erased handles are ignored. Removing a state releases its
     * references to fixed appearances and animation definitions.
     *
     * @pre An application must be initialized.
     */
    NEUTRINO_EXPORT void unregister_sprite_state(sprite_state_id state);
}

/**
 * @brief @c std::hash specialization so @ref neutrino::sprite_state_id keys
 *        @c unordered_map / @c unordered_set.
 */
template<>
struct std::hash <neutrino::sprite_state_id>
    : std::hash <neutrino::details::id_strong_type <neutrino::details::sprite_state_id_tag>> {
};
