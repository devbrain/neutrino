//
// Created by igor on 23/11/2026.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include <neutrino/video/blend_mode.hh>
#include <neutrino/video/color.hh>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/line_style.hh>
#include <neutrino/video/painter_state.hh>

namespace neutrino::detail {
    /**
     * A scene's painter state and its saves (painter.md §2.4, R48): what the draw calls read
     * while the scene is current. Each scene owns one. The current one is the scene the scene
     * manager is calling into (painter_context::scene), made current by a current_scope; with
     * none current, every painter call fails ENFORCE (instance()).
     *
     * The state is the painter's own: the draw calls set the SDL state they use from it at
     * each call, so the renderer's colour, blend and line style are scratch. The clip is put
     * on whichever target is bound (R39).
     */
    class painter_state_stack {
        public:
            class current_scope;

            painter_state_stack();
            ~painter_state_stack();

            painter_state_stack(const painter_state_stack&) = delete;
            painter_state_stack& operator=(const painter_state_stack&) = delete;

            /// The current scene's; null when no scene is current.
            [[nodiscard]] static painter_state_stack* maybe() noexcept;
            /// The current scene's. ENFORCE: a scene is current.
            [[nodiscard]] static painter_state_stack& instance();

            /// The state the draw calls read.
            [[nodiscard]] const painter_state& state() const noexcept;

            void set_draw_color(const color& c) noexcept;
            void set_blend(blend_mode mode) noexcept;
            void set_line(const line_style& style) noexcept;

            /// The painter's clip, if it has one. Putting it on a target is the painter
            /// context's (apply_clip()).
            [[nodiscard]] const std::optional <rect>& clip() const noexcept;

            /// Saves the state; the state does not change. Returns the save's id (never 0).
            std::uint64_t push();

            /// Saves the state, then makes @p state the state. Returns the save's id (never 0).
            std::uint64_t push(const painter_state& state);

            /// Restores the most recent save and drops it. ENFORCE: something is pushed.
            void pop();

            /// Pops the save @p id, if it is the most recent one: what a painter_state_scope
            /// ends. Otherwise does nothing and returns false: the save was undone at the end
            /// of the call into the scene that made it, or a push made after it was not popped.
            bool pop_save(std::uint64_t id);

            /// How many saves are held.
            [[nodiscard]] std::size_t depth() const noexcept;

            /// Pops saves, newest first, until @p depth are left, each restoring the state it
            /// saved. Never throws: a failure is logged.
            void pop_to(std::size_t depth) noexcept;

            /// Ends a call into the scene that began with @p depth saves (R48): a push left is
            /// undone, then fails ENFORCE, naming @p what.
            void end_call(std::size_t depth, const char* what);

        private:
            // Makes @p s the state; the clip goes on the target bound now only if it changed,
            // so a push and pop that leave it alone keep a clip set straight on the renderer.
            void apply(const painter_state& s);

            struct save {
                painter_state state;
                std::uint64_t id;
            };

            painter_state m_state;
            std::vector <save> m_saves;
    };

    /**
     * Makes a scene's painter state current for one call the scene manager makes into the
     * scene (R48), and puts back the one current before when it ends: calls nest, as a scene's
     * update may render the stack (screenshot()). The clip of the state made current goes on
     * the target bound now, and the previous one's comes back at the end.
     *
     * finish() is the end of a call that returned: a push left is undone, then fails ENFORCE.
     * A call that threw has its pushes undone by the destructor, which never throws.
     */
    class painter_state_stack::current_scope {
        public:
            explicit current_scope(painter_state_stack& stack);
            ~current_scope();

            current_scope(const current_scope&) = delete;
            current_scope& operator=(const current_scope&) = delete;

            /// ENFORCE: the call popped every push it made, naming @p what; a push left is
            /// undone first.
            void finish(const char* what);

        private:
            painter_state_stack& m_stack;
            painter_state_stack* m_previous;
            std::size_t m_depth;
    };
}
