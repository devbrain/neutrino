//
// Internal: suppression of one-shot input EDGES for the 2nd..Nth fixed substep of a frame.
//
// The polled input APIs (hotkey, mouse_click, gamepad_button) read SDL's per-frame transients,
// which are cleared once per FRAME (after rendering) -- not once per simulation step. With a
// fixed-step scheduler a single frame runs several fixed_update calls, so an ungated
// `hotkey::pressed()` reads true in every one of them and a one-shot action fires repeatedly for
// one physical press. At the default 120 Hz sim on a 60 Hz display that is twice per press.
//
// The application raises this flag for every substep after the first, so edges are observable in
// exactly one substep -- matching input_snapshot::without_edges(), which does the same job for the
// snapshot path. `held` is never suppressed: it describes a state, not a transition, and every
// substep should see it.
//
#pragma once

#include <sdlpp/app/game_application.hh> // sdlpp::button_state

namespace neutrino::input_detail {
    /// @brief Are one-shot edges currently masked (i.e. this is a follow-up substep)?
    [[nodiscard]] bool edges_suppressed() noexcept;

    /// @brief Set the mask, returning the previous value. Prefer @ref edge_suppression.
    bool set_edges_suppressed(bool value) noexcept;

    /// @brief Drop @c pressed / @c released from @p s while the mask is raised; @c held survives.
    [[nodiscard]] inline sdlpp::button_state gate_edges(sdlpp::button_state s) noexcept {
        if (edges_suppressed()) {
            s.pressed = false;
            s.released = false;
        }
        return s;
    }

    /// @brief Scoped edge mask -- restores the previous value on exit, including when a scene's
    /// fixed_update throws (otherwise edges would stay masked for the rest of the run).
    class edge_suppression {
        public:
            explicit edge_suppression(bool on) noexcept
                : m_prev(set_edges_suppressed(on)) {
            }

            ~edge_suppression() {
                set_edges_suppressed(m_prev);
            }

            edge_suppression(const edge_suppression&) = delete;
            edge_suppression& operator=(const edge_suppression&) = delete;

        private:
            bool m_prev;
    };
} // namespace neutrino::input_detail
