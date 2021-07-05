//
// Created by igor on 03/07/2021.
//

#ifndef NEUTRINO_ENGINE_APPLICATION_HH
#define NEUTRINO_ENGINE_APPLICATION_HH

#include <memory>
#include <neutrino/engine/application_monitor.hh>
#include <neutrino/engine/graphics/abstract_context.hh>
#include <neutrino/engine/events.hh>
#include <neutrino/hal/application.hh>
#include <neutrino/hal/video/window.hh>
#include <neutrino/utils/spimpl.h>

namespace neutrino::engine {
    class application : public hal::application, public utils::observer<events::quit> {
    public:
        application(std::unique_ptr<abstract_graphics_context> ctx, std::unique_ptr<application_monitor> monitor, hal::window_flags_t flags);
        application(std::unique_ptr<abstract_graphics_context> ctx, std::unique_ptr<application_monitor> monitor);
    public:
        std::shared_ptr<abstract_graphics_context> graphics_context();
    public:
        void on_event(const events::quit&) override;
    private:
        void on_terminating () override;
        void on_low_memory () override;
        void on_will_enter_background () override;
        void on_in_background () override;
        void on_in_foreground () override;

        void update(std::chrono::milliseconds ms) override;

    private:
        struct impl;
        spimpl::unique_impl_ptr<impl> m_pimpl;
    };
}


#endif

