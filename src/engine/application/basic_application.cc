//
// Created by igor on 07/06/2021.
//

#include <neutrino/engine/application/basic_application.hh>
#include <neutrino/sdl/timers.hh>
#include "engine/input_publisher.hh"
#include "engine/windows_manager.hh"

#include <atomic>


namespace neutrino::engine {

    namespace
    {
        basic_application* s_instance = nullptr;
        message_broker* s_message_broker = nullptr;
    }

    basic_application* get_application() {
        return s_instance;
    }

    message_broker* get_message_broker() {
        return s_message_broker;
    }
    // -------------------------------------------------------------------------------
    struct basic_application::impl {
        impl()
        : m_quit_flag(false) {
            s_message_broker = &m_message_broker;
        }

        std::atomic<bool> m_quit_flag;
        input_publisher m_input_publisher;
        message_broker  m_message_broker;
    };
    // -------------------------------------------------------------------------------
    basic_application::basic_application()
    : m_pimpl(new impl)
    {
        setup_observers();
        s_instance = this;
    }
    // -------------------------------------------------------------------------------
    basic_application::~basic_application() = default;
    // -------------------------------------------------------------------------------
    void basic_application::quit()
    {
        on_quit();
    }
    // -------------------------------------------------------------------------------
    void basic_application::run(int desired_fps)
    {
        static uint64_t frames = 0;
        static std::chrono::milliseconds total_time(0);

        unsigned old_fps = 0;

        const auto screen_ms_per_frame = std::chrono::milliseconds(1 + (1000 / desired_fps));
        auto start = sdl::get_ms_since_init();
        while (still_running())
        {
            const auto s = sdl::get_ms_since_init();
            const auto delta_t = s - start;
            start = s;
            {
                m_pimpl->m_input_publisher.run();
                clear();
                update (delta_t);
                render();
            }

            const auto e = sdl::get_ms_since_init();
            const auto ms = e - s;
            frames++;
            if (still_running())
            {
                if (screen_ms_per_frame > ms)
                {
                    const auto delta = screen_ms_per_frame - ms;
                    sdl::delay(delta);
                    total_time += delta;
                } else
                {
                    total_time += ms;
                }

                const unsigned fps = (1000u * frames) / static_cast<unsigned long>(total_time.count());

                if (old_fps == 0) {
                    notify(events::current_fps{fps});
                    old_fps = fps;
                }
                if (frames > 1000u)
                {
                    if (old_fps != fps) {
                        notify(events::current_fps{fps});
                    }

                    frames = 0u;
                    total_time = std::chrono::milliseconds(0);
                }
            }
        }
    }
    // -------------------------------------------------------------------------------
    void basic_application::on_terminating ()
    {

    }
    // -------------------------------------------------------------------------------
    void basic_application::on_low_memory ()
    {

    }
    // -------------------------------------------------------------------------------
    void basic_application::on_will_enter_background ()
    {

    }
    // -------------------------------------------------------------------------------
    void basic_application::on_in_background ()
    {

    }
    // -------------------------------------------------------------------------------
    void basic_application::on_in_foreground ()
    {

    }
    // -------------------------------------------------------------------------------
    void basic_application::clear() {
         windows_manager::instance().clear();
    }
    // -------------------------------------------------------------------------------
    void basic_application::render()
    {
        windows_manager::instance().present();
    }
    // -------------------------------------------------------------------------------
    void basic_application::on_quit()
    {
        m_pimpl->m_quit_flag = true;
    }
    // -------------------------------------------------------------------------------
    bool basic_application::still_running()
    {
        return !m_pimpl->m_quit_flag && windows_manager::instance().has_windows() ;
    }
    // -------------------------------------------------------------------------------
    void basic_application::setup_observers()
    {
        m_pimpl->m_input_publisher.attach(&windows_manager::instance());
        m_pimpl->m_input_publisher.attach(&m_pimpl->m_message_broker);
        m_pimpl->m_input_publisher.attach([this](const sdl::events::terminating&) {on_terminating();});
        m_pimpl->m_input_publisher.attach([this](const sdl::events::low_memory&) {on_low_memory();});
        m_pimpl->m_input_publisher.attach([this](const sdl::events::will_enter_background&) {on_will_enter_background();});
        m_pimpl->m_input_publisher.attach([this](const sdl::events::in_background&) {on_in_background();});
        m_pimpl->m_input_publisher.attach([this](const sdl::events::in_foreground&) {on_in_foreground();});
        m_pimpl->m_input_publisher.attach([this](const sdl::events::quit&) {on_quit();});
    }
}