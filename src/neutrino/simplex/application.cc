//
// Created by igor on 18/10/2026.
//

#include <neutrino/simplex/application.hh>
#include "simplex/detail/application_adapter.hh"


namespace neutrino::simplex {
    struct application::impl {
        impl(application* self, int width_px, int height_px, const std::string& title, float scale)
            : m_app(std::make_unique<detail::application_adapter>(self, width_px, height_px, title, scale)) {

        }
        std::unique_ptr<detail::application_adapter> m_app;
    };

    application::application(int width_px, int height_px, const std::string& title, float scale)
        : m_pimpl(std::make_unique<impl>(this, width_px, height_px, title, scale)) {
    }

    application::~application() = default;

    bool application::on_create() {
        return true;
    }

    void application::on_destroy() {
    }

    sdlpp::abstract_application* application::get_application() noexcept {
        return m_pimpl->m_app.get();
    }
}
