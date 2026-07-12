//
// Window presentation: render_size(), input mapping, and the on_resize hook.
//
#include <doctest/doctest.h>

#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include <neutrino/application.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/video/globals.hh>
#include <sdlpp/events/events.hh>

namespace {
    using namespace neutrino;

    // Records every on_resize delivery so tests can assert firing/dedup behaviour.
    struct probe_scene : base_scene {
        std::vector <dim> resizes;

        void update_physics(frame_duration) override {}
        void render(frame_duration) override {}
        void handle_action(const sdlpp::event&) override {}
        [[nodiscard]] bool is_opaque() const override { return true; }
        void on_resize(dim render) override { resizes.push_back(render); }
    };

    // application that installs a probe_scene as its first scene and exposes it.
    class probe_app : public application {
        public:
            using application::application;
            probe_scene* probe = nullptr;

        protected:
            std::unique_ptr <base_scene> create_initial_scene() override {
                auto s = std::make_unique <probe_scene>();
                probe = s.get();
                return s;
            }
    };

    // Drives an application through its real SDL lifecycle on the dummy drivers,
    // so get_renderer()/render_size() and the scene pipeline are all live.
    template <typename App>
    class app_harness {
        public:
            explicit app_harness(const application_config& cfg)
                : m_app(cfg) {
                SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER, "dummy", SDL_HINT_OVERRIDE);
                SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "dummy", SDL_HINT_OVERRIDE);
                auto* base = static_cast <sdlpp::abstract_application*>(&m_app);
                base->init_sdl_();
                base->on_init(0, nullptr);
                m_started = true;
            }

            ~app_harness() { shutdown(); }

            app_harness(const app_harness&) = delete;
            app_harness& operator =(const app_harness&) = delete;

            [[nodiscard]] App& app() noexcept { return m_app; }

            // Deliver one synthetic SDL event through the real handle_event path.
            void feed(const SDL_Event& ev) {
                static_cast <sdlpp::abstract_application*>(&m_app)->on_event(sdlpp::event(ev));
            }

            void shutdown() noexcept {
                if (!m_started) {
                    return;
                }
                auto* base = static_cast <sdlpp::abstract_application*>(&m_app);
                base->on_quit();
                base->shutdown_sdl_();
                m_started = false;
            }

        private:
            App m_app;
            bool m_started{false};
    };

    [[nodiscard]] application_config base_config(std::string title) {
        application_config cfg;
        cfg.title = std::move(title);
        cfg.width = 100;
        cfg.height = 100;
        cfg.flags = sdlpp::window_flags::hidden;
        return cfg;
    }

    [[nodiscard]] SDL_Event pixel_size_changed_event() {
        SDL_Event ev{};
        ev.type = SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED;
        return ev;
    }
}

TEST_SUITE("neutrino::video window presentation") {
    TEST_CASE("render_size() is the drawable size in reflow mode") {
        app_harness <application> h(base_config("render_size reflow"));

        const auto out = get_renderer().get_output_size <dim>();
        REQUIRE(out);
        const auto rs = render_size();
        CHECK(rs.width == out->width);
        CHECK(rs.height == out->height);
    }

    TEST_CASE("render_size() is the logical size in logical mode") {
        auto cfg = base_config("render_size logical");
        cfg.logical_size = dim{640, 360};
        cfg.scale = scale_mode::letterbox;
        app_harness <application> h(cfg);

        const auto rs = render_size();
        CHECK(rs.width == 640);
        CHECK(rs.height == 360);
    }

    TEST_CASE("to_render_coords() is the identity in reflow with no DPI scaling") {
        app_harness <application> h(base_config("coords reflow"));

        const auto p = to_render_coords(sdlpp::point <float>{10.0f, 20.0f});
        CHECK(p.x == doctest::Approx(10.0f));
        CHECK(p.y == doctest::Approx(20.0f));
    }

    TEST_CASE("to_window_coords() round-trips to_render_coords() under logical presentation") {
        auto cfg = base_config("coords roundtrip");
        cfg.logical_size = dim{640, 360};
        app_harness <application> h(cfg);

        const sdlpp::point <float> win{40.0f, 45.0f};
        const auto back = to_window_coords(to_render_coords(win));
        CHECK(back.x == doctest::Approx(win.x).epsilon(0.01));
        CHECK(back.y == doctest::Approx(win.y).epsilon(0.01));
    }

    TEST_CASE("on_resize fires once with render_size() when a scene becomes active") {
        app_harness <probe_app> h(base_config("on_resize activation"));

        auto* probe = h.app().probe;
        REQUIRE(probe != nullptr);
        REQUIRE(probe->resizes.size() == 1);
        const auto rs = render_size();
        CHECK(probe->resizes.front().width == rs.width);
        CHECK(probe->resizes.front().height == rs.height);
    }

    TEST_CASE("a pixel-size-changed event re-fires on_resize only when the render space changed") {
        auto cfg = base_config("on_resize event");
        cfg.logical_size = dim{640, 360};
        app_harness <probe_app> h(cfg);

        auto* probe = h.app().probe;
        REQUIRE(probe != nullptr);
        REQUIRE(probe->resizes.size() == 1); // activation

        // Same render space (logical size unchanged): the event dedups to nothing.
        h.feed(pixel_size_changed_event());
        CHECK(probe->resizes.size() == 1);

        // Change the render coordinate space at runtime, then the event delivers it.
        REQUIRE(SDL_SetRenderLogicalPresentation(
            get_renderer().get(), 320, 240, SDL_LOGICAL_PRESENTATION_LETTERBOX));
        h.feed(pixel_size_changed_event());
        REQUIRE(probe->resizes.size() == 2);
        CHECK(probe->resizes.back().width == 320);
        CHECK(probe->resizes.back().height == 240);
    }

    TEST_CASE("a freshly created window reports windowed state") {
        app_harness <application> h(base_config("fullscreen state"));
        CHECK_FALSE(h.app().is_fullscreen());
    }
}
