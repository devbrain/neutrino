#include <neutrino/application.hh>
#include <sdlpp/video/display.hh>
#include <failsafe/enforce.hh>
#include <failsafe/logger/backend/sdl_backend.hh>

#include <musac_backends/sdl3/sdl3_backend.hh>
#include <musac/audio_device.hh>
#include <musac/audio_source.hh>
#include <musac/audio_system.hh>
#include <musac/stream.hh>
#include <musac/error.hh>
#include <musac/codecs/register_codecs.hh>

namespace neutrino {
    static application* g_app = nullptr;

    struct application::impl {
        application_config m_cfg;
        sdlpp::renderer* m_renderer = nullptr;
        sdlpp::window* m_window = nullptr;

        std::shared_ptr <musac::audio_backend> m_audio_backend;
        std::shared_ptr<musac::decoders_registry> m_musac_codecs;

        explicit impl(const application_config& cfg)
            : m_cfg(cfg) {
            m_audio_backend = musac::create_sdl3_backend();
            m_musac_codecs = musac::create_registry_with_all_codecs();

            if (!musac::audio_system::init(m_audio_backend, m_musac_codecs)) {
                THROW_RUNTIME("Failed to initialize audio system");
            }
        }
    };

    application::application()
        : m_pimpl(std::make_unique <impl>(application_config{})) {
        ENFORCE(!g_app)("neutrino::application was already initialized");
        g_app = this;
    }

    application::application(const application_config& cfg)
        : m_pimpl(std::make_unique <impl>(cfg)) {
        ENFORCE(!g_app)("neutrino::application was already initialized");
        g_app = this;
    }

    application::~application() {
        musac::audio_system::done();
        g_app = nullptr;
    }

    sdlpp::window_config application::get_window_config() {
        if (m_pimpl->m_cfg.width <= 0 || m_pimpl->m_cfg.height <= 0) {
            if (auto d = sdlpp::display_manager::get_primary_display()) {
                if (auto mode = d->get_desktop_mode()) {
                    if (m_pimpl->m_cfg.width <= 0) {
                        m_pimpl->m_cfg.width = static_cast <int>(mode->width);
                    }
                    if (m_pimpl->m_cfg.height <= 0) {
                        m_pimpl->m_cfg.height = static_cast <int>(mode->height);
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

    void application::on_ready() {
        game_application::on_ready();
        failsafe::logger::set_backend(failsafe::logger::backends::make_sdl_backend());
        m_pimpl->m_renderer = &get_renderer();
        m_pimpl->m_window = &get_window();
    }
}
