/// @file scenes_manager.hh
/// @brief Stack-based scene manager that drives the scene lifecycle and frame dispatch.

#pragma once

#include <memory>
#include <vector>

#include <neutrino/scene/base_scene.hh>

namespace neutrino {
    /// @brief Manages a stack of base_scene instances, dispatching lifecycle and frame events.
    /// Only the topmost scene receives input; rendering walks down to the first opaque scene.
    class scenes_manager {
        public:
            scenes_manager();
            ~scenes_manager();

            /// @brief Push a new scene on top, pausing the current one.
            static void push_scene(std::unique_ptr <base_scene>&& scene);
            /// @brief Pop the topmost scene, resuming the one below.
            static void pop_scene();
            /// @brief Replace the topmost scene (pop + push as a single operation).
            static void replace_scene(std::unique_ptr <base_scene>&& scene);

            /// @brief Push a scene synchronously, bypassing the event queue.
            /// Only safe while no scene callbacks are running; used by
            /// application to install the initial scene before the first frame.
            void push_scene_sync(std::unique_ptr <base_scene>&& scene);

            void update_physics(frame_duration delta_t);
            void render(frame_duration time_since_last_frame);
            void handle_action(const sdlpp::event& ev);

            [[nodiscard]] bool empty() const;

            /// @brief Exit all scenes cleanly (called on application shutdown).
            void finish();

        private:
            void apply_push(std::unique_ptr <base_scene>&& scene);
            void apply_pop();
            void apply_replace(std::unique_ptr <base_scene>&& scene);

        private:
            std::vector <std::unique_ptr <base_scene>> m_stack;
    };
}
