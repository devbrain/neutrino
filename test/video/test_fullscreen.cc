//
// Created by igor on 25/11/2026.
//

#include <doctest/doctest.h>

#include <stdexcept>

#include <SDL3/SDL.h>

#include <neutrino/application.hh>
#include <neutrino/video/globals.hh>

#include "test_application.hh"
#include "video/fullscreen.hh"

namespace {
    using neutrino::detail::fullscreen_tracker;

    // The ENFORCE of a toggle with no application, not a failure to post into a stopped SDL.
    const doctest::Contains not_running("needs a running application");

    // Counts the request events a tracker asks to post.
    struct poster {
        int posted = 0;

        void operator()() {
            ++posted;
        }
    };

    // A window that is shown: SDL only records a fullscreen request for a hidden one, until it
    // is shown. The dummy video driver switches synchronously, as Windows does.
    [[nodiscard]] neutrino::application_config shown_window(const char* title) {
        neutrino::application_config cfg;
        cfg.title = title;
        cfg.width = 100;
        cfg.height = 100;
        cfg.flags = neutrino::window_flags::none;
        return cfg;
    }

    // One cycle's event handling: every queued event through the real dispatch.
    void handle_events(neutrino::test::test_application& app) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            app.feed(ev);
        }
    }
}

TEST_SUITE("neutrino::detail::fullscreen_tracker") {
    TEST_CASE("toggles made in one cycle cancel in pairs, and post one request") {
        fullscreen_tracker tracker;
        poster post;
        tracker.begin(false);

        tracker.toggle(std::ref(post));
        tracker.toggle(std::ref(post));
        CHECK(post.posted == 1);
        CHECK_FALSE(tracker.take().has_value());

        tracker.toggle(std::ref(post));
        tracker.toggle(std::ref(post));
        tracker.toggle(std::ref(post));
        CHECK(post.posted == 2);
        CHECK(tracker.take() == true);
    }

    TEST_CASE("a toggle made before the previous switch completed reverses it: asynchronous window systems") {
        fullscreen_tracker tracker;
        poster post;
        tracker.begin(false);

        tracker.toggle(std::ref(post));
        REQUIRE(tracker.take() == true);
        tracker.track(true);   // SDL took it; on Wayland or macOS its flag still says windowed

        tracker.toggle(std::ref(post));
        CHECK(tracker.take() == false);
    }

    TEST_CASE("a switch made by something else is followed: the next toggle flips it") {
        fullscreen_tracker tracker;
        poster post;
        tracker.begin(false);

        tracker.track(true);   // the window manager made the window fullscreen
        tracker.toggle(std::ref(post));
        CHECK(tracker.take() == false);
    }

    TEST_CASE("a toggle whose request could not be posted is undone") {
        fullscreen_tracker tracker;
        tracker.begin(false);

        CHECK_THROWS(tracker.toggle([] { throw std::runtime_error("queue full"); }));
        poster post;
        tracker.toggle(std::ref(post));
        CHECK(post.posted == 1);   // nothing was waiting
        CHECK(tracker.take() == true);
    }

    TEST_CASE("toggling needs a running application, and an ended one drops its toggles") {
        fullscreen_tracker tracker;
        poster post;
        CHECK_THROWS_WITH(tracker.toggle(std::ref(post)), not_running);

        tracker.begin(false);
        tracker.toggle(std::ref(post));
        tracker.end();
        CHECK_THROWS_WITH(tracker.toggle(std::ref(post)), not_running);
        CHECK(post.posted == 1);

        tracker.begin(false);
        CHECK_FALSE(tracker.take().has_value());
    }
}

TEST_SUITE("neutrino::toggle_fullscreen") {
    TEST_CASE("a toggle is applied when its event is handled; is_fullscreen() answers at once") {
        neutrino::test::test_application app(shown_window("deferred fullscreen"));
        REQUIRE_FALSE(neutrino::is_fullscreen());

        neutrino::toggle_fullscreen();
        CHECK_FALSE(neutrino::is_fullscreen());
        handle_events(app);
        CHECK(neutrino::is_fullscreen());
        CHECK(app.app().is_fullscreen());

        app.app().toggle_fullscreen();
        app.app().toggle_fullscreen();
        handle_events(app);
        CHECK(neutrino::is_fullscreen());

        neutrino::toggle_fullscreen();
        handle_events(app);
        CHECK_FALSE(neutrino::is_fullscreen());
    }

    TEST_CASE("a switch is tracked when it is sent, not when the window reports it") {
        neutrino::test::test_application app(shown_window("switch in flight"));
        // On Wayland or macOS the window reports the switch later; here it is held back.
        const auto in_flight = [&app] {
            SDL_Event ev;
            while (SDL_PollEvent(&ev)) {
                if (ev.type != SDL_EVENT_WINDOW_ENTER_FULLSCREEN && ev.type != SDL_EVENT_WINDOW_LEAVE_FULLSCREEN) {
                    app.feed(ev);
                }
            }
        };

        neutrino::toggle_fullscreen();
        in_flight();
        neutrino::toggle_fullscreen();
        in_flight();
        CHECK_FALSE(neutrino::is_fullscreen());
    }

    TEST_CASE("a switch made by something else is followed: the next toggle flips it") {
        neutrino::test::test_application app(shown_window("outside fullscreen"));
        REQUIRE(SDL_SetWindowFullscreen(neutrino::get_window().get(), true));
        handle_events(app);
        REQUIRE(neutrino::is_fullscreen());

        neutrino::toggle_fullscreen();
        handle_events(app);
        CHECK_FALSE(neutrino::is_fullscreen());
    }

    TEST_CASE("toggling needs a running application") {
        CHECK_THROWS_WITH(neutrino::toggle_fullscreen(), not_running);
        {
            neutrino::test::test_application app(shown_window("ended application"));
            app.shutdown();
            CHECK_THROWS_WITH(neutrino::toggle_fullscreen(), not_running);
        }
    }
}
