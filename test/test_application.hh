#pragma once

#include <neutrino/application.hh>

#include <SDL3/SDL.h>

#include <memory>
#include <string>

namespace neutrino::test {
    /// A scene that does nothing. Every application has at least one scene; the harness installs
    /// this one so tests get a live engine without scene behaviour of their own.
    struct idle_scene final : base_scene {
        void fixed_update(sim_duration, const input_snapshot&) override {}
        void render() override {}
    };

    /// The application the harness runs: the stock engine with an idle first scene.
    class harness_application final : public application {
        public:
            using application::application;

        protected:
            std::unique_ptr <base_scene> create_initial_scene() override {
                return std::make_unique <idle_scene>();
            }
    };

    class test_application {
        public:
            explicit test_application(std::string title = "Neutrino test application")
                : test_application(make_config(std::move(title))) {
            }

            explicit test_application(const application_config& cfg)
                : m_app(cfg) {
                SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER, "dummy", SDL_HINT_OVERRIDE);
                SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "dummy", SDL_HINT_OVERRIDE);

                auto* app = neutrino::detail::application_access::base(m_app);
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

            void iterate() {
                auto* app = neutrino::detail::application_access::base(m_app);
                app->on_iterate();
            }

            void shutdown() noexcept {
                if (!m_started) {
                    return;
                }

                auto* app = neutrino::detail::application_access::base(m_app);
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

            harness_application m_app;
            bool m_started{false};
    };
}
