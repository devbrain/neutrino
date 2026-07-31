//
// A frame-scoped snapshot of continuous input (pointer position + mouse button held/edge
// state), sampled ONCE per frame by the application and handed to base_scene::fixed_update.
// A pure value type: no service lookup, directly constructible in a test so a scene's update
// can be driven with synthetic input (findings #12). Discrete, one-shot events still arrive
// through base_scene::handle_action.
//

#pragma once

#include <sdlpp/events/mouse_codes.hh>     // sdlpp::mouse_button
#include <sdlpp/utility/geometry_types.hh> // sdlpp::point<float>

namespace neutrino {
    /// @brief The pointer position this frame, in BOTH window and render space. @c render is the one
    ///        scenes want: window input already mapped through to_render_coords() (so HiDPI scaling
    ///        and logical-presentation letterboxing are accounted for), so a scene never re-derives it.
    struct pointer_state {
        sdlpp::point <float> window{}; ///< Window pixels, as SDL delivers mouse coordinates.
        sdlpp::point <float> render{}; ///< Render / logical-presentation space.
        /// @brief Is the pointer actually over this application's window?
        ///
        /// **A scene that steers something from the pointer must check this.** When false the
        /// position is not meaningful -- the cursor is outside the window, or no pointer event has
        /// been seen yet -- and acting on it would snap the controlled object to a stale or
        /// arbitrary spot (KE's paddle jumping to the left wall at startup).
        bool on_screen{false};
    };

    /// @brief Per-frame edge/held state for one button. @c pressed / @c released are true ONLY on the
    ///        transition frame; @c held is true whenever the button is down.
    struct button_state {
        bool pressed = false;
        bool released = false;
        bool held = false;
    };

    /// @brief Immutable, frame-scoped input, sampled once per frame and passed to fixed_update.
    ///
    /// Input is polled at frame rate but consumed at the sim rate, so one frame's snapshot may be
    /// delivered to several fixed substeps. **Edges are delivered to exactly one of them.** A frame
    /// running N substeps hands the full snapshot to the first and an @ref without_edges copy to the
    /// rest, so a one-shot action (fire, jump, menu click) runs once per physical press instead of
    /// once per substep -- at the default 120 Hz sim on a 60 Hz display that would otherwise be
    /// *twice* for every click. @c held is current in every substep, since it describes a state
    /// rather than a transition.
    ///
    /// (A press landing on a frame that runs zero substeps is still best-effort: it is seen only if
    /// the button is still down on the next stepping frame. A latch can close that if a scene ever
    /// needs frame-perfect edges.)
    class input_snapshot {
        public:
            input_snapshot() = default;

            input_snapshot(pointer_state pointer,
                           button_state left, button_state middle, button_state right) noexcept
                : m_pointer(pointer), m_left(left), m_middle(middle), m_right(right) {
            }

            /// @brief A copy with every @c pressed / @c released edge cleared but @c held (and the
            /// pointer) intact -- what the 2nd..Nth substep of a frame receives, so a transition is
            /// never replayed. See the class note.
            [[nodiscard]] input_snapshot without_edges() const noexcept {
                const auto strip = [](button_state b) { return button_state{false, false, b.held}; };
                return input_snapshot{m_pointer, strip(m_left), strip(m_middle), strip(m_right)};
            }

            /// @brief The pointer position this frame (window + render space).
            [[nodiscard]] const pointer_state& pointer() const noexcept { return m_pointer; }

            /// @brief Edge/held state for a mouse button this frame (x1/x2 read as "nothing pressed").
            [[nodiscard]] button_state mouse(sdlpp::mouse_button b) const noexcept {
                switch (b) {
                    case sdlpp::mouse_button::middle: return m_middle;
                    case sdlpp::mouse_button::right:  return m_right;
                    case sdlpp::mouse_button::left:   return m_left;
                    default:                          return {};
                }
            }

        private:
            pointer_state m_pointer{};
            button_state  m_left{};
            button_state  m_middle{};
            button_state  m_right{};
    };
}
