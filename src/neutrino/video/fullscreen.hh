//
// Created by igor on 25/11/2026.
//

#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>

#include "events/event_types.hh"

namespace neutrino::detail {
    /// The engine event a toggle_fullscreen() posts: toggles wait to be applied. No payload.
    enum fullscreen_events : std::int32_t {
        TOGGLE_FULLSCREEN
    };

    using fullscreen_event = custom_event <fullscreen_events>;

    /**
     * @brief The state behind the deferred toggle_fullscreen() (P3.11, "Deferred fullscreen").
     *
     * A toggle flips the state the window is in, or is being switched to: the state last sent
     * to SDL, or a switch made by something else (the window manager, the system), which is
     * followed. Toggles are applied when their event is handled, at the start of the next
     * cycle: toggles made in one cycle cancel in pairs, and an odd number makes one switch.
     * Flipping SDL's fullscreen flag instead would lose a toggle on window systems that switch
     * asynchronously (Wayland, macOS): the flag changes only when a switch completes, so a
     * second toggle before that would ask for the same state again.
     *
     * No SDL here: the application posts the event, makes the SDL call and reports the result
     * through track(). toggle() may come from any thread; the rest is the main thread's.
     */
    class fullscreen_tracker {
        public:
            /// The application is up and its window is in @p actual.
            void begin(bool actual);

            /// The application stops: toggles not applied yet are dropped.
            void end();

            /// Flip the state, calling @p post to post the request event unless one is waiting
            /// already. If @p post throws, the toggle is undone. ENFORCE: an application runs
            /// (between begin() and end()).
            void toggle(const std::function <void()>& post);

            /// The request event is handled: the state to switch to, if the toggles since the
            /// last one did not cancel out.
            [[nodiscard]] std::optional <bool> take();

            /// The window is in, or is being switched to, @p state: SDL took a switch, refused
            /// one (the state it is still in), or the window entered or left fullscreen.
            void track(bool state);

        private:
            std::mutex m_mutex;
            bool m_running = false;
            // The state the window is in, or is being switched to.
            bool m_state = false;
            // An odd number of toggles is waiting.
            bool m_flip = false;
            // The request event is in SDL's queue.
            bool m_queued = false;
    };

    /// The application's tracker: begun in on_ready(), ended in on_quit().
    [[nodiscard]] fullscreen_tracker& fullscreen();
}
