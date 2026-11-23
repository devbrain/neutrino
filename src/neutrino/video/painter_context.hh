//
// Created by igor on 23/11/2026.
//

#pragma once

#include <optional>
#include <utility>

#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/texture.hh>

namespace neutrino::detail {
    class painter_state_stack;
    class scene_frame;

    /**
     * @brief What is current for drawing: one per application (painter.md §3.1).
     *
     * Three axes, each changed by scopes of its own, because they change at different times:
     *   - the scene the scene manager is calling into, whose painter state the draw calls read
     *     (R48): painter_state_stack::current_scope;
     *   - the frame open now, one scene's render() and its compositor: scene_frame;
     *   - the bound target, which may discard what is drawn into it (§3.6) and which holds
     *     the painter's clip (R39): discard_scope, texture::bind() and the
     *     renderer_target_scope it returns.
     * A layer push changes the target but not the state (R39), so no scope sets more than one
     * axis.
     *
     * application_scoped (R38): made at first use, destroyed when the application quits, so
     * each application starts with nothing current. Main thread only, like drawing.
     */
    struct painter_context {
        /// The current scene's painter state; null when no scene is current.
        painter_state_stack* scene{nullptr};
        /// The frame open now; null outside one.
        scene_frame* frame{nullptr};
        /// Drawing into the bound target does nothing: it is a layer's or a canvas's whose
        /// texture could not be made (§3.6).
        bool discards{false};

        /// The application's, made now if there is none. ENFORCE: the application is
        /// running (between on_ready() and on_quit()).
        [[nodiscard]] static painter_context& instance();
        /// The application's; null before its first use, and once the application quits.
        [[nodiscard]] static painter_context* maybe() noexcept;
    };

    // ---- the bound target -----------------------------------------------------------------------

    /// True while the bound target discards drawing; false with no application.
    [[nodiscard]] bool target_discards() noexcept;

    // The painter's clip follows the painter (R39): SDL keeps a clip per target, so it is put
    // on each target bound. A clip set straight on the renderer — a UI backend's — is kept
    // when the painter has none: only a texture just bound loses the clip it held. The
    // painter's clip is the current scene's (R48); with no scene current there is none.

    /// Sets @p clip on the target bound now. A failure is logged and counted, like a draw's.
    void apply_clip(const std::optional <rect>& clip);

    /// apply_clip(), if there is a renderer. Never throws: a failure is counted.
    void try_apply_clip(const std::optional <rect>& clip) noexcept;

    /// The state of a texture just bound (texture::bind(), so layer pushes too): it is drawn
    /// into, inside a failed layer too, and it gets the painter's clip if it has one;
    /// otherwise none, so a pooled texture does not keep its last owner's.
    void texture_bound();

    /// What a renderer_target_scope saves of the target bound when it begins.
    [[nodiscard]] target_state save_target() noexcept;

    /// Puts back what save_target() saved, when the target bound before is bound again:
    /// whether it discards, and its clip — the painter's if it has one; none if it had one
    /// when the scope began, since the target held that clip and it is gone; otherwise the
    /// target keeps its own. Never throws: a failure is counted.
    void restore_target(const target_state& saved) noexcept;

    /**
     * @brief While alive, the bound target discards drawing: what a failed layer or canvas
     *        holds instead of a binding. Ending it puts back the value from before.
     *
     * Movable, not assignable; a moved-from scope puts back nothing. Scopes end in the reverse
     * order of their creation, as target bindings do.
     */
    class discard_scope {
        public:
            discard_scope();

            discard_scope(discard_scope&& other) noexcept
                : m_previous(other.m_previous), m_owns(std::exchange(other.m_owns, false)) {
            }

            discard_scope& operator=(discard_scope&&) = delete;
            discard_scope(const discard_scope&) = delete;
            discard_scope& operator=(const discard_scope&) = delete;

            ~discard_scope();

        private:
            /// The value before this scope set it.
            bool m_previous;
            /// False once moved from.
            bool m_owns{true};
    };
}
