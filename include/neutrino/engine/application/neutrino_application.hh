//
// Created by igor on 21/06/2021.
//

#ifndef NEUTRINO_APPLICATION_NEUTRINO_APPLICATION_HH
#define NEUTRINO_APPLICATION_NEUTRINO_APPLICATION_HH

#include <neutrino/engine/application/basic_application.hh>
#include <neutrino/engine/windows/engine_window.hh>
#include <neutrino/sdl/system.hh>
#include <neutrino/engine/observer.hh>

#include <memory>

namespace neutrino::engine {
    template <typename Derived, typename ... Events>
    class application : public basic_application, public observer<Events...>
    {
    public:
        application()
                : m_initializer(sdl::init_flags::VIDEO, sdl::init_flags::AUDIO, sdl::init_flags::EVENTS) {}

        void open () {
            _generic_open();
        }
        void open (int x, int y) {
            _generic_open(x, y);
        }

        void open (const std::string& title) {
            _generic_open(title);
        }

        void open (int x, int y, const std::string& title) {
            _generic_open(x, y, title);
        }

    protected:
        virtual void before_window_opened() {}
        virtual void after_window_opened() {}

        void toggle_fullscreen() {
            m_window->toggle_fullscreen();
        }

        [[nodiscard]] std::string title() const {
            return m_window->title();
        }

        void title(const std::string& v) {
            m_window->title(v);
        }
    private:
        void _init() {
            if constexpr(sizeof...(Events) == 0) {
                auto f = [this](const neutrino::events::keyboard& ev)
                {
                    if (ev.pressed) {
                        if (ev.code == neutrino::events::scan_code_t::RETURN && (ev.mod & neutrino::events::key_mod_t::ALT)) {
                            this->toggle_fullscreen();
                        }
                        if (ev.code == neutrino::events::scan_code_t::ESCAPE) {
                            this->quit();
                        }
                    }
                };
                m_window->template attach(f);
            } else {
                this->template attach((Derived*)this);
                m_window->template attach((Derived*)this);
            }
        }

        template<typename ... Args>
        void _generic_open(Args&&... args) {
            before_window_opened();
            auto video_system = ((Derived*)this)->video_system();
            m_window = std::make_unique<engine_window>(video_system);
            _init();
            this->m_window->open(video_system->width(), video_system->height(), std::forward<Args>(args)...);
            after_window_opened();
        }
    private:
        std::unique_ptr<engine_window> m_window;
        sdl::system                    m_initializer;
    };
}

#endif
