#include <neutrino/application.hh>
#include <sdlpp/video/display.hh>

namespace neutrino {
    struct application::impl {
        application_config m_cfg;

        explicit impl(const application_config& cfg)
            : m_cfg(cfg) {
        }
    };

    application::application()
        : m_pimpl(std::make_unique <impl>(application_config{})) {
    }

    application::application(const application_config& cfg)
        : m_pimpl(std::make_unique <impl>(cfg)) {
    }

    application::~application() = default;

    sdlpp::window_config application::get_window_config() {
         if (m_pimpl->m_cfg.width <= 0 || m_pimpl->m_cfg.height <= 0) {
             if (auto d = sdlpp::display_manager::get_primary_display()) {
                 if (auto mode = d->get_desktop_mode()) {
                     if (m_pimpl->m_cfg.width <= 0) {
                         m_pimpl->m_cfg.width = static_cast<int>(mode->width);
                     }
                     if (m_pimpl->m_cfg.height <= 0) {
                         m_pimpl->m_cfg.height = static_cast<int>(mode->height);
                     }
                 }
             }
             // Fallbacks if query fails
             if (m_pimpl->m_cfg.width <= 0) m_pimpl->m_cfg.width = 1280;
             if (m_pimpl->m_cfg.height <= 0) m_pimpl->m_cfg.height = 720;
         }

         return {
                m_pimpl->m_cfg.title,
                m_pimpl->m_cfg.width,
                m_pimpl->m_cfg.height,
                m_pimpl->m_cfg.flags,
                m_pimpl->m_cfg.target_fps
            };
    }
}
