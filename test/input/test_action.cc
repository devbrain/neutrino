//
// Created by igor on 26/11/2026.
//

#include <doctest/doctest.h>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>

#include <failsafe/logger/backend/capture_backend.hh>

#include <neutrino/input/action.hh>
#include <neutrino/input/action_bindings.hh>
#include <neutrino/input/binding.hh>
#include <neutrino/input/input_snapshot.hh>
#include <neutrino/input/keys.hh>
#include <neutrino/scene/base_scene.hh>

#include "input/action_tables.hh"
#include "scene/scenes_manager.hh"
#include "services/service_locator.hh"
#include "test_application.hh"

using namespace neutrino;

namespace {
    using failsafe::logger::backends::scoped_log_capture;

    constexpr button_state down{.pressed = true, .released = false, .held = true};
    // sdlpp names no keypad '='; its SDL name is "Keypad =".
    constexpr auto kp_equals = static_cast <key>(SDL_SCANCODE_KP_EQUALS);
    constexpr button_state held_on{.pressed = false, .released = false, .held = true};

    [[nodiscard]] input_snapshot pressing(key k) {
        input_snapshot in;
        in.set_key(k, down);
        return in;
    }

    [[nodiscard]] input_snapshot holding(key k) {
        input_snapshot in;
        in.set_key(k, held_on);
        return in;
    }

    // Fires the global actions of the table records go to now, as the scene manager does.
    void fire(const input_snapshot& in) {
        (void)detail::current_action_table().fire_globals(in);
    }

    // The records registered after the first @p before.
    [[nodiscard]] std::vector <action_record> newer_than(std::size_t before) {
        auto records = action_records();
        records.erase(records.begin(), records.begin() + static_cast <std::ptrdiff_t>(before));
        return records;
    }

    // A key going down, as SDL delivers it: sdlpp tracks it, and the next frame samples it.
    void key_down(test::test_application& app, SDL_Scancode scancode) {
        SDL_Event ev{};
        ev.type = SDL_EVENT_KEY_DOWN;
        ev.key.scancode = scancode;
        ev.key.down = true;
        app.feed(ev);
    }

    // A small hidden window whose fixed step lasts @p period: a long one runs no step in a
    // frame, a short one at least one.
    [[nodiscard]] application_config stepping(float period) {
        application_config cfg;
        cfg.title = "global actions";
        cfg.width = 100;
        cfg.height = 100;
        cfg.flags = window_flags::hidden;
        cfg.fixed.period = sim_duration{period};
        return cfg;
    }

    // Records what F11 looked like to the top scene.
    class probe_scene final : public base_scene {
        public:
            explicit probe_scene(scene_flags flags = scene_flags::hides_below | scene_flags::pauses_below)
                : m_flags(flags) {
            }

            [[nodiscard]] scene_flags flags() const override { return m_flags; }

            void fixed_update(sim_duration, const input_snapshot& in) override {
                if (updates++ == 0) {
                    f11 = in.key(key::f11);
                    b = in.key(key::b);
                }
            }

            void render() override {}

            int updates = 0;
            button_state f11{};   // as the first step saw them
            button_state b{};

        private:
            scene_flags m_flags;
    };
}

TEST_SUITE("neutrino::action") {
    TEST_CASE("alternative bindings act as one virtual button; in.pressed(a) is a.pressed(in)") {
        const action left{key::left, key::a, pad_button::dpad_left};
        CHECK(left.bindings() == std::vector <binding>{key::left, key::a, gamepad_button{0, pad_button::dpad_left}});

        const auto in = pressing(key::a);
        CHECK(left.pressed(in));
        CHECK(in.pressed(left));
        CHECK(left.held(in));
        CHECK(in.held(left));
        CHECK_FALSE(left.released(in));
        CHECK_FALSE(in.released(left));

        input_snapshot both;
        both.set_key(key::left, held_on);
        both.set_key(key::a, down);
        CHECK(left.held(both));
        CHECK_FALSE(left.pressed(both));
    }

    TEST_CASE("an action is made explicitly, so a key or button never turns into one by itself") {
        static_assert(!std::is_convertible_v <key, action>);
        static_assert(!std::is_convertible_v <pad_button, action>);
        static_assert(!std::is_constructible_v <action, int>);
        // These were ambiguous while a pad button or a keycode also converted to an action.
        input_snapshot in;
        in.set_gamepad_connected(0, true);
        in.set_gamepad_button(0, pad_button::south, down);
        in.set_key(key::a, down);
        CHECK(in.pressed(pad_button::south));
        CHECK(in.pressed(hotkey{static_cast <keycode>('a')}));
    }

    TEST_CASE("one binding makes an action; a description may come first") {
        const action fire_key{key::space};
        CHECK(fire_key.held(holding(key::space)));

        const action jump{"Jump", key::space, mods::ctrl + key::j};
        CHECK(jump.description() == "Jump");
        CHECK(jump.bindings() == std::vector <binding>{key::space, mods::ctrl + key::j});
    }

    TEST_CASE("checking an action never given bindings answers false and warns once; rebind({}) is silent") {
        scoped_log_capture logs(LOGGER_LEVEL_WARN);
        const action empty;
        CHECK_FALSE(empty.held(holding(key::space)));
        CHECK_FALSE(empty.pressed(pressing(key::space)));
        CHECK(logs.count("before it was given bindings") == 1);

        const action described{"Fire"};
        CHECK_FALSE(described.pressed(pressing(key::space)));
        CHECK(logs.count("before it was given bindings") == 2);

        action unbound;
        unbound.rebind({});
        CHECK_FALSE(unbound.pressed(pressing(key::space)));
        CHECK(logs.count("before it was given bindings") == 2);
    }

    TEST_CASE("an empty action is not listed; it registers once given bindings, keeping its description") {
        const auto before = action_records().size();
        action later;
        later.set_description("Later");
        CHECK(action_records().size() == before);

        later.rebind({key::l});
        CHECK(action_records().size() == before + 1);
        CHECK(later.description() == "Later");
        CHECK(later.held(holding(key::l)));
    }

    TEST_CASE("rebind replaces the bindings; a malformed text keeps them, is logged, and answers false") {
        action fire_key{key::space};
        fire_key.rebind({mods::ctrl + key::f, mouse_button::left});
        CHECK(to_string(fire_key.bindings()) == "Ctrl+F, Mouse Left");

        CHECK(fire_key.rebind("Space, Pad South"));
        CHECK(to_string(fire_key.bindings()) == "Space, Pad South");

        scoped_log_capture logs(LOGGER_LEVEL_WARN);
        CHECK_FALSE(fire_key.rebind("Space, Nokey"));
        CHECK(to_string(fire_key.bindings()) == "Space, Pad South");
        CHECK(logs.count("Nokey") == 1);
    }

    TEST_CASE("a disabled action answers false without logging; the flag is its own") {
        action a{key::space};
        const action b{key::space};
        a.disable();
        CHECK_FALSE(a.enabled());
        CHECK_FALSE(a.held(holding(key::space)));
        CHECK(b.held(holding(key::space)));
        a.enable();
        CHECK(a.held(holding(key::space)));

        scoped_log_capture logs(LOGGER_LEVEL_WARN);
        action empty;
        empty.disable();
        CHECK_FALSE(empty.held(holding(key::space)));
        CHECK(logs.count() == 0);
    }

    TEST_CASE("moving an action moves its record; assigning over one removes the old record") {
        const auto before = action_records().size();
        action a{key::space};
        action b = std::move(a);
        CHECK(b.held(holding(key::space)));
        CHECK(action_records().size() == before + 1);

        action c{key::x};
        CHECK(action_records().size() == before + 2);
        c = std::move(b);
        CHECK(action_records().size() == before + 1);
        CHECK(c.held(holding(key::space)));
        CHECK_FALSE(c.held(holding(key::x)));
    }

    TEST_CASE("an action made while no application runs is checked in every application that follows") {
        const action jump{key::space};
        const auto in = holding(key::space);
        {
            test::test_application app("first application");
            CHECK(jump.held(in));
        }
        {
            test::test_application app("second application");
            CHECK(jump.held(in));
        }
    }

    TEST_CASE("actions made and destroyed on another thread while applications start and stop keep their records") {
        const auto before = action_records().size();
        std::atomic <bool> stop{false};
        std::atomic <int> made{0};
        std::atomic <int> lost{0};
        std::thread loader([&] {
            while (!stop.load()) {
                const action a{key::space};
                if (a.bindings().size() != 1) {
                    ++lost;
                }
                ++made;
            }
        });
        for (int i = 0; i < 4; ++i) {
            test::test_application app("restarted application");
        }
        stop = true;
        loader.join();
        CHECK(made.load() > 0);
        CHECK(lost.load() == 0);
        CHECK(action_records().size() == before);
    }

    TEST_CASE("an action that outlives its application is removed without a warning") {
        auto jump = std::make_unique <action>(key::space);
        {
            test::test_application app("outlived application");
            CHECK(jump->held(holding(key::space)));
        }
        scoped_log_capture logs(LOGGER_LEVEL_WARN);
        jump.reset();
        CHECK(logs.count("non existent") == 0);
    }
}

TEST_SUITE("neutrino::global_action") {
    TEST_CASE("fires once per frame, before the steps, and the scenes' snapshot does not have the press") {
        test::test_application app(stepping(0.0001f));   // every frame runs a step
        auto& mgr = service_locator::instance().get_scenes_manager();
        auto scene = std::make_unique <probe_scene>();   // pauses the scene below it
        auto* probe = scene.get();
        mgr.push_scene_sync(std::move(scene));

        int fired = 0;
        const global_action fullscreen{{key::f11}, [&fired] { ++fired; }};

        key_down(app, SDL_SCANCODE_F11);
        key_down(app, SDL_SCANCODE_B);
        app.iterate();
        CHECK(fired == 1);
        REQUIRE(probe->updates > 0);
        CHECK_FALSE(probe->f11.pressed);
        CHECK(probe->f11.held);
        CHECK(probe->b.pressed);

        app.iterate();   // still held: no repeat
        CHECK(fired == 1);
    }

    TEST_CASE("a frame that runs no step does not lose a global action's press") {
        test::test_application app(stepping(1000.0f));   // no frame runs a step
        int fired = 0;
        const global_action fullscreen{{key::f11}, [&fired] { ++fired; }};
        key_down(app, SDL_SCANCODE_F11);
        app.iterate();
        CHECK(fired == 1);
    }

    TEST_CASE("an application member made before on_ready() fires in the application") {
        int fired = 0;
        const global_action terminal{{key::grave}, [&fired] { ++fired; }};
        test::test_application app(stepping(1.0f / 60.0f));
        key_down(app, SDL_SCANCODE_GRAVE);
        app.iterate();
        CHECK(fired == 1);
    }

    TEST_CASE("rebind keeps the callback; a description may come first") {
        int fired = 0;
        global_action fullscreen{"Fullscreen", {mods::alt + key::return_key}, [&fired] { ++fired; }};
        CHECK(fullscreen.description() == "Fullscreen");
        fullscreen.rebind({key::f9});
        fire(pressing(key::f9));
        CHECK(fired == 1);

        fullscreen.disable();
        fire(pressing(key::f9));
        CHECK(fired == 1);
    }

    TEST_CASE("a default global action never fires and is not listed") {
        const auto before = action_records().size();
        global_action empty;
        empty.rebind({key::f9});
        CHECK(action_records().size() == before);
        fire(pressing(key::f9));   // nothing to fire; nothing breaks

        int fired = 0;
        empty = global_action{{key::f8}, [&fired] { ++fired; }};
        CHECK(action_records().size() == before + 1);
        fire(pressing(key::f8));
        CHECK(fired == 1);
    }
}

TEST_SUITE("neutrino::action_record") {
    TEST_CASE("a rebinding screen sees description, kind, current and default bindings, and rebinds through the record") {
        const auto before = action_records().size();
        const action left{"Move left", key::left};
        int fired = 0;
        const global_action fullscreen{"Fullscreen", {key::f11}, [&fired] { ++fired; }};

        auto records = newer_than(before);
        REQUIRE(records.size() == 2);
        CHECK(records[0].description() == "Move left");
        CHECK_FALSE(records[0].is_global());
        CHECK(records[1].description() == "Fullscreen");
        CHECK(records[1].is_global());

        CHECK(records[0].rebind({key::a}));
        CHECK(left.bindings() == std::vector <binding>{key::a});
        CHECK(records[0].bindings() == std::vector <binding>{key::a});
        CHECK(records[0].default_bindings() == std::vector <binding>{key::left});
        CHECK(records[0].reset());
        CHECK(left.bindings() == std::vector <binding>{key::left});

        CHECK(records[1].rebind({key::f12}));   // keeps the callback
        fire(pressing(key::f12));
        CHECK(fired == 1);
    }

    TEST_CASE("a record whose action is gone answers empty, and rebinding it does nothing") {
        std::optional <action_record> record;
        {
            const action gone{"Gone", key::g};
            record = action_records().back();
            CHECK(record->alive());
            CHECK(record->description() == "Gone");
        }
        CHECK_FALSE(record->alive());
        CHECK(record->description().empty());
        CHECK(record->bindings().empty());
        CHECK_FALSE(record->rebind({key::h}));
        CHECK_FALSE(record->reset());
    }
}

TEST_SUITE("neutrino::action_bindings") {
    TEST_CASE("writes one 'name = bindings' line per action, and reads them back in any order") {
        action left{key::left, key::a};
        action fire_key{mods::ctrl + key::f};
        global_action fullscreen{{mods::alt + key::return_key}, [] {}};
        const action_bindings b{{"left", left}, {"fire", fire_key}, {"fullscreen", fullscreen}};

        std::ostringstream out;
        out << b;
        CHECK(out.str() == "left = Left, A\nfire = Ctrl+F\nfullscreen = Alt+Return\n");

        std::istringstream in("# saved by the game\n\nfullscreen = F11\nleft = Mouse Left\n"
                              "fire = None+Space, Pad 2 South\n");
        in >> b;
        CHECK(to_string(left.bindings()) == "Mouse Left");
        CHECK(to_string(fire_key.bindings()) == "None+Space, Pad 2 South");
        CHECK(to_string(fullscreen.bindings()) == "F11");
    }

    TEST_CASE("a bad or missing line keeps the old bindings; a name not listed is skipped; all are logged") {
        action left{key::left};
        action fire_key{key::space};
        const action_bindings b{{"left", left}, {"fire", fire_key}};

        scoped_log_capture logs(LOGGER_LEVEL_WARN);
        std::istringstream in("left = Nokey\nunknown = Space\nno equals here\n");
        in >> b;
        CHECK(left.bindings() == std::vector <binding>{key::left});
        CHECK(fire_key.bindings() == std::vector <binding>{key::space});
        CHECK(logs.count("Malformed bindings for left") == 1);
        CHECK(logs.count("not listed are skipped: unknown") == 1);
        CHECK(logs.count("without '='") == 1);
        CHECK(logs.count("No bindings for fire") == 1);
    }

    TEST_CASE("bindings with '=' in them, and none at all, read back") {
        action equals{mods::ctrl + key::equals, kp_equals};
        action none{key::n};
        none.rebind({});
        const action_bindings b{{"zoom", equals}, {"none", none}};

        std::stringstream io;
        io << b;
        CHECK(io.str() == "zoom = Ctrl+=, Keypad =\nnone = \n");
        equals.rebind({key::z});
        none.rebind({key::n});
        io >> b;
        CHECK(equals.bindings() == std::vector <binding>{mods::ctrl + key::equals, kp_equals});
        CHECK(none.bindings().empty());
    }

    TEST_CASE("loaded bindings are not the defaults; an action never given bindings is not written") {
        action left{key::left};
        action fire_key{"Fire"};
        action never{"Never"};
        const action_bindings b{{"left", left}, {"fire", fire_key}, {"never", never}};

        std::istringstream in("left = A\nfire = Mouse Right\n");
        in >> b;
        CHECK(left.bindings() == std::vector <binding>{key::a});
        CHECK(fire_key.bindings() == std::vector <binding>{mouse_button::right});

        for (auto& r : action_records()) {
            if (r.description() == "Fire") {
                CHECK(r.default_bindings().empty());   // the game gave it none
                CHECK(r.reset());
            }
        }
        CHECK(fire_key.bindings().empty());

        std::ostringstream out;
        out << b;
        CHECK(out.str() == "left = A\nfire = \n");   // "never" was never given bindings
    }

    TEST_CASE("a keycode binding that maps to no key is saved as no key, with a warning") {
        action odd{hotkey{mods::ctrl, static_cast <keycode>(0xE9)}};   // é: no key on a US layout
        const action_bindings b{{"odd", odd}};
        scoped_log_capture logs(LOGGER_LEVEL_WARN);
        std::ostringstream out;
        out << b;
        CHECK(out.str() == "odd = Ctrl+Scancode 0\n");
        CHECK(logs.count("maps to no key") == 1);
    }

    TEST_CASE("names that cannot be written as a line are refused") {
        action a{key::a};
        action b{key::b};
        CHECK_THROWS(action_bindings{{"", a}});
        CHECK_THROWS(action_bindings{{"a=b", a}});
        CHECK_THROWS(action_bindings{{"#a", a}});
        CHECK_THROWS(action_bindings{{" a", a}});
        CHECK_THROWS(action_bindings{{"two\nlines", a}});
        CHECK_THROWS(action_bindings{{"x", a}, {"x", b}});
    }

    TEST_CASE("through a file") {
        const auto path = std::filesystem::temp_directory_path() / "neutrino_test_action_bindings.txt";
        action left{key::left};
        const action_bindings b{{"left", left}};
        {
            std::ofstream out(path);
            REQUIRE(out);
            out << b;
        }
        left.rebind({key::a});
        {
            std::ifstream in(path);
            REQUIRE(in);
            in >> b;
        }
        std::filesystem::remove(path);
        CHECK(left.bindings() == std::vector <binding>{key::left});
    }
}
