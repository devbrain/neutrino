//
// Created by igor on 23/11/2026.
//

#pragma once

#include <utility>

namespace neutrino::detail {
    // A layer or a canvas whose texture could not be made has nothing to draw into, so drawing
    // into it does nothing (painter.md §3.6): the painter knows the target bound for it
    // discards. Draw calls still check their arguments and that a scene is current, then
    // submit nothing; the state calls work as usual. The failure itself is counted once, when
    // the texture allocator refuses the texture. Main thread only, like drawing.
    //
    // The flag describes the bound target, so it follows the bindings: a discard_scope sets it
    // for a failed layer or canvas; texture::bind() clears it, since its texture is a real
    // target, and the renderer_target_scope it returns puts back the value from before.

    /// True while the bound target discards drawing.
    [[nodiscard]] bool target_discards() noexcept;

    /// Sets whether the bound target discards drawing; returns the value it had.
    bool set_target_discards(bool discards) noexcept;

    /**
     * @brief While alive, the bound target discards drawing: what a failed layer or canvas
     *        holds instead of a binding. Ending it puts back the value from before.
     *
     * Movable, not assignable; a moved-from scope puts back nothing. Scopes end in the reverse
     * order of their creation, as target bindings do.
     */
    class discard_scope {
        public:
            discard_scope() noexcept
                : m_previous(set_target_discards(true)) {
            }

            discard_scope(discard_scope&& other) noexcept
                : m_previous(other.m_previous), m_owns(std::exchange(other.m_owns, false)) {
            }

            discard_scope& operator=(discard_scope&&) = delete;
            discard_scope(const discard_scope&) = delete;
            discard_scope& operator=(const discard_scope&) = delete;

            ~discard_scope() {
                if (m_owns) {
                    (void) set_target_discards(m_previous);
                }
            }

        private:
            /// The value before this scope set it.
            bool m_previous;
            /// False once moved from.
            bool m_owns{true};
    };
}
