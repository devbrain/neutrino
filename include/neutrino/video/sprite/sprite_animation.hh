//
// Created by igor on 05/07/2026.
//

#pragma once

#include <chrono>
#include <cstddef>
#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/sprite/sprite_appearance.hh>

namespace neutrino {
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
            void validate_frame(const sprite_animation_frame& frame) const;
            void rebuild_total_duration() noexcept;

            std::vector <sprite_animation_frame> m_frames;
            sprite_animation_duration m_total_duration{0.0f};
            bool m_loop{true};
    };
}
