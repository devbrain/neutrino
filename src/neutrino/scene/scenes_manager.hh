/// @file scenes_manager.hh
/// @brief Stack-based scene manager that drives the scene lifecycle and frame dispatch.

#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include <neutrino/scene/base_scene.hh>

namespace neutrino {
    /// @brief Manages a stack of base_scene instances, dispatching lifecycle and frame events.
    /// Input runs from the top down through scene_flags::passes_input scenes. Rendering walks
    /// down to the nearest scene with scene_flags::hides_below, updating to the nearest one with
    /// scene_flags::pauses_below. on_pause / on_resume of the scene below follow pauses_below.
    /// An optional pinned scene sits above the stack: push, pop and replace act below it, and
    /// otherwise it is the topmost scene (kept as the last element of the stack).
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
            /// @brief Pin a scene above the stack, replacing the pinned scene if there is one.
            static void pin_scene(std::unique_ptr <base_scene>&& scene);
            /// @brief Exit and destroy the pinned scene; a no-op when none is pinned.
            static void unpin_scene();

            /// @brief Push a scene synchronously, bypassing the event queue.
            /// Only safe while no scene callbacks are running; used by
            /// application to install the initial scene before the first frame.
            void push_scene_sync(std::unique_ptr <base_scene>&& scene);

            void fixed_update(sim_duration dt, const input_snapshot& in);
            /// @brief Clears the target bound now to black, unless covers_target(), and draws
            ///        the visible scenes into it, bottom to top (painter.md §3.2).
            void render();
            /// @brief True when this frame's render() will cover the whole target with an
            ///        opaque copy — the bottom visible scene's canvas, drawn with @c none in
            ///        place — and so does not clear it first (R44). render() clears the target
            ///        itself otherwise, and late if that canvas ends the frame with nothing to
            ///        show.
            [[nodiscard]] bool covers_target() const;
            void handle_action(const sdlpp::event& ev);

            /// @brief Deliver a render-space size change to the active (top) scene and to the
            /// pinned scene.
            /// Called by application when the drawable changes; scene activation
            /// fires on_resize on its own. No-op when the stack is empty.
            void notify_resize(dim size);

            [[nodiscard]] bool empty() const;

            /// @brief Exit all scenes cleanly (called on application shutdown).
            void finish();

        private:
            /// What render() does this frame: the bottom visible scene (the topmost with
            /// hides_below, or the first), whether it draws straight into the target (R44),
            /// and whether its canvas's copy covers the target.
            struct frame_plan {
                std::size_t bottom{0};
                bool direct{false};
                bool covers{false};
            };
            [[nodiscard]] frame_plan plan() const;
            /// The scene's canvas before its on_enter(): its default blend from its flags.
            static void prepare_canvas(base_scene* scene);

            void apply_push(std::unique_ptr <base_scene>&& scene);
            void apply_pop();
            void apply_replace(std::unique_ptr <base_scene>&& scene);
            void apply_pin(std::unique_ptr <base_scene>&& scene);
            void apply_unpin();
            /// Fire on_resize on the top stack scene (below the pinned one) with the live
            /// render_size().
            void notify_top_resized();
            /// Run on_enter; if it throws, give the scene on_exit and erase it. False on rollback.
            bool enter_or_roll_back(base_scene* scene);
            /// The scenes push, pop and replace act on: the stack without the pinned scene.
            [[nodiscard]] std::size_t active_count() const noexcept;
            /// The top of those scenes, or nullptr.
            [[nodiscard]] base_scene* active_top() const noexcept;

            std::vector <std::unique_ptr <base_scene>> m_stack;
            /// render()'s lists, kept so a frame allocates nothing: the canvases drawn so far,
            /// for below(), and the canvases to draw into the target.
            std::vector <const texture*> m_below;
            std::vector <detail::canvas*> m_finished;
            bool m_pinned = false; ///< m_stack.back() is the pinned scene
    };
}
