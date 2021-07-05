//
// Created by igor on 03/07/2021.
//

#include <neutrino/engine/application.hh>
#include <neutrino/engine/scene_manager.hh>
#include "window.hh"

namespace neutrino::engine {
    struct application::impl {
        impl(std::unique_ptr<abstract_graphics_context> actx, std::unique_ptr<application_monitor> amonitor, hal::window_flags_t flags)
        : ctx(std::move(actx)),
          monitor(std::move(amonitor)),
          main_window(ctx, flags)
        {
            scene_manager::instance().attach(&main_window);
        }

        impl(std::unique_ptr<abstract_graphics_context> actx, std::unique_ptr<application_monitor> amonitor)
        : ctx(std::move(actx)),
          monitor(std::move(amonitor)),
          main_window(ctx)
        {
            scene_manager::instance().attach(&main_window);
        }

        std::shared_ptr<abstract_graphics_context> ctx;
        std::unique_ptr<application_monitor> monitor;
        window main_window;
    };


    application::application(std::unique_ptr<abstract_graphics_context> ctx, std::unique_ptr<application_monitor> monitor, hal::window_flags_t flags)
    : m_pimpl(spimpl::make_unique_impl<impl>(std::move(ctx), std::move(monitor), flags)) {
        scene_manager::instance().attach(this);

        m_pimpl->main_window.show();
    }

    application::application(std::unique_ptr<abstract_graphics_context> ctx, std::unique_ptr<application_monitor> monitor)
    : m_pimpl(spimpl::make_unique_impl<impl>(std::move(ctx), std::move(monitor))) {
        scene_manager::instance().attach(this);
        m_pimpl->main_window.show();
    }

    void application::on_terminating () {
        if (m_pimpl->monitor) {
            m_pimpl->monitor->on_terminating();
        }
    }
    void application::on_low_memory () {
        if (m_pimpl->monitor) {
            m_pimpl->monitor->on_low_memory();
        }
    }
    void application::on_will_enter_background () {
        if (m_pimpl->monitor) {
            m_pimpl->monitor->on_will_enter_background();
        }
    }
    void application::on_in_background () {
        if (m_pimpl->monitor) {
            m_pimpl->monitor->on_in_background();
        }
    }
    void application::on_in_foreground () {
        if (m_pimpl->monitor) {
            m_pimpl->monitor->on_in_foreground();
        }
    }

    void application::update(std::chrono::milliseconds ms) {
        scene_manager ::instance().update(ms);
    }

    void application::on_event(const events::quit&) {
        this->quit();
    }

    std::shared_ptr<abstract_graphics_context> application::graphics_context() {
        return m_pimpl->ctx;
    }
}
