//
// Created by igor on 26/06/2021.
//

#ifndef NEUTRINO_HAL_APPLICATION_HH
#define NEUTRINO_HAL_APPLICATION_HH

#include <chrono>

#include <neutrino/utils/observer.hh>
#include <neutrino/utils/spimpl.h>
#include <neutrino/hal/events/events.hh>
#include <neutrino/hal/events/events_broker.hh>

namespace neutrino::hal
{
    namespace detail
    {
        using application_publisher_t = utils::publisher<events::current_fps>;
    }

    class application : private detail::application_publisher_t {
    public:
        using detail::application_publisher_t::attach;
        using detail::application_publisher_t::detach;
    public:
        application();
        ~application() override;
        void run(int fps);
        void quit();

        events_broker& broker();
    protected:
        virtual void setup();
        virtual void on_terminating ();
        virtual void on_low_memory ();
        virtual void on_will_enter_background ();
        virtual void on_in_background ();
        virtual void on_in_foreground ();
    protected:
        virtual void clear();
        virtual void update(std::chrono::milliseconds ms) = 0;
        virtual void render();
    private:
        void on_quit();
    private:
        void setup_observers();
        bool still_running();
    private:
        struct impl;
    private:
        spimpl::unique_impl_ptr<impl> m_pimpl;
    };
    application* get_application();
}

#endif
