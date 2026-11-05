/// @file scenes_manager.hh
/// @brief Stack-based scene manager that drives the scene lifecycle and frame dispatch.

#pragma once

#include <memory>
#include <vector>

#include <neutrino/scene/base_scene.hh>

namespace neutrino {
    /// @brief Manages a stack of base_scene instances, dispatching lifecycle and frame events.
    /// Only the topmost scene receives input. Rendering walks down to the nearest scene with
    /// scene_flags::hides_below, updating to the nearest one with scene_flags::pauses_below.
    /// on_pause / on_resume of the scene below follow pauses_below.
    class scenes_manager {
        public:
            scenes_manager();
            ~scenes_manager();

            /// @brief Push a new scene on top; the current one gets on_pause if the new scene
            /// has scene_flags::pauses_below.
            static void push_scene(std::unique_ptr <base_scene>&& scene);
            /// @brief Pop the topmost scene; the one below gets on_resume if the popped scene
            /// had scene_flags::pauses_below.
            static void pop_scene();
            /// @brief Replace the topmost scene (pop + push as a single operation).
            static void replace_scene(std::unique_ptr <base_scene>&& scene);

            /// @brief Push a scene synchronously, bypassing the event queue.
            /// Only safe while no scene callbacks are running; used by
            /// application to install the initial scene before the first frame.
            void push_scene_sync(std::unique_ptr <base_scene>&& scene);

            void fixed_update(sim_duration dt, const input_snapshot& in);
            void render();
            void handle_action(const sdlpp::event& ev);

            /// @brief Deliver a render-space size change to the active (top) scene.
            /// Called by application when the drawable changes; scene activation
            /// fires on_resize on its own. No-op when the stack is empty.
            void notify_resize(dim size);

            [[nodiscard]] bool empty() const;

            /// @brief Exit all scenes cleanly (called on application shutdown).
            void finish();

        private:
            void apply_push(std::unique_ptr <base_scene>&& scene);
            void apply_pop();
            void apply_replace(std::unique_ptr <base_scene>&& scene);
            /// Fire on_resize on the current top scene with the live render_size().
            void notify_top_resized();

        private:
            std::vector <std::unique_ptr <base_scene>> m_stack;
    };
}
