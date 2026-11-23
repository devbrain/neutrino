//
// Created by igor on 20/11/2026.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include <neutrino/video/composition.hh>
#include <neutrino/video/texture.hh>

#include "video/painter_state_stack.hh"

namespace neutrino {
    class base_scene;
}

namespace neutrino::detail {
    class canvas;
    class layer_stack;

    /**
     * @brief Switches for the shortcuts that give the same pixels with less work. Both are on;
     *        the tests turn them off to check that the long way gives the same pixels.
     */
    struct render_shortcuts {
        /// The bottom full-screen scene draws straight into the target when its canvas would
        /// only copy it 1:1 (R44).
        bool bottom_scene_into_target{true};
        /// The base layer's texture serves as the canvas when drawing it onto a cleared canvas
        /// would copy it exactly (R41).
        bool base_layer_as_canvas{true};
    };

    /// @brief The switches; main thread only.
    [[nodiscard]] render_shortcuts& shortcuts() noexcept;

    /// @brief True if @p c is default_compositor, the one every scene starts with. A copy of it
    ///        made across a DLL boundary may not compare equal; it then counts as the scene's
    ///        own, which only costs the shortcuts.
    [[nodiscard]] bool is_default_compositor(const compositor& c) noexcept;

    /// @brief Where a scene's frame draws.
    enum class frame_target : std::uint8_t {
        /// Onto the scene's canvas, or into its layers, combined onto the canvas.
        canvas,
        /// Straight into the target bound now: the bottom full-screen scene when its canvas
        /// would only copy it 1:1 (R44). Its every-frame canvas gives its texture back.
        bound_target,
    };

    /**
     * @brief One scene's frame: brackets its render(), then combines its layers
     *        (painter.md §3.2).
     *
     * The scene manager makes one around each scene's render():
     * @code
     * detail::scene_frame frame(scene, below);
     * scene->render();
     * frame.finish();
     * @endcode
     * The constructor makes the scene's painter state current for the frame (R48), then starts
     * the scene's canvas frame, with the geometry of the target bound now, which the canvas and
     * the layers mirror (R42). In a scene without layers it binds
     * the canvas, so render() draws onto it. In a scene with layers it starts the layer frame
     * instead (layer_stack::begin_frame(), which pushes the base layer), and makes the scene's
     * layers the ones the free push_layer() / pop_layer() reach; the canvas is bound only when
     * the layers are combined (R41).
     *
     * finish() ends the frame. Without layers it marks the canvas drawn. With layers it ends
     * the layer frame (layer_stack::end_frame(), which fails if a layer is left pushed), then
     * combines (R43): binds the canvas, runs the scene's compositor with the default painter
     * state and a composition whose below() is @p below, and marks the layers pushed this
     * frame and the canvas drawn. When the base layer's texture can serve as the result —
     * the default compositor, the base layer's settings the identity, an every-frame canvas —
     * the declared layers are drawn straight onto it and the canvas shows it (R41). A canvas that cannot be made skips the compositor. Then it
     * ends the canvas's frame and its binding, and checks that the frame popped every
     * push_state() it pushed, as the compositor must too: a push left is popped, and the frame
     * fails naming it. The destructor puts back the painter state current before the frame.
     *
     * If render() throws, finish() is not reached and the destructor ends every push instead
     * (layer_stack::abort_frame()) and the canvas's binding, so the target bound before the
     * frame is bound again. If the compositor throws, its bindings and state end with it.
     * Either way the destructor ends the frame: no scene's layers are current afterwards.
     *
     * A null scene makes a frame with nothing in it: nothing is bound or pushed, push_layer()
     * fails as in a scene without layers, and the painter state current before stays so.
     *
     * Frames do not nest: scenes render one after another. Main thread only.
     */
    class scene_frame {
        public:
            /// @brief Starts the frame of @p scene, or an empty frame if null. @p below: the
            ///        canvases of the scenes drawn under it, bottom to top, for its compositor;
            ///        they must outlive the frame. @p onto: where it draws. ENFORCE: no other
            ///        frame is open; a scene drawn onto the bound target has no layers.
            explicit scene_frame(base_scene* scene, std::span <const texture* const> below = {},
                                 frame_target onto = frame_target::canvas);
            /// @brief Ends the frame: aborts it if finish() was not reached. Never throws.
            ~scene_frame();

            scene_frame(const scene_frame&) = delete;
            scene_frame& operator=(const scene_frame&) = delete;

            /// @brief render() returned: without layers, marks the canvas drawn; with layers,
            ///        ends the layer frame and combines. Then ends the canvas's frame and
            ///        binding. ENFORCE (from end_frame()): only the base layer is left pushed;
            ///        on that failure every push has already been ended, so the destructor has
            ///        nothing to undo. What the compositor throws passes through.
            void finish();

            /// @brief True when render() draws straight into the target bound before the frame:
            ///        the scene draws there (frame_target::bound_target), or its own target —
            ///        its canvas, or its base layer — could not be made.
            [[nodiscard]] bool draws_into_target() const;

            /// @brief True while a frame is open, whether or not its scene has layers; the
            ///        compositor runs inside it.
            [[nodiscard]] static bool rendering() noexcept;
            /// @brief The layers of the scene being rendered; null outside a frame, and in a
            ///        scene without layers.
            [[nodiscard]] static layer_stack* get_current_layers_stack() noexcept;
            /// @brief clear() for the scene being rendered: its current layer; without layers,
            ///        its canvas; drawing straight into the target, that target, to black.
            ///        ENFORCE: a frame is open.
            static void clear_current();
        private:
            /// Runs the compositor onto the canvas, then marks the pushed layers drawn.
            void combine();

            /// The scene's painter state, current for the frame; none for a null scene. Made
            /// before anything is bound, so the canvas and the layers take its clip, and ended
            /// after the binding, so the target bound before gets back the clip it had.
            std::optional <painter_state_stack::current_scope> m_painter;
            layer_stack* m_layers;
            canvas* m_canvas;
            const compositor* m_compositor;
            std::span <const texture* const> m_below;
            /// The canvas bound for render() in a scene without layers.
            std::optional <renderer_target_scope> m_canvas_binding;
            bool m_finished{false};
            static bool s_rendering;
            static layer_stack* s_current;
            /// The frame open now; null outside one.
            static scene_frame* s_frame;
    };
}
