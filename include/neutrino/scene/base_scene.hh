/// @file base_scene.hh
/// @brief Abstract base class for all game scenes (gameplay, menus, loading, etc.).

#pragma once

#include <chrono>
#include <sdlpp/events/event_category.hh>
#include <neutrino/neutrino_export.h>
#include <neutrino/video/geometry_types.hh>

namespace neutrino {
    using frame_duration = std::chrono::duration <float, std::milli>;

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

            /// @brief Called when another scene is pushed on top of this one.
            virtual void on_pause() {
            }

            /// @brief Called when the scene above is popped, making this one active again.
            virtual void on_resume() {
            }

            /// @brief Called with the current render_size() when this scene becomes
            /// active (enter/resume) and whenever the render coordinate space changes
            /// (window resize, fullscreen toggle, HiDPI move). In logical-presentation
            /// mode the render space is constant, so it fires once at activation.
            /// Default: no-op — override to lay out against the render size instead of
            /// polling it every frame.
            virtual void on_resize([[maybe_unused]] dim size) {
            }

            /// @brief Fixed-timestep logic update (input processing, state changes, polling).
            virtual void update_physics(frame_duration delta_t) = 0;
            /// @brief Render the scene; called every frame after update_physics.
            virtual void render(frame_duration time_since_last_frame) = 0;
            /// @brief Handle an SDL event (input, window, etc.).
            virtual void handle_action(const sdlpp::event& ev) = 0;
            /// @brief Return true if this scene fully covers the screen (no need to render below).
            [[nodiscard]] virtual bool is_opaque() const = 0;
    };
}
