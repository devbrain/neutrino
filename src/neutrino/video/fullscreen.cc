//
// Created by igor on 25/11/2026.
//

#include "fullscreen.hh"

#include <neutrino/video/globals.hh>
#include <failsafe/enforce.hh>

namespace neutrino {
    namespace detail {
        void fullscreen_tracker::begin(bool actual) {
            const std::lock_guard lock(m_mutex);
            m_running = true;
            m_state = actual;
            m_flip = false;
            m_queued = false;
        }

        void fullscreen_tracker::end() {
            const std::lock_guard lock(m_mutex);
            m_running = false;
            m_flip = false;
            m_queued = false;
        }

        void fullscreen_tracker::toggle(const std::function <void()>& post) {
            const std::lock_guard lock(m_mutex);
            ENFORCE(m_running, "toggle_fullscreen() needs a running application");
            if (!m_queued) {
                post();
                m_queued = true;
            }
            m_flip = !m_flip;
        }

        std::optional <bool> fullscreen_tracker::take() {
            const std::lock_guard lock(m_mutex);
            m_queued = false;
            if (!m_flip) {
                return std::nullopt;
            }
            m_flip = false;
            return !m_state;
        }

        void fullscreen_tracker::track(bool state) {
            const std::lock_guard lock(m_mutex);
            m_state = state;
        }

        fullscreen_tracker& fullscreen() {
            static fullscreen_tracker tracker;
            return tracker;
        }
    }

    void toggle_fullscreen() {
        detail::fullscreen().toggle([] {
            detail::fullscreen_event::push(detail::TOGGLE_FULLSCREEN, nullptr);
        });
    }

    bool is_fullscreen() {
        return get_window().is_fullscreen();
    }
}
