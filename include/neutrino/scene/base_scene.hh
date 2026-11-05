/// @file base_scene.hh
/// @brief Abstract base class for all game scenes (gameplay, menus, loading, etc.).

#pragma once

#include <cstdint>
#include <chrono>

#include <sdlpp/events/event_category.hh>
#include <neutrino/neutrino_export.h>
#include <neutrino/input/input_snapshot.hh>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/detail/enum.hh>

namespace neutrino {
    /// @brief Floating-point milliseconds. Wall-clock frame delta for render-rate cosmetics; the
    /// simulation itself runs on @ref sim_duration, not this.
    using frame_duration = std::chrono::duration <float, std::milli>;

    /// @brief The one simulation-time duration, seconds-based. Physics speaks this end to end; it is
    /// the fixed tick handed to @ref base_scene::fixed_update.
    using sim_duration = std::chrono::duration <float>;

    /// @brief A raw SDL event (keyboard, mouse, window, text input, ...), as delivered to
    ///        @ref base_scene::handle_action and application::event.
    using event = sdlpp::event; // sdlpp-ok

    /// @brief What a scene does to the scenes beneath it on the stack. The two bits are
    ///        independent; the scene carrying them is always updated and drawn itself.
    enum class scene_flags : std::uint8_t {
        none         = 0, ///< An overlay: the scenes below keep updating and are drawn.
        hides_below  = 1, ///< The scenes below are not drawn (this scene covers the display).
        pauses_below = 2  ///< The scenes below are not updated; the one directly below gets
                          ///< on_pause when this scene is pushed and on_resume when it is popped.
    };
    d_NEUTRINO_PROMOTE_ENUM(scene_flags);

    /// @brief Abstract scene interface with lifecycle hooks and per-frame callbacks.
    /// Scenes are managed in a stack by scenes_manager.
    class NEUTRINO_EXPORT base_scene {
        public:
            virtual ~base_scene();

            /// @brief Called when the scene becomes the active (top) scene.
            virtual void on_enter() {
            }

            /// @brief Called when the scene is removed from the stack.
            virtual void on_exit() {
            }

            /// @brief Called when a scene with scene_flags::pauses_below is pushed on top of this
            /// one: fixed_update stops until on_resume.
            virtual void on_pause() {
            }

            /// @brief Called when the scene above, which paused this one, is popped: fixed_update
            /// runs again.
            virtual void on_resume() {
            }

            /// @brief Called with the current render_size() when this scene becomes
            /// active (enter/resume) and whenever the render coordinate space changes
            /// (window resize, fullscreen toggle, HiDPI move). In logical-presentation
            /// mode the render space is constant, so it fires once at activation.
            /// Default: no-op — override to lay out against the render size instead of
            /// polling it every frame.
            virtual void on_resize(dim size);

            /// @brief Fixed-timestep logic update. Called 0..N times per frame with a CONSTANT @p dt
            /// and the frame's @p in input snapshot. The first substep receives input edges;
            /// later substeps receive held state without edges. Covered scenes receive neutral
            /// input. This is where input is polled and state advances.
            virtual void fixed_update(sim_duration dt, const input_snapshot& in) = 0;
            /// @brief Render the scene; called every frame after fixed_update. Draws the latest
            /// committed state -- no time argument and no interpolation factor (for integer-scaled
            /// pixel art, sub-step interpolation would round away anyway).
            virtual void render() = 0;
            /// @brief Handle an SDL event (input, window, etc.).
            virtual void handle_action(const event& ev);
            /// @brief What this scene does to the scenes beneath it (see @ref scene_flags).
            /// Default: `hides_below | pauses_below` -- a full-screen scene. Overlays override it:
            /// a pause menu returns `pauses_below`, a busy spinner returns `none`. The value must
            /// not change while the scene is on the stack: push and pop each read it, and a change
            /// in between would unbalance on_pause / on_resume.
            [[nodiscard]] virtual scene_flags flags() const;
    };
}
