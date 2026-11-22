//
// Created by igor on 17/11/2026.
//

#pragma once

#include <optional>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/blend_mode.hh>
#include <neutrino/video/color.hh>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/line_style.hh>

namespace neutrino {
    /**
     * @brief What every draw call reads: the draw colour, the blend mode, the line style and
     *        the clip (painter.md §2.4).
     *
     * There is one, the painter's (R37). push_state() saves it and pop_state() restores it,
     * so a helper that wraps its changes in scoped_state() cannot leave them for its caller.
     * Pushing a layer does not touch it: a layer is where draws go, the state how (R39). The
     * clip belongs to the painter, not to a target: it applies to whichever target is bound.
     * Each scene's frame must pop every push it makes: one that ends with a push left fails,
     * after the push is undone. Keeping a copy per scene between its frames, so that a colour
     * set by one scene does not reach the scene drawn after it, comes with RM1 step 4 (not
     * yet: until then a state set without a push carries over). The defaults are what a scene
     * draws with today: SDL's renderer defaults, and the opaque black the frame's clear
     * leaves as the draw colour.
     */
    struct NEUTRINO_EXPORT painter_state {
        /// @brief The colour of draws that are not given one (set_draw_color()). Default:
        ///        opaque black.
        color draw_color;
        /// @brief How draws without a texture combine with the target (set_draw_blend_mode()):
        ///        a preset, or a custom_blend() once it exists. Textures carry their own. Default:
        ///        @c none, SDL's. Translucent primitives drawn into a layer need @c blend: with
        ///        @c none the layer keeps colour not multiplied by alpha, and it is combined too
        ///        bright (painter.md §2.8).
        blend_mode blend{blend_mode::none};
        /// @brief How draw_line() draws: solid, dashed or dotted (set_line_style()). Default:
        ///        solid.
        line_style line;
        /// @brief Nothing outside it is drawn. Unset: no clip.
        std::optional <rect> clip;
        //view                view;                // camera transform; identity until RM2
    };

    // The painter state functions (painter.md §2.4). They act on the one painter; main thread
    // only, like drawing. Each fails ENFORCE outside a running application: before on_ready()
    // or once on_quit() has begun (application_scoped, R38).
    //
    // Until the painter keeps its own state (RM1 step 4), the state is the renderer's, which
    // is what the draw calls read: set_draw_color(), set_draw_blend_mode() and
    // set_line_style() change it, get_painter_state() reads it, push_state() saves it and
    // pop_state() writes the saved state back. The clip is the painter's own: it is applied to
    // whichever target is bound, after a layer push or a texture::bind() too (R39).

    class NEUTRINO_EXPORT painter_state_scope;

    /// @brief push_state() now; pop_state() when the returned scope ends.
    NEUTRINO_EXPORT [[nodiscard]] painter_state_scope scoped_state();
    /// @brief push_state(@p s) now; pop_state() when the returned scope ends.
    NEUTRINO_EXPORT [[nodiscard]] painter_state_scope scoped_state(const painter_state& s);

    /// @brief A copy of the current state: inspect it, or change it and pass it to push_state().
    NEUTRINO_EXPORT [[nodiscard]] painter_state get_painter_state();
    /// @brief Save the current state; the state itself does not change. The common case: save,
    ///        change a field or two, draw, restore. A custom blend set on the renderer through
    ///        native() is saved and restored as it is, though get_painter_state() cannot name it.
    NEUTRINO_EXPORT void push_state();
    /// @brief Save the current state, then make @p s the state: a helper that wants a known
    ///        state whatever its caller set (`push_state(painter_state{})`: the defaults).
    NEUTRINO_EXPORT void push_state(const painter_state& s);
    /// @brief Restore the state the most recent push_state() saved. ENFORCE: something is
    ///        pushed.
    NEUTRINO_EXPORT void pop_state();

    /**
     * @brief Ends a push_state() when it is destroyed: what scoped_state() returns, so an
     *        exception cannot leave a save behind.
     *
     * Movable, not assignable. A moved-from scope ends nothing. Scopes end in the reverse
     * order of their creation, as pushes and pops do, which is why one cannot be assigned
     * over: the new scope has already pushed, so ending the old one would pop the new save.
     * To replace a scope held in a @c std::optional, @c reset() it, then @c emplace() the new
     * one. Ending a scope never throws: a failure is logged. After the application has quit
     * there is nothing to pop, and ending a scope does nothing.
     */
    class NEUTRINO_EXPORT painter_state_scope {
        friend painter_state_scope scoped_state();
        friend painter_state_scope scoped_state(const painter_state& s);

        public:
            /// @brief Takes @p other's push; @p other ends nothing.
            painter_state_scope(painter_state_scope&& other) noexcept;
            painter_state_scope& operator=(painter_state_scope&& other) = delete;
            painter_state_scope(const painter_state_scope&) = delete;
            painter_state_scope& operator=(const painter_state_scope&) = delete;

            /// @brief Pops the push this scope owns, if it still owns one.
            ~painter_state_scope();
        private:
            /// push_state(), owned by the new scope.
            painter_state_scope();
            /// push_state(@p state), owned by the new scope.
            explicit painter_state_scope(const painter_state& state);
        private:
            /// True while the scope owns a push it has not ended.
            bool m_valid{false};
            /// Pops the owned push, if any, and owns nothing afterwards. Logs a failure instead
            /// of throwing; does nothing once the application has quit.
            void finalize() noexcept;
    };
}
