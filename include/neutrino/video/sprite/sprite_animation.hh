//
// Created by igor on 05/07/2026.
//

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/sprite/detail/id_strong_type.hh>
#include <neutrino/video/sprite/sprite_appearance.hh>

namespace neutrino {
    namespace details {
        struct sprite_animation_id_tag;
    }

    /**
     * @brief Opaque handle to a registered @ref sprite_animation definition.
     *
     * The handle identifies animation metadata stored by the internal sprite
     * manager. It is not a playback clock and carries no world position.
     */
    class NEUTRINO_EXPORT sprite_animation_id
        : public details::id_strong_type <details::sprite_animation_id_tag> {
        friend struct details::id_access;

        public:
            /**
             * @brief Construct an invalid animation handle.
             */
            sprite_animation_id() = default;

        private:
            explicit sprite_animation_id(std::uint32_t value)
                : id_strong_type(value) {
            }
    };

    /**
     * @brief Duration type used by sprite animation definitions.
     */
    using sprite_animation_duration = std::chrono::duration <float, std::milli>;

    /**
     * @brief One timed appearance in a sprite animation definition.
     *
     * This is still render metadata only. It carries no world position and owns no
     * animation clock.
     */
    struct sprite_animation_frame {
        /**
         * @brief Appearance shown while this frame is active.
         */
        sprite_appearance appearance;

        /**
         * @brief How long this frame remains active.
         */
        sprite_animation_duration duration{0.0f};
    };

    /**
     * @brief Static sprite animation definition.
     *
     * The animation maps elapsed time to a @ref sprite_appearance. It does not store
     * elapsed time, position, scene membership, or gameplay state.
     */
    class NEUTRINO_EXPORT sprite_animation {
        public:
            /**
             * @brief Construct an empty animation definition.
             */
            explicit sprite_animation(bool loop = true);

            /**
             * @brief Construct an animation from a complete frame list.
             *
             * @pre Every frame duration must be positive.
             */
            sprite_animation(std::vector <sprite_animation_frame> frames, bool loop = true);

            /**
             * @brief Should elapsed time wrap after the final frame?
             */
            [[nodiscard]] bool loop() const noexcept;

            /**
             * @brief Set whether elapsed time wraps after the final frame.
             */
            void set_loop(bool loop) noexcept;

            /**
             * @brief Number of timed frames in the animation.
             */
            [[nodiscard]] std::size_t frame_count() const noexcept;

            /**
             * @brief Is the animation frame list empty?
             */
            [[nodiscard]] bool empty() const noexcept;

            /**
             * @brief Sum of all frame durations.
             */
            [[nodiscard]] sprite_animation_duration total_duration() const noexcept;

            /**
             * @brief Get a frame by index.
             *
             * @pre @p index must be less than @ref frame_count.
             */
            [[nodiscard]] const sprite_animation_frame& frame(std::size_t index) const;

            /**
             * @brief Append a timed frame.
             *
             * @pre @p frame.duration must be positive.
             */
            void add_frame(sprite_animation_frame frame);

            /**
             * @brief Return the frame active at @p elapsed.
             *
             * Negative elapsed times are clamped to the first frame. Non-looping
             * animations clamp past-the-end elapsed times to the final frame.
             *
             * @pre The animation must contain at least one frame.
             */
            [[nodiscard]] const sprite_animation_frame& frame_at(sprite_animation_duration elapsed) const;

            /**
             * @brief Return the appearance active at @p elapsed.
             *
             * @pre The animation must contain at least one frame.
             */
            [[nodiscard]] sprite_appearance appearance_at(sprite_animation_duration elapsed) const;

        private:
            static void validate_frame(const sprite_animation_frame& frame);
            void rebuild_frame_index();

            std::vector <sprite_animation_frame> m_frames;
            /// Cumulative end time per frame, so frame_at is a binary search.
            std::vector <sprite_animation_duration> m_end_times;
            sprite_animation_duration m_total_duration{0.0f};
            bool m_loop{true};
    };

    /**
     * @brief Register a sprite animation definition with the active application.
     *
     * Registered definitions are immutable render metadata. Runtime playback time
     * lives in @ref sprite_state_id values created from the returned handle.
     *
     * @pre An application must be initialized.
     * @pre @p animation must contain at least one frame.
     */
    NEUTRINO_EXPORT sprite_animation_id register_sprite_animation(sprite_animation animation);

    /**
     * @brief Unregister a sprite animation definition.
     *
     * Invalid or already-erased handles are ignored. A registered animation cannot
     * be unregistered while any runtime sprite state is playing it.
     *
     * @pre An application must be initialized.
     */
    NEUTRINO_EXPORT void unregister_sprite_animation(sprite_animation_id animation);
}

/**
 * @brief @c std::hash specialization so @ref neutrino::sprite_animation_id keys
 *        @c unordered_map / @c unordered_set.
 */
template<>
struct std::hash <neutrino::sprite_animation_id>
    : std::hash <neutrino::details::id_strong_type <neutrino::details::sprite_animation_id_tag>> {
};
