/// @file base_scene.hh
/// @brief Abstract base class for all game scenes (gameplay, menus, loading, etc.).

#pragma once

#include <memory>
#include <cstdint>
#include <chrono>

#include <sdlpp/events/event_category.hh>
#include <neutrino/neutrino_export.h>
#include <neutrino/input/input_snapshot.hh>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/composition.hh>
#include <neutrino/video/layer_id.hh>
#include <neutrino/video/layer_settings.hh>
#include <neutrino/detail/enum.hh>

namespace neutrino {
    /// @brief Floating-point milliseconds. Wall-clock frame delta for render-rate cosmetics; the
    /// simulation itself runs on @ref sim_duration, not this.
    using frame_duration = std::chrono::duration <float, std::milli>;

    /// @brief The one simulation-time duration, seconds-based. Physics speaks this end to end; it is
    /// the fixed tick handed to @ref base_scene::fixed_update.
    using sim_duration = std::chrono::duration <float>;

    /// @brief A raw SDL event (keyboard, mouse, window, text input, ...), as delivered to
    ///        @ref base_scene::handle_event and application::event.
    using event = sdlpp::event; // sdlpp-ok

    /// @brief What a scene does to the scenes beneath it on the stack. The bits are
    ///        independent; the scene carrying them is always updated and drawn itself.
    enum class scene_flags : std::uint8_t {
        none         = 0, ///< An overlay: the scenes below keep updating and are drawn.
        hides_below  = 1, ///< The scenes below are not drawn (this scene covers the display).
        pauses_below = 2, ///< The scenes below are not updated; the one directly below gets
                          ///< on_pause when this scene is pushed and on_resume when it is popped.
        passes_input = 4  ///< The scene sees the live input and the events, and passes them on
                          ///< to the scene below. Input runs from the top down through such
                          ///< scenes and stops at the first scene without this flag; it never
                          ///< reaches a scene paused by pauses_below.
    };

    d_NEUTRINO_PROMOTE_ENUM(scene_flags);

    namespace detail {
        class layer_stack;
        struct scene_context_access;
        class canvas;
        class painter_state_stack;
    }

    class texture;

    /// @brief Abstract scene interface with lifecycle hooks and per-frame callbacks.
    /// Scenes are managed in a stack by scenes_manager.
    class NEUTRINO_EXPORT base_scene {
        friend struct detail::scene_context_access;

        public:
            base_scene();
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
            /// input, unless every scene above them carries scene_flags::passes_input. This is
            /// where input is polled and state advances.
            virtual void fixed_update(sim_duration dt, const input_snapshot& in) = 0;
            /// @brief Render the scene; called every frame after fixed_update. Draws the latest
            /// committed state -- no time argument and no interpolation factor (for integer-scaled
            /// pixel art, sub-step interpolation would round away anyway).
            virtual void render() = 0;
            /// @brief Handle an SDL event (input, window, etc.).
            virtual void handle_event(const event& ev);
            /// @brief What this scene does to the scenes beneath it (see @ref scene_flags).
            /// Default: `hides_below | pauses_below` -- a full-screen scene. Overlays override it:
            /// a pause menu returns `pauses_below`, a busy spinner returns `none`.
            /// `passes_input` may change at any time: it only routes input and is read on every
            /// update and every event. The other bits must not change while the scene is on the
            /// stack: push and pop each read `pauses_below`, and a change in between would
            /// unbalance on_pause / on_resume.
            [[nodiscard]] virtual scene_flags flags() const;

            /**
             * @brief Replaces the scene's compositor, which draws its layers onto its canvas
             *        after each render() (painter.md §2.9). Every scene starts with
             *        default_compositor; `set_compositor(default_compositor)` puts it back. A
             *        scene that declares no layer draws straight onto its canvas, and its
             *        compositor is not called.
             *
             * ENFORCE: @p c is not empty; the scene's compositor is not running.
             */
            void set_compositor(compositor c);
            /// @brief The scene's compositor: to wrap it (a transition), or to put it back later.
            [[nodiscard]] const compositor& get_compositor() const noexcept;

        protected:
            /**
             * @brief Declares a layer of this scene, after the ones declared before it, and
             *        returns its id (painter.md §2.5).
             *
             * Meant for the scene's constructor or member initialisers. The first call also
             * makes the scene's base layer (R31), under every declared one. No texture is taken
             * yet: a layer takes its target at its first push. A scene that declares no layer
             * draws as before, straight onto the bound target, and pays nothing.
             *
             * The layers are pushed with the free push_layer() / pop_layer()
             * (video/layer_ops.hh) during this scene's render(); an id of this scene's is
             * refused while another scene renders.
             */
            [[nodiscard]] layer_id declare_layer(const layer_settings& settings = {});

            /**
             * @brief How the layer @p lid is drawn when the scene's layers are combined: its
             *        style, live (painter.md §2.7).
             *
             * Change it at any time — in fixed_update(), in render() — and the compositor reads
             * it after render(): `style_of(m_world).offset = m_shake.offset();`. Animating it
             * frame by frame is how a layer fades, shakes or turns; nothing is redrawn into the
             * layer. Its first value is layer_settings::style.
             *
             * ENFORCE: the scene declared layers, and @p lid is one of them. The base layer —
             * where drawing with no layer pushed goes — has no style of its own: it is drawn
             * as it is; declare a layer for drawing that must move or fade (R45).
             */
            [[nodiscard]] layer_style& style_of(layer_id lid);

        private:
            /// The scene's layers; made by the first declare_layer(), null until then. Reached
            /// by the scene manager through detail::scene_context_access.
            std::unique_ptr <detail::layer_stack> m_layer_stack;
            /// The scene's result; made with the scene, its texture taken when first drawn (R41).
            std::unique_ptr <detail::canvas> m_canvas;
            /// Draws the layers onto the canvas after render(); run by detail::scene_frame.
            compositor m_compositor{default_compositor};
            /// The scene's painter state and its saves (R48): current during every call the
            /// scene manager makes into the scene.
            std::unique_ptr <detail::painter_state_stack> m_painter_state_stack;
    };
}
