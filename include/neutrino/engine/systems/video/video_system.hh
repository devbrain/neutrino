//
// Created by igor on 19/06/2021.
//

#ifndef NEUTRINO_SYSTEMS_VIDEO_VIDEO_SYSTEM_HH
#define NEUTRINO_SYSTEMS_VIDEO_VIDEO_SYSTEM_HH

#include <neutrino/utils/observer.hh>
#include <neutrino/engine/events/events.hh>
#include <neutrino/engine/windows/basic_window.hh>

namespace neutrino::engine::systems::video {

    template <typename Renderer>
    class video_system : public Renderer, public observer<events::window_resized> {
    public:
        virtual ~video_system() = default;

        video_system(int width, int height, bool resizable)
        : m_width(width), m_height(height), m_resizable(resizable) {}

        [[nodiscard]] int width() const noexcept {
            return m_width;
        }

        [[nodiscard]] int height() const noexcept {
            return m_height;
        }

        template<typename Publisher>
        void attach(Publisher& publisher) {
            if (m_resizable) {
                publisher.attach(this);
            }
        }
    private:
        void on_event(const events::window_resized& e) override {
            m_width = e.w;
            m_height = e.h;
            handle_resize();
        }
    protected:
        virtual void handle_resize() {}
    private:
        int m_width;
        int m_height;
        const bool m_resizable;
    };
}

#endif
