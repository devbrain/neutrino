//
// Created by igor on 13/06/2021.
//

#ifndef NEUTRINO_APPLICATION_SINGLE_WINDOW_APPLICATION_HH
#define NEUTRINO_APPLICATION_SINGLE_WINDOW_APPLICATION_HH

#include <neutrino/utils/mp/introspection.hh>
#include <neutrino/utils/mp/constexpr_for.hh>
#include <neutrino/engine/application/basic_application.hh>
#include <neutrino/engine/windows/engine_window.hh>
#include <neutrino/sdl/system.hh>

#include <memory>

namespace neutrino::engine {

    template <typename WINDOW, typename Derived>
    class single_window_application : public basic_application {
    public:
        single_window_application();
        ~single_window_application() override;

        void open (int w, int h);
        void open (int w, int h, int x, int y);

        void open (int w, int h, const std::string& title);
        void open (int w, int h, int x, int y, const std::string& title);
    protected:
        virtual void update(std::chrono::milliseconds ms) = 0;
        void toggle_fullscreen();
        [[nodiscard]] std::string title() const;
        void title(const std::string& v);
    private:
        void on_terminating () override;
        void on_low_memory () override;
        void on_will_enter_background () override;
        void on_in_background () override;
        void on_in_foreground () override;


        void init();
        void _before_open();
        void _after_open();
    private:
        engine_window* m_window;
        sdl::system    m_initializer;
    };
}
// =========================================================================
// Implementation
// =========================================================================
namespace neutrino::engine {
    namespace detail {
        GENERATE_HAS_MEMBER_FUNCTION(on_event);
        GENERATE_HAS_MEMBER_FUNCTION(handle_terminating);
        GENERATE_HAS_MEMBER_FUNCTION(handle_low_memory);
        GENERATE_HAS_MEMBER_FUNCTION(handle_will_enter_background);
        GENERATE_HAS_MEMBER_FUNCTION(handle_in_background);
        GENERATE_HAS_MEMBER_FUNCTION(handle_in_foreground);
        GENERATE_HAS_MEMBER_FUNCTION(pre_open);
        GENERATE_HAS_MEMBER_FUNCTION(post_open);
    }

    template <typename WINDOW, typename Derived>
    single_window_application<WINDOW, Derived>::single_window_application()
    : m_initializer(sdl::init_flags::VIDEO, sdl::init_flags::AUDIO, sdl::init_flags::EVENTS)
    {
        m_window = new WINDOW();
        init ();
    }
    // ----------------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived>
    single_window_application<WINDOW, Derived>::~single_window_application() {
        delete m_window;
    }
    // ----------------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived>
    void single_window_application<WINDOW, Derived>::toggle_fullscreen() {
        m_window->toggle_fullscreen();
    }
    // ----------------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived>
    std::string single_window_application<WINDOW, Derived>::title() const {
        return m_window->title();
    }
    // ----------------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived>
    void single_window_application<WINDOW, Derived>::title(const std::string& v) {
        m_window->title(v);
    }
    // ----------------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived>
    void single_window_application<WINDOW, Derived>::on_terminating () {
        if constexpr(detail::has_handle_terminating<Derived, void()>::value) {
            static_cast<Derived*>(this)->handle_terminating();
        }
    }
    // ----------------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived>
    void single_window_application<WINDOW, Derived>::on_low_memory () {
        if constexpr(detail::has_handle_low_memory<Derived, void()>::value) {
            static_cast<Derived*>(this)->handle_low_memory();
        }
    }
    // ----------------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived>
    void single_window_application<WINDOW, Derived>::on_will_enter_background () {
        if constexpr(detail::has_handle_will_enter_background<Derived, void()>::value) {
            static_cast<Derived*>(this)->handle_will_enter_background();
        }
    }
    // ----------------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived>
    void single_window_application<WINDOW, Derived>::on_in_background () {
        if constexpr(detail::has_handle_in_background<Derived, void()>::value) {
            static_cast<Derived*>(this)->handle_in_background();
        }
    }
    // ----------------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived>
    void single_window_application<WINDOW, Derived>::on_in_foreground () {
        if constexpr(detail::has_handle_in_foreground<Derived, void()>::value) {
            static_cast<Derived*>(this)->handle_in_foreground();
        }
    }
    // ----------------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived>
    void single_window_application<WINDOW, Derived>::init() {
        static constexpr auto n = events::all_events_t::size();
        mp::constexpr_for<0, n, 1> ([this](auto i) {
            constexpr size_t idx = i.value;
            using ev_t = mp::type_list_at_t<idx, events::all_events_t>;

            if constexpr(detail::has_on_event<Derived, void(const ev_t&)>::value) {
                auto f = [this](const ev_t& e) {
                    static_cast<Derived*>(this)->on_event(e);
                };

                m_window->attach(f);
            }
        });
    }
    // ----------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived>
    void single_window_application<WINDOW, Derived>::open (int w, int h) {
        _before_open();
        m_window->open(w, h);
        _after_open();
    }
    // ----------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived>
    void single_window_application<WINDOW, Derived>::open (int w, int h, int x, int y) {
        _before_open();
        m_window->open(w, h, x, y);
        _after_open();
    }
    // ----------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived>
    void single_window_application<WINDOW, Derived>::open (int w, int h, const std::string& title) {
        _before_open();
        m_window->open(w, h, title);
        _after_open();
    }
    // ----------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived>
    void single_window_application<WINDOW, Derived>::open (int w, int h, int x, int y, const std::string& title) {
        _before_open();
        m_window->open(w, h, x, y, title);
        _after_open();
    }
    // ----------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived>
    void single_window_application<WINDOW, Derived>::_before_open() {
        if constexpr(detail::has_pre_open<Derived,void()>::value) {
            static_cast<Derived*>(this)->pre_open ();
        }
    }
    // ----------------------------------------------------------------------------------
    template <typename WINDOW, typename Derived>
    void single_window_application<WINDOW, Derived>::_after_open() {
        if constexpr(detail::has_pre_open<Derived,void()>::value) {
            static_cast<Derived*>(this)->post_open ();
        }
    }
}
#endif
