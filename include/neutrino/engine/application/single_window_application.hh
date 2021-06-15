//
// Created by igor on 13/06/2021.
//

#ifndef NEUTRINO_APPLICATION_SINGLE_WINDOW_APPLICATION_HH
#define NEUTRINO_APPLICATION_SINGLE_WINDOW_APPLICATION_HH

#include <neutrino/engine/application/basic_application.hh>
#include <neutrino/engine/windows/engine_window.hh>
#include <neutrino/sdl/system.hh>
#include <neutrino/engine/observer.hh>

#include <memory>

namespace neutrino::engine {

    template <typename WINDOW, typename Derived, typename ... Events>
    class single_window_application : public basic_application, public observer<Events...> {
    public:
        single_window_application();
        ~single_window_application() override;

        void open (int w, int h);
        void open (int w, int h, int x, int y);

        void open (int w, int h, const std::string& title);
        void open (int w, int h, int x, int y, const std::string& title);
    protected:
        virtual void update(std::chrono::milliseconds ms) = 0;
        virtual void before_open();
        virtual void after_open();

        void toggle_fullscreen();
        [[nodiscard]] std::string title() const;
        void title(const std::string& v);
    private:
        void _init();
        template<typename ... Args>
        void _generic_open(Args&&... args) {
            _init();
            before_open();
            this->m_window->open(std::forward<Args>(args)...);
            after_open();
        }
    private:
        engine_window* m_window;
        sdl::system    m_initializer;
    };
}
// =========================================================================
// Implementation
// =========================================================================
namespace neutrino::engine {


    template <typename WINDOW, typename Derived, typename ... Events>
    single_window_application<WINDOW, Derived, Events...>::single_window_application()
    : m_initializer(sdl::init_flags::VIDEO, sdl::init_flags::AUDIO, sdl::init_flags::EVENTS)
    {
        m_window = new WINDOW();

    }
    // ----------------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived, typename ... Events>
    single_window_application<WINDOW, Derived, Events...>::~single_window_application() {
        delete m_window;
    }
    // ----------------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived, typename ... Events>
    void single_window_application<WINDOW, Derived, Events...>::toggle_fullscreen() {
        m_window->toggle_fullscreen();
    }
    // ----------------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived, typename ... Events>
    std::string single_window_application<WINDOW, Derived, Events...>::title() const {
        return m_window->title();
    }
    // ----------------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived, typename ... Events>
    void single_window_application<WINDOW, Derived, Events...>::title(const std::string& v) {
        m_window->title(v);
    }
    // ----------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived, typename ... Events>
    void single_window_application<WINDOW, Derived, Events...>::_init() {
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
    // ----------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived, typename ... Events>
    void single_window_application<WINDOW, Derived, Events...>::open (int w, int h) {
        _generic_open(w, h);
    }
    // ----------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived, typename ... Events>
    void single_window_application<WINDOW, Derived, Events...>::open (int w, int h, int x, int y) {
        _generic_open(w, h, x, y);
    }
    // ----------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived, typename ... Events>
    void single_window_application<WINDOW, Derived, Events...>::open (int w, int h, const std::string& title) {
        _generic_open(w, h, title);
    }
    // ----------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived, typename ... Events>
    void single_window_application<WINDOW, Derived, Events...>::open (int w, int h, int x, int y, const std::string& title) {
        _generic_open(w, h, x, y, title);
    }
    // ----------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived, typename ... Events>
    void single_window_application<WINDOW, Derived, Events...>::before_open() {
    }
    // ----------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived, typename ... Events>
    void single_window_application<WINDOW, Derived, Events...>::after_open() {
    }
}
#endif
