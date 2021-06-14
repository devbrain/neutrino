//
// Created by igor on 07/06/2021.
//

#ifndef NEUTRINO_ENGINE_APPLICATION_BASIC_APPLICATION_HH
#define NEUTRINO_ENGINE_APPLICATION_BASIC_APPLICATION_HH

#include <neutrino/engine/observer.hh>
#include <neutrino/engine/message_broker.hh>
#include <neutrino/engine/events/events.hh>
#include <chrono>
#include <memory>


namespace neutrino::engine
{
    namespace detail
    {
        using application_publisher_t = publisher<events::current_fps>;
    }

    class basic_application : private detail::application_publisher_t {
    public:
        using detail::application_publisher_t::attach;
        using detail::application_publisher_t::detach;
    public:
        basic_application();
        ~basic_application() override;
        void run(int fps);
        void quit();
    protected:
        virtual void on_terminating ();
        virtual void on_low_memory ();
        virtual void on_will_enter_background ();
        virtual void on_in_background ();
        virtual void on_in_foreground ();
    protected:
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
        std::unique_ptr<impl> m_pimpl;
    };

    basic_application* get_application();
    message_broker* get_message_broker();
}

#endif //NEUTRINO_APPLICATION_HH
