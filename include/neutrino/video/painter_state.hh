//
// Created by igor on 17/11/2026.
//

#pragma once

#include <cstdint>
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
     * Each scene owns one, with its own saves (R48): the painter calls act on the scene the
     * scene manager is calling into — its on_enter / on_exit, updates, events, render(), its
     * compositor, pause, resume and resize. A new scene starts from the defaults, and what it
     * sets stays until it changes it, so a colour set in one scene never reaches another.
     * push_state() saves it and pop_state() restores it, so a helper that wraps its changes in
     * scoped_state() cannot leave them for its caller. Each call into a scene must pop every
     * push it makes: one that ends with a push left fails, after the push is undone. Pushing a
     * layer does not touch it: a layer is where draws go, the state how (R39). The clip
     * belongs to the painter, not to a target: it applies to whichever target is bound.
     */
    struct NEUTRINO_EXPORT painter_state {
        /// @brief The colour of draws that are not given one (set_draw_color()). Default:
        ///        opaque black.
        color draw_color;
        /// @brief How every draw, textures included, combines with the target
        ///        (set_draw_blend_mode()). A texture keeps no blend of its own: the form SDL uses
        ///        follows from its alpha_mode (painter.md §2.8, R49). Default: @c blend.
        blend_mode blend{blend_mode::blend};
        /// @brief How draw_line() draws: solid, dashed or dotted (set_line_style()). Default:
        ///        solid.
        line_style line;
        /// @brief Nothing outside it is drawn. Unset: no clip.
        std::optional <rect> clip;
        //view                view;                // camera transform; identity until RM2
    };

    // The painter state functions (painter.md §2.4). They act on the current scene's state;
    // main thread only, like drawing. Each fails ENFORCE with no scene current: in ready(),
    // teardown(), a scene's constructor (R48).
    //
    // The state is the painter's own: set_draw_color(), set_draw_blend_mode() and
    // set_line_style() change it, and each draw call sets the SDL state it uses from it, so
    // state set straight on the renderer through native() does not reach neutrino's draws.
    // The clip is applied to whichever target is bound, after a layer push or a
    // texture::bind() too (R39).

    class NEUTRINO_EXPORT painter_state_scope;

    /// @brief push_state() now; pop_state() when the returned scope ends.
    NEUTRINO_EXPORT [[nodiscard]] painter_state_scope scoped_state();
    /// @brief push_state(@p s) now; pop_state() when the returned scope ends.
    NEUTRINO_EXPORT [[nodiscard]] painter_state_scope scoped_state(const painter_state& s);

    /// @brief A copy of the current state: inspect it, or change it and pass it to push_state().
    NEUTRINO_EXPORT [[nodiscard]] painter_state get_painter_state();
    /// @brief Save the current state; the state itself does not change. The common case: save,
    ///        change a field or two, draw, restore.
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
     * one. Ending a scope never throws: a failure is logged. A scope pops only its own save:
     * one that ends after the call into its scene has undone its push, or in another scene,
     * or once the application has quit, does nothing, and that is logged.
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
            /// The id of the save the scope owns and has not ended; 0 when it owns none.
            std::uint64_t m_save{0};
            /// Pops the owned save, if any, and owns nothing afterwards. Logs a failure instead
            /// of throwing; does nothing, logged, when its save is not the current scene's
            /// most recent one.
            void finalize() noexcept;
    };
}
