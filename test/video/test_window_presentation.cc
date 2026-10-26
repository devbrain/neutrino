//
// Application frame hooks, input snapshots, and window presentation.
//
#include <doctest/doctest.h>

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include <neutrino/application.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/scene/scene_transitions.hh>
#include <neutrino/video/globals.hh>
#include <sdlpp/events/events.hh>
#include <sdlpp/video/texture.hh>

#include "services/service_locator.hh"

namespace {
    using namespace neutrino;

    // Records every on_resize delivery so tests can assert firing/dedup behaviour.
    struct probe_scene : base_scene {
        std::vector <dim> resizes;
        std::function<void()> draw;
        std::function<void(sim_duration, const input_snapshot&)> tick;
        std::function<void(const sdlpp::event&)> action;
        std::function<void(dim)> resized;
        std::function<void()> entered;
        std::function<void()> destroyed;

        ~probe_scene() override { if (destroyed) destroyed(); }
        void on_enter() override { if (entered) entered(); }
        void fixed_update(sim_duration dt, const input_snapshot& input) override {
            if (tick) tick(dt, input);
        }
        void render() override { if (draw) draw(); }
        void handle_action(const sdlpp::event& e) override { if (action) action(e); }
        [[nodiscard]] bool is_opaque() const override { return true; }
        void on_resize(dim render) override {
            resizes.push_back(render);
            if (resized) resized(render);
        }
    };

    class render_probe_app : public application {
        public:
            using application::application;
            std::function<void(sdlpp::renderer&)> draw;
            int updates = 0;
            std::function<void(float, const input_snapshot&)> tick;

        protected:
            void update_scene(float dt, const input_snapshot& input) override {
                ++updates;
                if (tick) tick(dt, input);
            }
            void render_scene() override { if (draw) draw(get_renderer()); }
    };

    class event_probe_app : public render_probe_app {
        public:
            using render_probe_app::render_probe_app;
            std::function<void(const sdlpp::event&)> dispatch;
            std::function<void(dim)> resized;
            bool forward_events = false;
            bool forward_resizes = false;

        protected:
            void event(const sdlpp::event& e) override {
                if (dispatch) dispatch(e);
                if (forward_events) application::event(e);
            }
            void resize_scene(dim size) override {
                if (resized) resized(size);
                if (forward_resizes) application::resize_scene(size);
            }
    };

    // application that installs a probe_scene as its first scene and exposes it.
    template <typename Base>
    class scene_probe_app : public Base {
        public:
            using Base::Base;
            probe_scene* probe = nullptr;

        protected:
            std::unique_ptr <base_scene> create_initial_scene() override {
                auto s = std::make_unique <probe_scene>();
                probe = s.get();
                return s;
            }
    };

    using probe_app = scene_probe_app<application>;
    using custom_scene_app = scene_probe_app<render_probe_app>;
    using event_probe_scene_app = scene_probe_app<event_probe_app>;

    class selective_render_app : public probe_app {
        public:
            using probe_app::probe_app;
            int scene_render_passes = 0;

        protected:
            void render_scene() override {
                for (int i = 0; i < scene_render_passes; ++i) {
                    application::render_scene();
                }
            }
    };

    // Drive the real final hooks with an exact delta, independent of wall-clock timing.
    struct frame_driver : sdlpp::game_application {
        static void frame(sdlpp::game_application& app, float dt) {
            const auto tick = &frame_driver::on_update;
            const auto draw = &frame_driver::on_render;
            (app.*tick)(dt);
            (app.*draw)(neutrino::get_renderer());
        }

        // The live (frame-level) input state of the SDL base. Not engine API -- application
        // inherits it privately -- but the event tests check it is updated before event() runs.
        static sdlpp::button_state key(sdlpp::game_application& app, sdlpp::scancode scan) {
            const auto get = &frame_driver::get_key;
            return (app.*get)(scan);
        }

        static sdlpp::button_state mouse(sdlpp::game_application& app, sdlpp::mouse_button button) {
            const auto get = static_cast <sdlpp::button_state (sdlpp::game_application::*)(sdlpp::mouse_button) const noexcept>(
                &frame_driver::get_mouse);
            return (app.*get)(button);
        }
    };

    template <typename App>
    [[nodiscard]] sdlpp::game_application& sdl_base(App& app) {
        return static_cast <sdlpp::game_application&>(*neutrino::detail::application_access::base(app));
    }

    // Drives an application through its real SDL lifecycle on the dummy drivers,
    // so get_renderer()/render_size() and the scene pipeline are all live.
    template <typename App>
    class app_harness {
        public:
            explicit app_harness(const application_config& cfg)
                : m_app(cfg) {
                SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER, "dummy", SDL_HINT_OVERRIDE);
                SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "dummy", SDL_HINT_OVERRIDE);
                auto* base = neutrino::detail::application_access::base(m_app);
                base->init_sdl_();
                base->on_init(0, nullptr);
                m_started = true;
            }

            ~app_harness() { shutdown(); }

            app_harness(const app_harness&) = delete;
            app_harness& operator =(const app_harness&) = delete;

            [[nodiscard]] App& app() noexcept { return m_app; }

            void iterate() {
                neutrino::detail::application_access::base(m_app)->on_iterate();
            }

            void frame(float dt) { frame_driver::frame(sdl_base(m_app), dt); }

            // Deliver one synthetic SDL event through the real handle_event path.
            void feed(const SDL_Event& ev) {
                neutrino::detail::application_access::base(m_app)->on_event(sdlpp::event(ev));
            }

            void shutdown() noexcept {
                if (!m_started) {
                    return;
                }
                auto* base = neutrino::detail::application_access::base(m_app);
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

    void check_pixel(sdlpp::renderer& renderer, int x, int y, const sdlpp::color& expected) {
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> pixels(
            SDL_RenderReadPixels(renderer.get(), nullptr), SDL_DestroySurface);
        REQUIRE(pixels);
        Uint8 r = 0, g = 0, b = 0, a = 0;
        REQUIRE(SDL_ReadSurfacePixel(pixels.get(), x, y, &r, &g, &b, &a));
        CHECK(r == expected.r);
        CHECK(g == expected.g);
        CHECK(b == expected.b);
    }

    [[nodiscard]] SDL_Event take_transition_event() {
        SDL_Event ev{};
        REQUIRE(SDL_PeepEvents(&ev, 1, SDL_GETEVENT, SDL_EVENT_USER, SDL_EVENT_LAST) == 1);
        return ev;
    }

    [[nodiscard]] SDL_Event pixel_size_changed_event() {
        SDL_Event ev{};
        ev.type = SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED;
        return ev;
    }
}

TEST_SUITE("neutrino::video window presentation") {
    TEST_CASE("render_scene override runs after clear without scenes") {
        app_harness<render_probe_app> h(base_config("application drawing"));
        auto& renderer = get_renderer();
        auto output = sdlpp::texture::create(renderer, sdlpp::pixel_format_enum::RGBA8888,
                                             sdlpp::texture_access::target, 16, 16);
        REQUIRE(output);
        // Capture a target texture: window contents are undefined after present().
        const sdlpp::renderer::target_guard guard(renderer, *output);
        REQUIRE(renderer.set_draw_color(sdlpp::colors::green));
        REQUIRE(renderer.clear());
        int renders = 0;
        h.app().draw = [&](sdlpp::renderer& r) {
            ++renders;
            CHECK(&r == &renderer);
            check_pixel(r, 0, 0, sdlpp::colors::black);
            REQUIRE(r.set_draw_color(sdlpp::colors::red));
            REQUIRE(r.draw_point(1, 1));
        };
        for (int frame = 1; frame <= 2; ++frame) {
            h.iterate();
            CHECK(h.app().updates == frame);
            CHECK(renders == frame);
            check_pixel(renderer, 1, 1, sdlpp::colors::red);
        }
    }

    TEST_CASE("default render_scene draws the scene stack after clear") {
        app_harness<probe_app> h(base_config("default scene drawing"));
        auto& renderer = get_renderer();
        auto output = sdlpp::texture::create(renderer, sdlpp::pixel_format_enum::RGBA8888,
                                             sdlpp::texture_access::target, 16, 16);
        REQUIRE(output);
        const sdlpp::renderer::target_guard guard(renderer, *output);
        REQUIRE(renderer.set_draw_color(sdlpp::colors::green));
        REQUIRE(renderer.clear());
        REQUIRE(h.app().probe);
        int renders = 0;
        h.app().probe->draw = [&] {
            ++renders;
            check_pixel(renderer, 0, 0, sdlpp::colors::black);
            REQUIRE(renderer.set_draw_color(sdlpp::colors::blue));
            REQUIRE(renderer.draw_point(3, 3));
        };
        h.iterate();
        CHECK(renders == 1);
        check_pixel(renderer, 3, 3, sdlpp::colors::blue);
    }

    TEST_CASE("render_scene override replaces scene-stack rendering") {
        app_harness<custom_scene_app> h(base_config("custom scene drawing"));
        auto& renderer = get_renderer();
        auto output = sdlpp::texture::create(renderer, sdlpp::pixel_format_enum::RGBA8888,
                                             sdlpp::texture_access::target, 16, 16);
        REQUIRE(output);
        const sdlpp::renderer::target_guard guard(renderer, *output);
        int renders = 0;
        h.app().draw = [&](sdlpp::renderer& r) {
            ++renders;
            REQUIRE(r.set_draw_color(sdlpp::colors::red));
            REQUIRE(r.draw_point(1, 1));
        };
        bool scene_rendered = false;
        REQUIRE(h.app().probe);
        h.app().probe->draw = [&] { scene_rendered = true; };
        h.iterate();
        CHECK(renders == 1);
        CHECK_FALSE(scene_rendered);
        check_pixel(renderer, 1, 1, sdlpp::colors::red);
    }

    TEST_CASE("render_scene override failures propagate without running scene rendering") {
        app_harness<custom_scene_app> h(base_config("custom scene render failure"));
        bool scene_rendered = false;
        REQUIRE(h.app().probe);
        h.app().probe->draw = [&] { scene_rendered = true; };
        h.app().draw = [](sdlpp::renderer&) { throw std::runtime_error("render failed"); };
        CHECK_THROWS_AS(h.iterate(), std::runtime_error);
        CHECK_FALSE(scene_rendered);
    }

    TEST_CASE("default render_scene contains scene render failures") {
        app_harness<probe_app> h(base_config("default scene render failure"));
        REQUIRE(h.app().probe);
        int renders = 0;
        h.app().probe->draw = [&] {
            ++renders;
            throw std::runtime_error("scene render failed");
        };
        CHECK_NOTHROW(h.iterate());
        CHECK(renders == 1);
    }

    TEST_CASE("custom rendering preserves gamepad edges until frame completion") {
        // Replacing scene event dispatch must preserve gamepad tracking and edge cleanup.
        app_harness<event_probe_app> h(base_config("custom render input bookkeeping"));
        SDL_VirtualJoystickDesc desc{};
        SDL_INIT_INTERFACE(&desc);
        desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
        desc.nbuttons = 1;
        desc.button_mask = 1u << SDL_GAMEPAD_BUTTON_SOUTH;
        desc.name = "render bookkeeping test gamepad";
        struct virtual_gamepad {
            SDL_JoystickID id;
            ~virtual_gamepad() { if (id) SDL_DetachVirtualJoystick(id); }
        } pad{SDL_AttachVirtualJoystick(&desc)};
        REQUIRE(pad.id != 0);
        SDL_Event added{};
        added.type = SDL_EVENT_GAMEPAD_ADDED;
        added.gdevice.which = pad.id;
        h.feed(added);
        const auto* gamepads = service_locator::instance().get_gamepads();
        REQUIRE(gamepads);
        REQUIRE(gamepads->is_gamepad_connected(0));
        const auto state = [&] {
            return gamepads->get_gamepad_button_state(0, sdlpp::gamepad_button::south);
        };
        SDL_Event button{};
        button.type = SDL_EVENT_GAMEPAD_BUTTON_DOWN;
        button.gbutton.which = pad.id;
        button.gbutton.button = SDL_GAMEPAD_BUTTON_SOUTH;
        button.gbutton.down = true;
        h.feed(button);
        int renders = 0;
        h.app().draw = [&](sdlpp::renderer&) {
            ++renders;
            CHECK(state().pressed);
            CHECK_FALSE(state().released);
            CHECK(state().held);
        };
        h.iterate();
        CHECK(renders == 1);
        CHECK_FALSE(state().pressed);
        CHECK_FALSE(state().released);
        CHECK(state().held);

        button.type = SDL_EVENT_GAMEPAD_BUTTON_UP;
        button.gbutton.down = false;
        h.feed(button);
        h.app().draw = [&](sdlpp::renderer&) {
            ++renders;
            CHECK_FALSE(state().pressed);
            CHECK(state().released);
            CHECK_FALSE(state().held);
        };
        h.iterate();
        CHECK(renders == 2);
        CHECK_FALSE(state().pressed);
        CHECK_FALSE(state().released);
        CHECK_FALSE(state().held);
    }

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

TEST_SUITE("neutrino::application frame updates") {
    TEST_CASE("default update_scene preserves fixed steps and delivers edges only once") {
        auto cfg = base_config("default scene updates");
        cfg.fixed.period = sim_duration{0.125f};
        cfg.fixed.max_frame = sim_duration{0.5f};
        app_harness<probe_app> h(cfg);
        REQUIRE(h.app().probe);
        std::vector<input_snapshot> inputs;
        h.app().probe->tick = [&](sim_duration dt, const input_snapshot& input) {
            CHECK(dt.count() == doctest::Approx(0.125f));
            inputs.push_back(input);
        };
        h.frame(0.0625f);
        CHECK(inputs.empty());
        SDL_Event key{};
        key.type = SDL_EVENT_KEY_DOWN;
        key.key.scancode = SDL_SCANCODE_SPACE;
        key.key.down = true;
        h.feed(key);
        SDL_Event wheel{};
        wheel.type = SDL_EVENT_MOUSE_WHEEL;
        wheel.wheel.y = 2.0f;
        h.feed(wheel);
        h.frame(0.3125f);
        REQUIRE(inputs.size() == 3);
        for (std::size_t step = 0; step < inputs.size(); ++step) {
            CHECK(inputs[step].pressed(sdlpp::scancode::space) == (step == 0));
            CHECK(inputs[step].held(sdlpp::scancode::space));
            CHECK(inputs[step].wheel() == (step == 0 ? 2 : 0));
        }
        h.frame(0.0f);
        CHECK(inputs.size() == 3);
    }

    TEST_CASE("update_scene override receives frame input and replaces the scheduler") {
        app_harness<custom_scene_app> h(base_config("custom scene updates"));
        REQUIRE(h.app().probe);
        int scene_updates = 0;
        h.app().probe->tick = [&](sim_duration, const input_snapshot&) { ++scene_updates; };
        SDL_Event key{};
        key.type = SDL_EVENT_KEY_DOWN;
        key.key.scancode = SDL_SCANCODE_SPACE;
        key.key.down = true;
        h.feed(key);
        std::vector<float> deltas;
        h.app().tick = [&](float dt, const input_snapshot& input) {
            deltas.push_back(dt);
            CHECK(input.pressed(sdlpp::scancode::space));
            CHECK(input.held(sdlpp::scancode::space));
        };
        // Direct hook driving does not run SDL++'s keyboard-edge cleanup between frames.
        h.frame(0.0f);
        h.frame(0.5f);
        REQUIRE(deltas.size() == 2);
        CHECK(deltas[0] == doctest::Approx(0.0f));
        CHECK(deltas[1] == doctest::Approx(0.5f));
        CHECK(scene_updates == 0);
    }

    TEST_CASE("custom update failures propagate to the entry point") {
        app_harness<custom_scene_app> h(base_config("custom update failure"));
        REQUIRE(h.app().probe);
        int scene_updates = 0;
        h.app().probe->tick = [&](sim_duration, const input_snapshot&) { ++scene_updates; };
        h.app().tick = [](float, const input_snapshot&) { throw std::runtime_error("update failed"); };
        CHECK_THROWS_AS(h.frame(0.05f), std::runtime_error);
        CHECK(scene_updates == 0);
    }

    TEST_CASE("default update_scene contains scene failures and suppresses that frame's render") {
        auto cfg = base_config("default update failure");
        cfg.fixed.period = sim_duration{0.125f};
        app_harness<probe_app> h(cfg);
        REQUIRE(h.app().probe);
        int updates = 0;
        int renders = 0;
        h.app().probe->tick = [&](sim_duration, const input_snapshot&) {
            ++updates;
            throw std::runtime_error("scene update failed");
        };
        h.app().probe->draw = [&] { ++renders; };
        CHECK_NOTHROW(h.frame(0.125f));
        CHECK(updates == 1);
        CHECK(renders == 0);
    }
}


TEST_SUITE("neutrino::application event hooks") {
    TEST_CASE("default event dispatch reaches scenes and contains scene errors") {
        app_harness<probe_app> h(base_config("default scene events"));
        int calls = 0;
        h.app().probe->action = [&](const sdlpp::event& e) {
            CHECK(e.type() == sdlpp::event_type::key_down);
            ++calls;
            throw std::runtime_error("scene event failed");
        };
        SDL_Event key{};
        key.type = SDL_EVENT_KEY_DOWN;
        key.key.scancode = SDL_SCANCODE_SPACE;
        key.key.down = true;
        CHECK_NOTHROW(h.feed(key));
        CHECK(calls == 1);
        CHECK(frame_driver::key(sdl_base(h.app()), sdlpp::scancode::space).pressed);
    }

    TEST_CASE("event overrides replace scene dispatch after input tracking") {
        app_harness<event_probe_scene_app> h(base_config("custom scene events"));
        int scene_events = 0;
        int app_events = 0;
        h.app().probe->action = [&](const sdlpp::event&) { ++scene_events; };
        h.app().dispatch = [&](const sdlpp::event& e) {
            ++app_events;
            if (e.type() == sdlpp::event_type::key_down) {
                CHECK(frame_driver::key(sdl_base(h.app()), sdlpp::scancode::space).pressed == (app_events == 1));
                CHECK(frame_driver::key(sdl_base(h.app()), sdlpp::scancode::space).held);
            } else if (e.type() == sdlpp::event_type::mouse_button_down) {
                CHECK(frame_driver::mouse(sdl_base(h.app()), sdlpp::mouse_button::left).pressed);
            }
        };
        SDL_Event key{};
        key.type = SDL_EVENT_KEY_DOWN;
        key.key.scancode = SDL_SCANCODE_SPACE;
        key.key.down = true;
        h.feed(key);
        SDL_Event mouse{};
        mouse.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        mouse.button.button = SDL_BUTTON_LEFT;
        mouse.button.down = true;
        h.feed(mouse);
        CHECK(app_events == 2);
        CHECK(scene_events == 0);

        h.app().forward_events = true;
        h.feed(key);
        CHECK(app_events == 3);
        CHECK(scene_events == 1);
    }

    TEST_CASE("event override exceptions propagate without dispatching scenes") {
        app_harness<event_probe_scene_app> h(base_config("custom event failure"));
        int scene_events = 0;
        h.app().probe->action = [&](const sdlpp::event&) { ++scene_events; };
        h.app().dispatch = [](const sdlpp::event&) { throw std::runtime_error("event failed"); };
        CHECK_THROWS_AS(h.feed(pixel_size_changed_event()), std::runtime_error);
        CHECK(scene_events == 0);
    }

    TEST_CASE("default event dispatch applies queued push replace and pop") {
        int entered = 0;
        int destroyed = 0;
        app_harness<probe_app> h(base_config("scene transition ownership"));
        const auto scene = [&] {
            auto s = std::make_unique<probe_scene>();
            s->entered = [&] { ++entered; };
            s->destroyed = [&] { ++destroyed; };
            return s;
        };
        push_scene(scene());
        h.feed(take_transition_event());
        CHECK(entered == 1);
        CHECK(destroyed == 0);
        replace_scene(scene());
        h.feed(take_transition_event());
        CHECK(entered == 2);
        CHECK(destroyed == 1);
        pop_scene();
        h.feed(take_transition_event());
        CHECK(destroyed == 2);
        h.shutdown();
        CHECK(destroyed == 2);
    }

    TEST_CASE("intercepted scene transitions are reclaimed at shutdown") {
        int entered = 0;
        int destroyed = 0;
        app_harness<event_probe_scene_app> h(base_config("ignored scene transitions"));
        bool throws = false;
        bool deliver = true;
        SUBCASE("requests remain queued") { deliver = false; }
        SUBCASE("override ignores transitions") {}
        SUBCASE("override throws") {
            throws = true;
            h.app().dispatch = [](const sdlpp::event&) { throw std::runtime_error("event failed"); };
        }
        const auto scene = [&] {
            auto s = std::make_unique<probe_scene>();
            s->entered = [&] { ++entered; };
            s->destroyed = [&] { ++destroyed; };
            return s;
        };
        push_scene(scene());
        replace_scene(scene());
        for (int i = 0; deliver && i < 2; ++i) {
            const auto e = take_transition_event();
            if (throws) {
                CHECK_THROWS_AS(h.feed(e), std::runtime_error);
            } else {
                CHECK_NOTHROW(h.feed(e));
            }
        }
        CHECK(entered == 0);
        CHECK(destroyed == 0);
        h.shutdown();
        CHECK(destroyed == 2);
    }

    TEST_CASE("rejected transition events release their scenes immediately") {
        int destroyed = 0;
        app_harness<probe_app> h(base_config("rejected scene transitions"));
        {
            struct event_filter_guard {
                SDL_EventFilter previous = nullptr;
                void* data = nullptr;
                event_filter_guard() {
                    SDL_GetEventFilter(&previous, &data);
                    SDL_SetEventFilter([](void*, SDL_Event* e) { return e->type < SDL_EVENT_USER; }, nullptr);
                }
                ~event_filter_guard() { SDL_SetEventFilter(previous, data); }
            } filter;
            const auto scene = [&] {
                auto s = std::make_unique<probe_scene>();
                s->destroyed = [&] { ++destroyed; };
                return s;
            };
            CHECK_THROWS(push_scene(scene()));
            CHECK(destroyed == 1);
            CHECK_THROWS(replace_scene(scene()));
            CHECK(destroyed == 2);
        }
        h.shutdown();
        CHECK(destroyed == 2);
    }

    TEST_CASE("resize overrides replace scene notification and can delegate explicitly") {
        app_harness<event_probe_scene_app> h(base_config("custom scene resize"));
        std::vector<dim> sizes;
        std::vector<std::string> order;
        h.app().resized = [&](dim size) { sizes.push_back(size); order.push_back("resize"); };
        h.app().dispatch = [&](const sdlpp::event&) { order.push_back("event"); };
        REQUIRE(h.app().probe->resizes.size() == 1); // Initial activation belongs to the scene.
        REQUIRE(get_renderer().set_logical_presentation(320, 200, sdlpp::logical_presentation::letterbox));
        h.feed(pixel_size_changed_event());
        h.feed(pixel_size_changed_event());
        REQUIRE(sizes.size() == 1);
        CHECK(sizes[0].width == 320);
        CHECK(sizes[0].height == 200);
        CHECK(order == std::vector<std::string>{"resize", "event", "event"});
        CHECK(h.app().probe->resizes.size() == 1);

        h.app().forward_resizes = true;
        REQUIRE(get_renderer().set_logical_presentation(640, 400, sdlpp::logical_presentation::letterbox));
        h.feed(pixel_size_changed_event());
        REQUIRE(h.app().probe->resizes.size() == 2);
        CHECK(h.app().probe->resizes.back().width == 640);
        CHECK(h.app().probe->resizes.back().height == 400);
    }

    TEST_CASE("resize hooks work without scenes and override errors propagate") {
        app_harness<event_probe_app> h(base_config("scene-free resize"));
        int calls = 0;
        h.app().resized = [&](dim size) {
            ++calls;
            CHECK(size.width == 320);
            CHECK(size.height == 200);
        };
        REQUIRE(get_renderer().set_logical_presentation(320, 200, sdlpp::logical_presentation::letterbox));
        h.feed(pixel_size_changed_event());
        CHECK(calls == 1);
        h.app().resized = [](dim) { throw std::runtime_error("resize failed"); };
        REQUIRE(get_renderer().set_logical_presentation(640, 400, sdlpp::logical_presentation::letterbox));
        CHECK_THROWS_AS(h.feed(pixel_size_changed_event()), std::runtime_error);
    }

    TEST_CASE("default resize failures are contained and raw event dispatch continues") {
        app_harness<probe_app> h(base_config("scene resize failure"));
        int events = 0;
        h.app().probe->action = [&](const sdlpp::event&) { ++events; };
        SUBCASE("standard exception") {
            h.app().probe->resized = [](dim) { throw std::runtime_error("scene resize failed"); };
        }
        SUBCASE("unknown exception") {
            h.app().probe->resized = [](dim) { throw 42; };
        }
        REQUIRE(get_renderer().set_logical_presentation(320, 200, sdlpp::logical_presentation::letterbox));
        CHECK_NOTHROW(h.feed(pixel_size_changed_event()));
        CHECK(events == 1);
        CHECK(h.app().probe->resizes.size() == 2);
    }

    TEST_CASE("scene update failure expires even when custom rendering bypasses scenes") {
        auto cfg = base_config("scene failure frame lifetime");
        cfg.fixed.period = sim_duration{0.125f};
        app_harness<selective_render_app> h(cfg);
        int renders = 0;
        h.app().probe->draw = [&] { ++renders; };
        auto failing = std::make_unique<probe_scene>();
        failing->tick = [](sim_duration, const input_snapshot&) { throw std::runtime_error("update failed"); };
        failing->draw = [&] { FAIL("failed scene must not render"); };
        push_scene(std::move(failing));
        h.feed(take_transition_event());
        h.frame(0.125f); // Custom rendering does not call the base implementation.
        CHECK(renders == 0);
        h.feed(take_transition_event()); // Apply recovery pop, revealing the healthy scene.
        h.app().scene_render_passes = 1;
        h.frame(0.0f);
        CHECK(renders == 1);
    }

    TEST_CASE("scene failure suppresses every default rendering pass in the same frame") {
        auto cfg = base_config("scene failure render passes");
        cfg.fixed.period = sim_duration{0.125f};
        app_harness<selective_render_app> h(cfg);
        h.app().scene_render_passes = 2;
        h.app().probe->tick = [](sim_duration, const input_snapshot&) { throw std::runtime_error("update failed"); };
        int renders = 0;
        h.app().probe->draw = [&] { ++renders; };
        h.frame(0.125f);
        CHECK(renders == 0);
    }
}
