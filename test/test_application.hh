#pragma once

#include <neutrino/application.hh>

#include <SDL3/SDL.h>

#include <string>

namespace neutrino::test {
    class test_application {
        public:
            explicit test_application(std::string title = "Neutrino test application")
                : test_application(make_config(std::move(title))) {
            }

            explicit test_application(const application_config& cfg)
                : m_app(cfg) {
                SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER, "dummy", SDL_HINT_OVERRIDE);

                auto* app = static_cast <sdlpp::abstract_application*>(&m_app);
                app->init_sdl_();
                app->on_init(0, nullptr);
                m_started = true;
            }

            ~test_application() {
                shutdown();
            }

            test_application(const test_application&) = delete;
            test_application& operator =(const test_application&) = delete;
            test_application(test_application&&) = delete;
            test_application& operator =(test_application&&) = delete;

            [[nodiscard]] application& app() noexcept {
                return m_app;
            }

            [[nodiscard]] const application& app() const noexcept {
                return m_app;
            }

            void shutdown() noexcept {
                if (!m_started) {
                    return;
                }

                auto* app = static_cast <sdlpp::abstract_application*>(&m_app);
                app->on_quit();
                app->shutdown_sdl_();
                m_started = false;
            }

        private:
            [[nodiscard]] static application_config make_config(std::string title) {
                application_config cfg;
                cfg.title = std::move(title);
                cfg.width = 100;
                cfg.height = 100;
                cfg.flags = sdlpp::window_flags::hidden;
                return cfg;
            }

            application m_app;
            bool m_started{false};
    };
}
