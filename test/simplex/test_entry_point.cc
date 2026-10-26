// Drive the real SIMPLEX_MAIN callbacks from doctest's existing main.
#define SDL_MAIN_HANDLED
#include <neutrino/simplex/entry_point.hh>
#undef main

#include <doctest/doctest.h>
#include <neutrino/video/globals.hh>
#include <neutrino/video/draw.hh>
#include <sdlpp/app/game_application.hh>
#include <functional>
#include <limits>
#include <vector>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
    struct probe_state {
        int width = 64;
        int height = 48;
        float scale = 1.0f;
        bool create_result = true;
        bool update_result = true;
        int cleanup_failure = 0;
        std::function<void()> create;
        std::function<void()> update;
        std::vector<float> updates;
        std::vector<neutrino::input_snapshot> inputs;
        int constructor_failure = 0;
        int created = 0;
        int cleaned = 0;
        int destroyed = 0;
        int argc = 0;
        std::string argument;
        bool video_alive_at_cleanup = false;
        bool video_alive_at_destruction = false;
        bool window_alive_at_destruction = false;
        SDL_Window* window = nullptr;
    };
    probe_state probe;

    class probe_app final : public neutrino::simplex::application {
        public:
            probe_app(int argc, char* argv[]) : application(probe.width, probe.height, "Simplex entry test", probe.scale) {
                probe.argc = argc;
                if (argc > 0) probe.argument = argv[0];
                if (probe.constructor_failure == 1) throw std::runtime_error("constructor failure");
                if (probe.constructor_failure == 2) throw 42;
            }

            ~probe_app() override {
                ++probe.destroyed;
                probe.video_alive_at_destruction = SDL_WasInit(SDL_INIT_VIDEO) != 0;
                if (probe.window) {
                    probe.window_alive_at_destruction = SDL_GetWindowID(probe.window) != 0;
                }
            }

            bool on_create() override {
                ++probe.created;
                probe.window = neutrino::get_window().get();
                if (probe.create) probe.create();
                return probe.create_result;
            }

            bool on_update(float dt, const neutrino::input_snapshot& input) override {
                probe.updates.push_back(dt);
                probe.inputs.push_back(input);
                if (probe.update) probe.update();
                return probe.update_result;
            }

            void on_destroy() override {
                ++probe.cleaned;
                probe.video_alive_at_cleanup = SDL_WasInit(SDL_INIT_VIDEO) != 0;
                if (probe.cleanup_failure == 1) throw std::runtime_error("cleanup failure");
                if (probe.cleanup_failure == 2) throw 42;
            }
    };
}

SIMPLEX_MAIN(probe_app, argc, argv)

namespace {
    // Access the virtual frame hooks through member pointers, without inventing
    // a derived object or changing the production clock. This exercises the real
    // engine scheduler with exact deltas, including frames with zero physics ticks.
    struct frame_driver : sdlpp::game_application {
        static void frame(sdlpp::game_application& app, float dt) {
            const auto tick = &frame_driver::on_update;
            const auto draw = &frame_driver::on_render;
            (app.*tick)(dt);
            (app.*draw)(neutrino::get_renderer());
        }
    };

    class app_session {
        public:
            app_session() {
                probe = {};
                SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "dummy", SDL_HINT_OVERRIDE);
                SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER, "dummy", SDL_HINT_OVERRIDE);
                SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "software", SDL_HINT_OVERRIDE);
            }

            ~app_session() {
                quit();
                SDL_ResetHint(SDL_HINT_VIDEO_DRIVER);
                SDL_ResetHint(SDL_HINT_AUDIO_DRIVER);
                SDL_ResetHint(SDL_HINT_RENDER_DRIVER);
            }

            SDL_AppResult init() {
                char name[] = "simplex-test";
                char* argv[] = {name, nullptr};
                return SDL_AppInit(&state, 1, argv);
            }

            void frame(float dt) {
                auto& owner = *static_cast<neutrino::simplex::application*>(state);
                auto* runtime = neutrino::simplex::detail::application_access::get(owner);
                frame_driver::frame(*static_cast<sdlpp::game_application*>(runtime), dt);
            }

            void quit() { SDL_AppQuit(std::exchange(state, nullptr), SDL_APP_SUCCESS); }
            void* state = nullptr;
    };

    // Read a target texture rather than the undefined post-present window buffer.
    class capture_target {
        public:
            capture_target() : renderer(neutrino::get_renderer().get()), previous(SDL_GetRenderTarget(renderer)) {
                texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 64, 48);
                REQUIRE(texture != nullptr);
                REQUIRE(SDL_SetRenderTarget(renderer, texture));
            }
            ~capture_target() {
                SDL_SetRenderTarget(renderer, previous);
                SDL_DestroyTexture(texture);
            }
            SDL_Renderer* renderer;
            SDL_Texture* previous;
            SDL_Texture* texture;
    };

    void check_pixel(SDL_Renderer* renderer, int x, int y, Uint8 red, Uint8 green, Uint8 blue) {
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> pixels(SDL_RenderReadPixels(renderer, nullptr),
                                                                        SDL_DestroySurface);
        REQUIRE(pixels != nullptr);
        Uint8 r = 0, g = 0, b = 0, a = 0;
        REQUIRE(SDL_ReadSurfacePixel(pixels.get(), x, y, &r, &g, &b, &a));
        CHECK(r == red);
        CHECK(g == green);
        CHECK(b == blue);
    }
}

TEST_SUITE("neutrino::simplex entry point") {
    TEST_CASE("forwards arguments and events and destroys the owner before SDL") {
        app_session session;
        REQUIRE(session.init() == SDL_APP_CONTINUE);
        REQUIRE(session.state != nullptr);
        CHECK(probe.argc == 1);
        CHECK(probe.argument == "simplex-test");
        CHECK(probe.created == 1);
        CHECK(SDL_AppIterate(session.state) == SDL_APP_CONTINUE);
        SDL_Event event{};
        event.type = SDL_EVENT_QUIT;
        CHECK(SDL_AppEvent(session.state, &event) == SDL_APP_SUCCESS);
        CHECK(SDL_AppIterate(session.state) == SDL_APP_SUCCESS);
        session.quit();
        CHECK(probe.cleaned == 1);
        CHECK(probe.destroyed == 1);
        CHECK(probe.video_alive_at_cleanup);
        CHECK(probe.video_alive_at_destruction);
        CHECK(probe.window_alive_at_destruction);
        CHECK(SDL_WasInit(0) == 0);
    }

    TEST_CASE("constructor exceptions become initialization failure") {
        app_session session;
        SUBCASE("standard exception") { probe.constructor_failure = 1; }
        SUBCASE("unknown exception") { probe.constructor_failure = 2; }
        CHECK(session.init() == SDL_APP_FAILURE);
        CHECK(session.state == nullptr);
        CHECK(probe.created == 0);
        CHECK(probe.cleaned == 0);
        CHECK(probe.destroyed == 0);
        CHECK(SDL_WasInit(0) == 0);
        session.quit(); // SDL still calls AppQuit after a failed AppInit.
    }

    TEST_CASE("SDL initialization failure still destroys the constructed owner") {
        app_session session;
        SDL_SetHintWithPriority(SDL_HINT_VIDEO_DRIVER, "simplex-invalid-driver", SDL_HINT_OVERRIDE);
        CHECK(session.init() == SDL_APP_FAILURE);
        REQUIRE(session.state != nullptr);
        session.quit();
        CHECK(probe.created == 0);
        CHECK(probe.cleaned == 0);
        CHECK(probe.destroyed == 1);
        CHECK(SDL_WasInit(0) == 0);
    }

    TEST_CASE("renderer initialization failure releases the window and owner") {
        app_session session;
        SDL_SetHintWithPriority(SDL_HINT_RENDER_DRIVER, "simplex-invalid-renderer", SDL_HINT_OVERRIDE);
        CHECK(session.init() == SDL_APP_FAILURE);
        REQUIRE(session.state != nullptr);
        session.quit();
        CHECK(probe.created == 0);
        CHECK(probe.cleaned == 0);
        CHECK(probe.destroyed == 1);
        CHECK(probe.video_alive_at_destruction);
        CHECK(SDL_WasInit(0) == 0);
    }

    TEST_CASE("callbacks safely reject an absent application") {
        CHECK(SDL_AppIterate(nullptr) == SDL_APP_FAILURE);
        SDL_Event event{};
        CHECK(SDL_AppEvent(nullptr, &event) == SDL_APP_FAILURE);
        SDL_AppQuit(nullptr, SDL_APP_FAILURE);
    }
}


TEST_SUITE("neutrino::simplex frames") {
    TEST_CASE("update runs once with the frame delta for zero and multiple physics steps") {
        app_session session;
        REQUIRE(session.init() == SDL_APP_CONTINUE);
        for (float dt : {0.0f, 0.05f, 0.5f, 0.001f}) session.frame(dt);
        REQUIRE(probe.updates.size() == 4);
        CHECK(probe.updates[0] == doctest::Approx(0.0f));
        CHECK(probe.updates[1] == doctest::Approx(0.05f));
        CHECK(probe.updates[2] == doctest::Approx(0.5f));
        CHECK(probe.updates[3] == doctest::Approx(0.001f));
    }

    TEST_CASE("scene-free updates receive keyboard mouse wheel and gamepad snapshots") {
        app_session session;
        REQUIRE(session.init() == SDL_APP_CONTINUE);
        SDL_VirtualJoystickDesc desc{};
        SDL_INIT_INTERFACE(&desc);
        desc.type = SDL_JOYSTICK_TYPE_GAMEPAD;
        desc.nbuttons = 1;
        desc.button_mask = 1u << SDL_GAMEPAD_BUTTON_SOUTH;
        desc.name = "Simplex input snapshot test";
        struct virtual_gamepad {
            SDL_JoystickID id;
            ~virtual_gamepad() { if (id) SDL_DetachVirtualJoystick(id); }
        } pad{SDL_AttachVirtualJoystick(&desc)};
        REQUIRE(pad.id != 0);
        SDL_Event added{};
        added.type = SDL_EVENT_GAMEPAD_ADDED;
        added.gdevice.which = pad.id;
        REQUIRE(SDL_AppEvent(session.state, &added) == SDL_APP_CONTINUE);

        SDL_Event key{};
        key.type = SDL_EVENT_KEY_DOWN;
        key.key.scancode = SDL_SCANCODE_SPACE;
        key.key.down = true;
        REQUIRE(SDL_AppEvent(session.state, &key) == SDL_APP_CONTINUE);
        SDL_Event mouse{};
        mouse.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        mouse.button.button = SDL_BUTTON_LEFT;
        mouse.button.down = true;
        REQUIRE(SDL_AppEvent(session.state, &mouse) == SDL_APP_CONTINUE);
        SDL_Event wheel{};
        wheel.type = SDL_EVENT_MOUSE_WHEEL;
        wheel.wheel.y = 2.0f;
        REQUIRE(SDL_AppEvent(session.state, &wheel) == SDL_APP_CONTINUE);
        SDL_Event button{};
        button.type = SDL_EVENT_GAMEPAD_BUTTON_DOWN;
        button.gbutton.which = pad.id;
        button.gbutton.button = SDL_GAMEPAD_BUTTON_SOUTH;
        button.gbutton.down = true;
        REQUIRE(SDL_AppEvent(session.state, &button) == SDL_APP_CONTINUE);

        const auto check_buttons = [](const neutrino::input_snapshot& input,
                                      bool pressed, bool released, bool held) {
            CHECK(input.gamepad_connected(0));
            for (const auto state : {input.key(sdlpp::scancode::space),
                                     input.mouse(sdlpp::mouse_button::left),
                                     input.gamepad_button_state(0, sdlpp::gamepad_button::south)}) {
                CHECK(state.pressed == pressed);
                CHECK(state.released == released);
                CHECK(state.held == held);
            }
        };
        REQUIRE(SDL_AppIterate(session.state) == SDL_APP_CONTINUE);
        REQUIRE(probe.inputs.size() == 1);
        check_buttons(probe.inputs[0], true, false, true);
        CHECK(probe.inputs[0].wheel() == 2);
        REQUIRE(SDL_AppIterate(session.state) == SDL_APP_CONTINUE);
        REQUIRE(probe.inputs.size() == 2);
        check_buttons(probe.inputs[1], false, false, true);
        CHECK(probe.inputs[1].wheel() == 0);
        // Copies remain stable even after the engine clears its transient state.
        check_buttons(probe.inputs[0], true, false, true);

        key.type = SDL_EVENT_KEY_UP;
        key.key.down = false;
        mouse.type = SDL_EVENT_MOUSE_BUTTON_UP;
        mouse.button.down = false;
        button.type = SDL_EVENT_GAMEPAD_BUTTON_UP;
        button.gbutton.down = false;
        REQUIRE(SDL_AppEvent(session.state, &key) == SDL_APP_CONTINUE);
        REQUIRE(SDL_AppEvent(session.state, &mouse) == SDL_APP_CONTINUE);
        REQUIRE(SDL_AppEvent(session.state, &button) == SDL_APP_CONTINUE);
        REQUIRE(SDL_AppIterate(session.state) == SDL_APP_CONTINUE);
        REQUIRE(probe.inputs.size() == 3);
        check_buttons(probe.inputs[2], false, true, false);
        REQUIRE(SDL_AppIterate(session.state) == SDL_APP_CONTINUE);
        REQUIRE(probe.inputs.size() == 4);
        check_buttons(probe.inputs[3], false, false, false);
    }

    TEST_CASE("pointer snapshots use scaled window coordinates while drawing to the canvas") {
        app_session session;
        probe.scale = 3.0f;
        REQUIRE(session.init() == SDL_APP_CONTINUE);
        auto* renderer = neutrino::get_renderer().get();
        // Verify the window's scale and viewport offset, including unclamped
        // coordinates outside the viewport on dummy drivers with mouse at (0,0).
        const SDL_Rect viewport{7, 5, probe.width - 7, probe.height - 5};
        REQUIRE(SDL_SetRenderViewport(renderer, &viewport));
        float x = 0.0f, y = 0.0f;
        SDL_GetMouseState(&x, &y);
        const sdlpp::point<float> expected{x / probe.scale - viewport.x,
                                           y / probe.scale - viewport.y};
        probe.update = [&] {
            REQUIRE(SDL_GetRenderTarget(renderer) != nullptr);
            REQUIRE(probe.inputs.size() == 1);
            const auto& pointer = probe.inputs.back().pointer();
            CHECK(pointer.window.x == doctest::Approx(x));
            CHECK(pointer.window.y == doctest::Approx(y));
            CHECK(pointer.render.x == doctest::Approx(expected.x));
            CHECK(pointer.render.y == doctest::Approx(expected.y));
            CHECK(pointer.on_screen == (SDL_GetMouseFocus() != nullptr));
        };
        session.frame(0.0f);
        CHECK(probe.updates.size() == 1);
        CHECK(SDL_GetRenderTarget(renderer) == nullptr);
    }

    TEST_CASE("callback drawing survives the engine clear and canvas pixels persist") {
        app_session session;
        probe.create = [] {
            neutrino::draw_point(1, 1, sdlpp::colors::green);
            neutrino::set_draw_color(sdlpp::colors::red);
        };
        probe.update = [] {
            if (probe.updates.size() == 1) neutrino::draw_point(3, 3);
            if (probe.updates.size() == 2) neutrino::draw_point(4, 4);
        };
        REQUIRE(session.init() == SDL_APP_CONTINUE);
        capture_target output;
        session.frame(0.05f);
        CHECK(SDL_GetRenderTarget(output.renderer) == output.texture);
        check_pixel(output.renderer, 1, 1, 0, 255, 0);
        check_pixel(output.renderer, 3, 3, 255, 0, 0);
        session.frame(0.0f);
        check_pixel(output.renderer, 3, 3, 255, 0, 0);
        check_pixel(output.renderer, 4, 4, 255, 0, 0);
        probe.update = [] {
            neutrino::set_draw_color(sdlpp::colors::blue);
            REQUIRE(neutrino::get_renderer().clear());
        };
        session.frame(0.0f);
        check_pixel(output.renderer, 1, 1, 0, 0, 255);
        check_pixel(output.renderer, 3, 3, 0, 0, 255);
    }

    TEST_CASE("callback draw color retains floating-point precision between frames") {
        app_session session;
        const sdlpp::fcolor color{0.1234f, 0.4567f, 0.7891f, 0.6543f};
        probe.create = [color] {
            REQUIRE(neutrino::get_renderer().set_draw_color_float(color));
        };
        probe.update = [color] {
            const auto current = neutrino::get_renderer().get_draw_color_float();
            REQUIRE(current);
            CHECK(current->r == doctest::Approx(color.r));
            CHECK(current->g == doctest::Approx(color.g));
            CHECK(current->b == doctest::Approx(color.b));
            CHECK(current->a == doctest::Approx(color.a));
        };
        REQUIRE(session.init() == SDL_APP_CONTINUE);
        session.frame(0.0f);
        session.frame(0.05f);
    }

    TEST_CASE("returning false stops immediately and never invokes another update") {
        app_session session;
        probe.update_result = false;
        REQUIRE(session.init() == SDL_APP_CONTINUE);
        CHECK(SDL_AppIterate(session.state) == SDL_APP_SUCCESS);
        REQUIRE(probe.updates.size() == 1);
        CHECK(SDL_AppIterate(session.state) == SDL_APP_SUCCESS);
        session.frame(0.05f);
        CHECK(probe.updates.size() == 1);
        session.quit();
        CHECK(probe.cleaned == 1);
        CHECK(probe.destroyed == 1);
    }

    TEST_CASE("update exceptions restore the target and stop future callbacks") {
        app_session session;
        probe.update = [] { throw std::runtime_error("update failure"); };
        REQUIRE(session.init() == SDL_APP_CONTINUE);
        {
            capture_target output;
            CHECK(SDL_AppIterate(session.state) == SDL_APP_FAILURE);
            CHECK(SDL_GetRenderTarget(output.renderer) == output.texture);
            CHECK(SDL_AppIterate(session.state) == SDL_APP_SUCCESS);
            CHECK(probe.updates.size() == 1);
        }
        session.quit();
        CHECK(probe.cleaned == 1);
    }

    TEST_CASE("failed creation has no frame callbacks and receives cleanup once") {
        app_session session;
        SUBCASE("false") { probe.create_result = false; }
        SUBCASE("exception") { probe.create = [] { throw std::runtime_error("create failure"); }; }
        SUBCASE("unknown exception") { probe.create = [] { throw 42; }; }
        CHECK(session.init() == SDL_APP_FAILURE);
        REQUIRE(session.state != nullptr);
        CHECK(SDL_GetRenderTarget(neutrino::get_renderer().get()) == nullptr);
        CHECK(SDL_AppIterate(session.state) == SDL_APP_SUCCESS);
        CHECK(probe.updates.empty());
        session.quit();
        CHECK(probe.created == 1);
        CHECK(probe.cleaned == 1);
        CHECK(probe.destroyed == 1);
    }

    TEST_CASE("throwing cleanup during shutdown stops callbacks and is not repeated") {
        app_session session;
        SUBCASE("standard exception") { probe.cleanup_failure = 1; }
        SUBCASE("unknown exception") { probe.cleanup_failure = 2; }
        REQUIRE(session.init() == SDL_APP_CONTINUE);
        auto& owner = *static_cast<neutrino::simplex::application*>(session.state);
        auto* runtime = neutrino::simplex::detail::application_access::get(owner);
        runtime->on_quit();
        CHECK(probe.cleaned == 1);
        CHECK(probe.video_alive_at_cleanup);
        CHECK(SDL_AppIterate(session.state) == SDL_APP_SUCCESS);
        CHECK(probe.updates.empty());
        runtime->on_quit();
        CHECK(probe.cleaned == 1);
        session.quit();
        CHECK(probe.cleaned == 1);
        CHECK(probe.destroyed == 1);
    }
}

TEST_SUITE("neutrino::simplex scaling") {
    TEST_CASE("scale enlarges the window and leaves callback coordinates at the canvas size") {
        app_session session;
        probe.width = 320;
        probe.height = 200;
        probe.scale = 3.0f;
        auto check_coordinates = [] {
            const auto logical = neutrino::render_size();
            CHECK(logical.width == 320);
            CHECK(logical.height == 200);
            const auto point = neutrino::to_render_coords(sdlpp::point<float>{30.0f, 30.0f});
            CHECK(point.x == doctest::Approx(10.0f));
            CHECK(point.y == doctest::Approx(10.0f));
        };
        probe.create = check_coordinates;
        probe.update = check_coordinates;
        REQUIRE(session.init() == SDL_APP_CONTINUE);
        int width = 0, height = 0;
        REQUIRE(SDL_GetWindowSize(neutrino::get_window().get(), &width, &height));
        CHECK(width == 960);
        CHECK(height == 600);
        session.frame(0.05f);
    }

    TEST_CASE("invalid sizes and scales fail before SDL startup or float to int conversion") {
        app_session session;
        SUBCASE("zero width") { probe.width = 0; }
        SUBCASE("negative height") { probe.height = -1; }
        SUBCASE("zero scale") { probe.scale = 0.0f; }
        SUBCASE("negative scale") { probe.scale = -1.0f; }
        SUBCASE("NaN") { probe.scale = std::numeric_limits<float>::quiet_NaN(); }
        SUBCASE("infinity") { probe.scale = std::numeric_limits<float>::infinity(); }
        SUBCASE("large finite scale") { probe.scale = std::numeric_limits<float>::max(); }
        SUBCASE("window width overflows") { probe.width = std::numeric_limits<int>::max(); probe.scale = 2.0f; }
        SUBCASE("window height overflows") { probe.height = std::numeric_limits<int>::max(); probe.scale = 2.0f; }
        SUBCASE("scaled dimension truncates to zero") { probe.scale = std::numeric_limits<float>::min(); }
        CHECK_THROWS_AS(probe_app(0, nullptr), std::invalid_argument);
        CHECK(session.init() == SDL_APP_FAILURE);
        CHECK(session.state == nullptr);
        CHECK(probe.created == 0);
        CHECK(probe.cleaned == 0);
        CHECK(SDL_WasInit(0) == 0);
    }

    TEST_CASE("INT_MAX is representable at unit scale without float rounding") {
        app_session session;
        probe.width = std::numeric_limits<int>::max();
        // Construction validates configuration without allocating the enormous canvas.
        CHECK_NOTHROW(probe_app(0, nullptr));
        CHECK(probe.created == 0);
        CHECK(SDL_WasInit(0) == 0);
    }
}
