#pragma once

#include <neutrino/application.hh>

#include <sdlpp/events/events.hh>
#include <SDL3/SDL.h>

#include <memory>
#include <optional>
#include <string>

#include "scene/scene_context_access.hh"
#include "video/painter_state_stack.hh"

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

    /// Runs @p App -- an application constructible from an application_config -- through its
    /// real SDL lifecycle on the dummy drivers: on_init() in the constructor, on_quit() in
    /// shutdown() or the destructor.
    template<typename App>
    class basic_test_application {
        public:
            explicit basic_test_application(std::string title = "Neutrino test application")
                : basic_test_application(make_config(std::move(title))) {
            }

            explicit basic_test_application(const application_config& cfg)
                : m_app(cfg) {
                SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER, "dummy", SDL_HINT_OVERRIDE);
                SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "dummy", SDL_HINT_OVERRIDE);

                auto* app = neutrino::detail::application_access::base(m_app);
                app->init_sdl_();
                app->on_init(0, nullptr);
                m_started = true;
                // Painter calls need a scene current (R48); a test runs as this one.
                m_scene = std::make_unique <idle_scene>();
                m_as_scene.emplace(*neutrino::detail::scene_context_access::painter_state_stack_of(m_scene.get()));
            }

            ~basic_test_application() {
                shutdown();
            }

            basic_test_application(const basic_test_application&) = delete;
            basic_test_application& operator =(const basic_test_application&) = delete;
            basic_test_application(basic_test_application&&) = delete;
            basic_test_application& operator =(basic_test_application&&) = delete;

            [[nodiscard]] App& app() noexcept {
                return m_app;
            }

            [[nodiscard]] const App& app() const noexcept {
                return m_app;
            }

            void iterate() {
                auto* app = neutrino::detail::application_access::base(m_app);
                app->on_iterate();
            }

            /// Ends the harness's scene: no scene is current any more, as in ready(), for the
            /// tests of what fails outside scenes. Pushes it holds are undone.
            void leave_scene() noexcept {
                m_as_scene.reset();
            }

            /// Deliver one synthetic SDL event through the real dispatch: sdlpp's on_event(),
            /// its lifecycle hooks (on_low_memory(), ...), then the application's handle_event().
            void feed(const SDL_Event& ev) {
                neutrino::detail::application_access::base(m_app)->on_event(sdlpp::event(ev));
            }

            void shutdown() noexcept {
                if (!m_started) {
                    return;
                }
                leave_scene();
                m_scene.reset();

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

            App m_app;
            bool m_started{false};
            /// The scene tests run as (R48). Not on the scene stack, so a test that empties the
            /// stack (scenes_manager::finish()) does not end it.
            std::unique_ptr <base_scene> m_scene;
            std::optional <neutrino::detail::painter_state_stack::current_scope> m_as_scene;
    };

    /// The harness most tests use: the stock engine with an idle first scene.
    using test_application = basic_test_application <harness_application>;
}
