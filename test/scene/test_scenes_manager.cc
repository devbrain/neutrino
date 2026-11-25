//
// Tests for scenes_manager's shutdown path.
//
// finish() runs at application shutdown, while the renderer and audio are still alive, and the
// application's own teardown() runs immediately after it. That ordering is a contract: teardown
// may release assets the scenes were using, which is only safe if finish() actually emptied the
// stack. A scene whose on_exit throws must therefore not be able to strand itself -- or anything
// beneath it -- on the stack.
//
#include <doctest/doctest.h>

#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <sdlpp/events/events.hh>

#include <neutrino/scene/base_scene.hh>
#include <neutrino/scene/scene_transitions.hh>

#include "scene/scenes_manager.hh"        // internal: the shutdown path under test
#include "services/service_locator.hh"    // ...reached through the app's own instance
#include "test_application.hh"

using namespace neutrino;

namespace {
    // Records its own exit into a shared log, so the test can see the whole stack drain in order.
    class recording_scene : public base_scene {
        public:
            recording_scene(std::vector <int>& log, int id, bool throw_on_exit)
                : m_log(log), m_id(id), m_throw(throw_on_exit) {
            }

            void on_exit() override {
                m_log.push_back(m_id);
                if (m_throw) {
                    throw std::runtime_error("scene refused to exit");
                }
            }

            void fixed_update(sim_duration, const input_snapshot&) override {}
            void render() override {}

        private:
            std::vector <int>& m_log;
            int m_id;
            bool m_throw;
    };

    // What one scene saw during a test. Owned by the test (declared before the application, so it
    // outlives the scene that writes to it).
    struct probe_counts {
        int updates = 0;
        int live_input = 0; // updates that received the frame's real snapshot
        int renders = 0;
        int pauses = 0;
        int resumes = 0;
        int actions = 0;
        int enters = 0;
        int exits = 0;
        int resizes = 0;
    };

    // A scene with chosen flags that records every call the manager makes on it. Renders also go
    // to a shared log, so the bottom-to-top draw order can be checked.
    class flagged_scene : public base_scene {
        public:
            flagged_scene(scene_flags flags, probe_counts& counts, const input_snapshot* live,
                          std::vector <std::string>* render_log = nullptr, std::string name = {})
                : m_flags(flags), m_counts(counts), m_live(live), m_render_log(render_log),
                  m_name(std::move(name)) {
            }

            [[nodiscard]] scene_flags flags() const override { return m_flags; }

            // passes_input may change while the scene is on the stack.
            void set_flags(scene_flags flags) { m_flags = flags; }
            // Flags to switch to inside the next handle_event, as a console does when it opens.
            void set_flags_on_action(scene_flags flags) { m_flags_on_action = flags; }

            void fixed_update(sim_duration, const input_snapshot& in) override {
                ++m_counts.updates;
                if (&in == m_live) {
                    ++m_counts.live_input;
                }
            }

            void render() override {
                ++m_counts.renders;
                if (m_render_log) {
                    m_render_log->push_back(m_name);
                }
            }

            void on_pause() override { ++m_counts.pauses; }
            void on_resume() override { ++m_counts.resumes; }
            void on_enter() override { ++m_counts.enters; }
            void on_exit() override { ++m_counts.exits; }
            void on_resize(dim) override { ++m_counts.resizes; }
            void handle_event(const event&) override {
                ++m_counts.actions;
                if (m_flags_on_action) {
                    m_flags = *m_flags_on_action;
                    m_flags_on_action.reset();
                }
            }

        private:
            scene_flags m_flags;
            std::optional <scene_flags> m_flags_on_action;
            probe_counts& m_counts;
            const input_snapshot* m_live;
            std::vector <std::string>* m_render_log;
            std::string m_name;
    };

    constexpr auto both = scene_flags::hides_below | scene_flags::pauses_below;

    // The application's scenes_manager with an empty stack. scenes_manager is a singleton (its ctor
    // ENFORCEs that no other exists), so tests drive the one the test application owns -- after
    // clearing the idle first scene the harness installs, so each test builds its own stack.
    scenes_manager& empty_manager() {
        scenes_manager& mgr = service_locator::instance().get_scenes_manager();
        mgr.finish();
        REQUIRE(mgr.empty());
        return mgr;
    }

    // One frame: a fixed step with the given live snapshot, then a render.
    void frame(scenes_manager& mgr, const input_snapshot& live) {
        mgr.fixed_update(sim_duration{1.0f / 60.0f}, live);
        mgr.render();
    }

    // A key press delivered straight to the manager, as the main loop delivers raw events.
    void key_event(scenes_manager& mgr) {
        SDL_Event raw{};
        raw.type = SDL_EVENT_KEY_DOWN;
        mgr.handle_event(sdlpp::event(raw));
    }

    // Transition requests are only queued; deliver them the way the main loop does. The main loop
    // contains each event's exception (application::event) and keeps polling, so this does too and
    // rethrows the first one at the end. Stopping at a throw would leave SDL's poll sentinel in the
    // queue, and the next drain would end at it before reaching the events queued after it.
    void deliver(scenes_manager& mgr) {
        std::exception_ptr failure;
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            try {
                mgr.handle_event(sdlpp::event(ev));
            } catch (...) {
                if (!failure) {
                    failure = std::current_exception();
                }
            }
        }
        if (failure) {
            std::rethrow_exception(failure);
        }
    }

    void pop_now(scenes_manager& mgr) {
        scenes_manager::pop_scene();
        deliver(mgr);
    }

    void push_now(scenes_manager& mgr, std::unique_ptr <base_scene> scene) {
        neutrino::push_scene(std::move(scene));
        deliver(mgr);
    }

    void replace_now(scenes_manager& mgr, std::unique_ptr <base_scene> scene) {
        neutrino::replace_scene(std::move(scene));
        deliver(mgr);
    }

    void pin_now(scenes_manager& mgr, std::unique_ptr <base_scene> scene) {
        neutrino::pin_scene(std::move(scene));
        deliver(mgr);
    }

    void unpin_now(scenes_manager& mgr) {
        neutrino::unpin_scene();
        deliver(mgr);
    }
} // namespace

TEST_SUITE("neutrino::scene scenes_manager shutdown") {
    TEST_CASE("finish drains the whole stack even when a scene's on_exit throws") {
        neutrino::test::test_application app("scenes_manager finish test");
        scenes_manager& mgr = empty_manager();

        std::vector <int> exited;
        mgr.push_scene_sync(std::make_unique <recording_scene>(exited, 0, /*throw=*/false));
        mgr.push_scene_sync(std::make_unique <recording_scene>(exited, 1, /*throw=*/true));
        mgr.push_scene_sync(std::make_unique <recording_scene>(exited, 2, /*throw=*/false));
        REQUIRE_FALSE(mgr.empty());

        CHECK_NOTHROW(mgr.finish());

        // Top-to-bottom, and the thrower does not stop the ones underneath it. Before the
        // per-scene guard, the throw escaped finish() with scenes 1 and 0 still on the stack --
        // and shutdown carried on regardless, so application::teardown() would then free assets
        // those two still referenced.
        REQUIRE(exited.size() == 3);
        CHECK(exited[0] == 2);
        CHECK(exited[1] == 1); // threw
        CHECK(exited[2] == 0); // still exited
        CHECK(mgr.empty());    // the postcondition teardown depends on
    }

    TEST_CASE("finish on an empty stack is a no-op") {
        neutrino::test::test_application app("scenes_manager empty finish test");
        scenes_manager& mgr = empty_manager();
        CHECK_NOTHROW(mgr.finish());
        CHECK(mgr.empty());
    }
}

// The two scene_flags bits are independent: hides_below stops the scenes below from being drawn,
// pauses_below stops them from being updated. The scene that carries a flag is itself always
// updated and drawn -- the flag is about what lies beneath it.
TEST_SUITE("neutrino::scene scenes_manager flags") {
    TEST_CASE("a single scene with the default flags is updated and rendered") {
        probe_counts only;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager single scene");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, only, &live));

        frame(mgr, live);
        frame(mgr, live);

        CHECK(only.updates == 2);
        CHECK(only.live_input == 2);
        CHECK(only.renders == 2);
        mgr.finish();
    }

    TEST_CASE("each flag combination on the top scene") {
        probe_counts below;
        probe_counts top;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager flag combinations");
        scenes_manager& mgr = empty_manager();

        scene_flags top_flags = scene_flags::none;
        bool below_updated = false;
        bool below_rendered = false;
        SUBCASE("none: the scene below is updated and drawn") {
            top_flags = scene_flags::none;
            below_updated = true;
            below_rendered = true;
        }
        SUBCASE("hides_below: the scene below is updated but not drawn") {
            top_flags = scene_flags::hides_below;
            below_updated = true;
            below_rendered = false;
        }
        SUBCASE("pauses_below: the scene below is drawn but frozen") {
            top_flags = scene_flags::pauses_below;
            below_updated = false;
            below_rendered = true;
        }
        SUBCASE("both: the scene below is neither updated nor drawn") {
            top_flags = both;
            below_updated = false;
            below_rendered = false;
        }

        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, below, &live));
        mgr.push_scene_sync(std::make_unique <flagged_scene>(top_flags, top, &live));
        frame(mgr, live);

        CHECK(top.updates == 1);
        CHECK(top.renders == 1);
        CHECK(top.live_input == 1);
        CHECK(below.updates == (below_updated ? 1 : 0));
        CHECK(below.renders == (below_rendered ? 1 : 0));
        // Even when updated, a covered scene never sees the frame's real input.
        CHECK(below.live_input == 0);
        mgr.finish();
    }

    TEST_CASE("scenes are drawn bottom to top") {
        probe_counts a;
        probe_counts b;
        probe_counts c;
        std::vector <std::string> drawn;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager draw order");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, a, &live, &drawn, "a"));
        mgr.push_scene_sync(std::make_unique <flagged_scene>(scene_flags::none, b, &live, &drawn, "b"));
        mgr.push_scene_sync(std::make_unique <flagged_scene>(scene_flags::none, c, &live, &drawn, "c"));

        mgr.render();

        CHECK(drawn == std::vector <std::string>{"a", "b", "c"});
        mgr.finish();
    }

    TEST_CASE("each walk stops at the nearest scene carrying its flag") {
        // bottom: both, mid: pauses_below, top: hides_below.
        //   update walks down to mid (nearest pauses_below): mid and top update, bottom is frozen.
        //   render walks down to top (nearest hides_below): only top is drawn.
        probe_counts bottom;
        probe_counts mid;
        probe_counts top;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager independent walks");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, bottom, &live));
        mgr.push_scene_sync(std::make_unique <flagged_scene>(scene_flags::pauses_below, mid, &live));
        mgr.push_scene_sync(std::make_unique <flagged_scene>(scene_flags::hides_below, top, &live));

        frame(mgr, live);

        CHECK(bottom.updates == 0);
        CHECK(bottom.renders == 0);
        CHECK(mid.updates == 1);
        CHECK(mid.live_input == 0);
        CHECK(mid.renders == 0);
        CHECK(top.updates == 1);
        CHECK(top.live_input == 1);
        CHECK(top.renders == 1);
        mgr.finish();
    }

    TEST_CASE("a flag deep in the stack still bounds the walk") {
        // bottom: both, then two overlays with no flags: everything from bottom up runs.
        probe_counts bottom;
        probe_counts mid;
        probe_counts top;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager deep walk");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, bottom, &live));
        mgr.push_scene_sync(std::make_unique <flagged_scene>(scene_flags::none, mid, &live));
        mgr.push_scene_sync(std::make_unique <flagged_scene>(scene_flags::none, top, &live));

        frame(mgr, live);

        CHECK(bottom.updates == 1);
        CHECK(bottom.renders == 1);
        CHECK(mid.updates == 1);
        CHECK(mid.renders == 1);
        CHECK(top.updates == 1);
        CHECK(top.renders == 1);
        CHECK(bottom.live_input + mid.live_input == 0);
        CHECK(top.live_input == 1);
        mgr.finish();
    }

    TEST_CASE("only the top scene receives events") {
        probe_counts below;
        probe_counts top;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager event routing");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, below, &live));
        mgr.push_scene_sync(std::make_unique <flagged_scene>(scene_flags::none, top, &live));

        SDL_Event raw{};
        raw.type = SDL_EVENT_KEY_DOWN;
        mgr.handle_event(sdlpp::event(raw));

        CHECK(top.actions == 1);
        CHECK(below.actions == 0);
        mgr.finish();
    }

    TEST_CASE("on_pause / on_resume follow pauses_below, not hides_below") {
        probe_counts below;
        probe_counts top;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager pause follows pauses_below");
        scenes_manager& mgr = empty_manager();

        scene_flags top_flags = scene_flags::none;
        int expected = 0;
        SUBCASE("none: not paused") {
            top_flags = scene_flags::none;
            expected = 0;
        }
        SUBCASE("hides_below: hidden but still updating, so not paused") {
            top_flags = scene_flags::hides_below;
            expected = 0;
        }
        SUBCASE("pauses_below: a pause menu freezes the scene below, so it is paused") {
            top_flags = scene_flags::pauses_below;
            expected = 1;
        }
        SUBCASE("both: paused") {
            top_flags = both;
            expected = 1;
        }

        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, below, &live));
        mgr.push_scene_sync(std::make_unique <flagged_scene>(top_flags, top, &live));
        CHECK(below.pauses == expected);
        CHECK(below.resumes == 0);

        pop_now(mgr);

        CHECK(below.pauses == expected);
        CHECK(below.resumes == expected);
        mgr.finish();
    }

    TEST_CASE("a scene whose on_enter throws resumes the scene it paused") {
        class failing_scene : public base_scene {
            public:
                explicit failing_scene(scene_flags flags) : m_flags(flags) {}
                [[nodiscard]] scene_flags flags() const override { return m_flags; }
                void on_enter() override { throw std::runtime_error("cannot enter"); }
                void fixed_update(sim_duration, const input_snapshot&) override {}
                void render() override {}
            private:
                scene_flags m_flags;
        };

        probe_counts below;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager on_enter rollback");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, below, &live));

        scene_flags flags = scene_flags::none;
        int expected = 0;
        SUBCASE("pauses_below: paused, then resumed by the rollback") {
            flags = scene_flags::pauses_below;
            expected = 1;
        }
        SUBCASE("hides_below: never paused, so not resumed") {
            flags = scene_flags::hides_below;
            expected = 0;
        }

        CHECK_NOTHROW(mgr.push_scene_sync(std::make_unique <failing_scene>(flags)));

        CHECK(below.pauses == expected);
        CHECK(below.resumes == expected);
        frame(mgr, live); // the scene below is the top again
        CHECK(below.live_input == 1);
        mgr.finish();
    }

    TEST_CASE("a full-screen scene pauses and resumes the scene below") {
        probe_counts below;
        probe_counts top;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager pause resume");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, below, &live));
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, top, &live));
        CHECK(below.pauses == 1);
        CHECK(below.resumes == 0);

        pop_now(mgr);

        CHECK(below.pauses == 1);
        CHECK(below.resumes == 1);
        frame(mgr, live); // the resumed scene is the top again: updated with live input
        CHECK(below.updates == 1);
        CHECK(below.live_input == 1);
        mgr.finish();
    }

    TEST_CASE("an overlay with no flags neither pauses nor resumes the scene below") {
        probe_counts below;
        probe_counts top;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager overlay");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, below, &live));
        mgr.push_scene_sync(std::make_unique <flagged_scene>(scene_flags::none, top, &live));
        pop_now(mgr);

        CHECK(below.pauses == 0);
        CHECK(below.resumes == 0);
        mgr.finish();
    }
}

// passes_input: the scene sees the live input and the events and passes them on to the scene
// below. Input runs from the top down through such scenes, stops at the first scene without the
// flag, and never reaches a scene paused by pauses_below.
TEST_SUITE("neutrino::scene scenes_manager passes_input") {
    TEST_CASE("the live snapshot follows passes_input down the stack") {
        probe_counts game;
        probe_counts top;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager passes_input snapshot");
        scenes_manager& mgr = empty_manager();

        scene_flags top_flags = scene_flags::none;
        int game_updates = 0;
        int game_live = 0;
        SUBCASE("passes_input (an FPS counter): both see the live input") {
            top_flags = scene_flags::passes_input;
            game_updates = 1;
            game_live = 1;
        }
        SUBCASE("none (a busy spinner): the game updates with neutral input") {
            top_flags = scene_flags::none;
            game_updates = 1;
            game_live = 0;
        }
        SUBCASE("pauses_below (a pause menu): the game is not updated") {
            top_flags = scene_flags::pauses_below;
            game_updates = 0;
            game_live = 0;
        }
        SUBCASE("pauses_below | passes_input: a paused scene gets no input") {
            top_flags = scene_flags::pauses_below | scene_flags::passes_input;
            game_updates = 0;
            game_live = 0;
        }

        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, game, &live));
        mgr.push_scene_sync(std::make_unique <flagged_scene>(top_flags, top, &live));
        frame(mgr, live);

        CHECK(top.updates == 1);
        CHECK(top.live_input == 1);
        CHECK(game.updates == game_updates);
        CHECK(game.live_input == game_live);
        mgr.finish();
    }

    TEST_CASE("events follow the same chain as the live snapshot") {
        probe_counts game;
        probe_counts top;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager passes_input events");
        scenes_manager& mgr = empty_manager();

        scene_flags top_flags = scene_flags::none;
        int game_actions = 0;
        SUBCASE("passes_input: the scene below gets the event too") {
            top_flags = scene_flags::passes_input;
            game_actions = 1;
        }
        SUBCASE("none: the event stops at the top scene") {
            top_flags = scene_flags::none;
            game_actions = 0;
        }
        SUBCASE("pauses_below | passes_input: a paused scene gets no events") {
            top_flags = scene_flags::pauses_below | scene_flags::passes_input;
            game_actions = 0;
        }

        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, game, &live));
        mgr.push_scene_sync(std::make_unique <flagged_scene>(top_flags, top, &live));
        key_event(mgr);

        CHECK(top.actions == 1);
        CHECK(game.actions == game_actions);
        mgr.finish();
    }

    TEST_CASE("input stops at the first scene without passes_input") {
        // bottom: both, mid: none, top: passes_input. All three update (the only pauses_below is
        // at the bottom); top and mid see the live input and the event, bottom sees neither.
        probe_counts bottom;
        probe_counts mid;
        probe_counts top;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager passes_input chain");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, bottom, &live));
        mgr.push_scene_sync(std::make_unique <flagged_scene>(scene_flags::none, mid, &live));
        mgr.push_scene_sync(std::make_unique <flagged_scene>(scene_flags::passes_input, top, &live));

        frame(mgr, live);
        key_event(mgr);

        CHECK(top.live_input == 1);
        CHECK(mid.live_input == 1);
        CHECK(bottom.updates == 1);
        CHECK(bottom.live_input == 0);
        CHECK(top.actions == 1);
        CHECK(mid.actions == 1);
        CHECK(bottom.actions == 0);
        mgr.finish();
    }

    TEST_CASE("a scene that drops passes_input while handling an event still passes that event on") {
        // A closed console sees its toggle key and opens: the key that opened it still reaches
        // the game, the next one does not.
        probe_counts game;
        probe_counts console;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager passes_input read before delivery");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, game, &live));
        auto top = std::make_unique <flagged_scene>(scene_flags::passes_input, console, &live);
        top->set_flags_on_action(scene_flags::none);
        mgr.push_scene_sync(std::move(top));

        key_event(mgr);
        CHECK(console.actions == 1);
        CHECK(game.actions == 1);

        key_event(mgr);
        CHECK(console.actions == 2);
        CHECK(game.actions == 1);
        mgr.finish();
    }

    TEST_CASE("changing passes_input at run time reroutes the next frame") {
        probe_counts game;
        probe_counts top;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager passes_input at run time");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, game, &live));
        auto scene = std::make_unique <flagged_scene>(scene_flags::none, top, &live);
        flagged_scene* const raw = scene.get();
        mgr.push_scene_sync(std::move(scene));

        frame(mgr, live);
        CHECK(game.live_input == 0);

        raw->set_flags(scene_flags::passes_input);
        frame(mgr, live);
        CHECK(game.live_input == 1);

        raw->set_flags(scene_flags::none);
        frame(mgr, live);
        CHECK(game.live_input == 1);
        CHECK(top.live_input == 3);
        mgr.finish();
    }
}

// The pinned scene: one slot above the stack. push, pop and replace act below it; otherwise it is
// the topmost scene, with all its flags.
TEST_SUITE("neutrino::scene scenes_manager pinned scene") {
    using strings = std::vector <std::string>;

    TEST_CASE("the pinned scene stays above scenes pushed after it") {
        probe_counts game;
        probe_counts menu;
        probe_counts fps;
        strings drawn;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager pinned above pushes");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, game, &live, &drawn, "game"));
        pin_now(mgr, std::make_unique <flagged_scene>(scene_flags::passes_input, fps, &live, &drawn, "fps"));
        push_now(mgr, std::make_unique <flagged_scene>(scene_flags::none, menu, &live, &drawn, "menu"));

        mgr.render();
        CHECK(drawn == strings{"game", "menu", "fps"});

        // The pinned scene is the top of the input chain: it passes the input on to the menu.
        mgr.fixed_update(sim_duration{1.0f / 60.0f}, live);
        CHECK(fps.live_input == 1);
        CHECK(menu.live_input == 1);
        CHECK(game.live_input == 0);
        mgr.finish();
    }

    TEST_CASE("pop and replace act below the pinned scene") {
        probe_counts game;
        probe_counts menu;
        probe_counts level;
        probe_counts fps;
        strings drawn;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager pop and replace below the pin");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, game, &live, &drawn, "game"));
        pin_now(mgr, std::make_unique <flagged_scene>(scene_flags::passes_input, fps, &live, &drawn, "fps"));
        push_now(mgr, std::make_unique <flagged_scene>(scene_flags::none, menu, &live, &drawn, "menu"));

        pop_now(mgr);
        CHECK(menu.exits == 1);
        CHECK(fps.exits == 0);

        replace_now(mgr, std::make_unique <flagged_scene>(both, level, &live, &drawn, "level"));
        CHECK(game.exits == 1);
        CHECK(level.enters == 1);
        CHECK(fps.exits == 0);

        mgr.render();
        CHECK(drawn == strings{"level", "fps"});
        mgr.finish();
    }

    TEST_CASE("with only the pinned scene left, pop and replace are errors") {
        probe_counts fps;
        probe_counts other;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager nothing below the pin");
        scenes_manager& mgr = empty_manager();
        pin_now(mgr, std::make_unique <flagged_scene>(scene_flags::passes_input, fps, &live));

        CHECK_THROWS(pop_now(mgr));
        CHECK_THROWS(replace_now(mgr, std::make_unique <flagged_scene>(both, other, &live)));
        CHECK(fps.exits == 0);
        CHECK(other.enters == 0);
        CHECK_FALSE(mgr.empty());
        mgr.finish();
    }

    TEST_CASE("a pause menu pushed below the pinned scene pauses the active scene") {
        probe_counts game;
        probe_counts menu;
        probe_counts fps;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager pause below the pin");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, game, &live));
        pin_now(mgr, std::make_unique <flagged_scene>(scene_flags::passes_input, fps, &live));

        push_now(mgr, std::make_unique <flagged_scene>(scene_flags::pauses_below, menu, &live));
        CHECK(game.pauses == 1);
        CHECK(fps.pauses == 0);

        pop_now(mgr);
        CHECK(game.resumes == 1);
        CHECK(fps.resumes == 0);
        mgr.finish();
    }

    TEST_CASE("a pinned scene with pauses_below pauses the stack, and unpinning resumes it") {
        probe_counts game;
        probe_counts console;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager pausing pin");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, game, &live));
        pin_now(mgr, std::make_unique <flagged_scene>(scene_flags::pauses_below, console, &live));
        CHECK(game.pauses == 1);

        frame(mgr, live);
        CHECK(console.updates == 1);
        CHECK(console.live_input == 1);
        CHECK(game.updates == 0);

        unpin_now(mgr);
        CHECK(console.exits == 1);
        CHECK(game.resumes == 1);

        frame(mgr, live);
        CHECK(game.updates == 1);
        CHECK(game.live_input == 1);
        mgr.finish();
    }

    TEST_CASE("pinning replaces the pinned scene") {
        probe_counts game;
        probe_counts first;
        probe_counts second;
        strings drawn;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager pin replaces");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, game, &live, &drawn, "game"));
        pin_now(mgr, std::make_unique <flagged_scene>(scene_flags::none, first, &live, &drawn, "first"));
        pin_now(mgr, std::make_unique <flagged_scene>(scene_flags::none, second, &live, &drawn, "second"));

        CHECK(first.enters == 1);
        CHECK(first.exits == 1);
        CHECK(second.enters == 1);
        mgr.render();
        CHECK(drawn == strings{"game", "second"});
        mgr.finish();
    }

    TEST_CASE("unpinning with nothing pinned is a no-op") {
        probe_counts game;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager unpin nothing");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, game, &live));

        CHECK_NOTHROW(unpin_now(mgr));
        CHECK(game.exits == 0);
        CHECK_FALSE(mgr.empty());
        mgr.finish();
    }

    TEST_CASE("a pinned scene whose on_enter throws is rolled back") {
        class failing_scene : public base_scene {
            public:
                [[nodiscard]] scene_flags flags() const override { return scene_flags::pauses_below; }
                void on_enter() override { throw std::runtime_error("cannot enter"); }
                void fixed_update(sim_duration, const input_snapshot&) override {}
                void render() override {}
        };

        probe_counts game;
        probe_counts menu;
        strings drawn;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager pin rollback");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, game, &live, &drawn, "game"));

        CHECK_NOTHROW(pin_now(mgr, std::make_unique <failing_scene>()));
        CHECK(game.pauses == 1);
        CHECK(game.resumes == 1);

        // Nothing is pinned: a pushed scene is the top, and pop removes it.
        push_now(mgr, std::make_unique <flagged_scene>(scene_flags::none, menu, &live, &drawn, "menu"));
        mgr.render();
        CHECK(drawn == strings{"game", "menu"});
        pop_now(mgr);
        CHECK(menu.exits == 1);
        CHECK(game.exits == 0);
        mgr.finish();
    }

    TEST_CASE("resize notices reach the active scene and the pinned scene") {
        probe_counts game;
        probe_counts menu;
        probe_counts fps;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager pinned resize");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, game, &live));
        CHECK(game.resizes == 1);

        pin_now(mgr, std::make_unique <flagged_scene>(scene_flags::passes_input, fps, &live));
        CHECK(fps.resizes == 1);
        CHECK(game.resizes == 1);

        push_now(mgr, std::make_unique <flagged_scene>(scene_flags::none, menu, &live));
        CHECK(menu.resizes == 1); // sized when it becomes active, though the pinned scene is on top
        CHECK(fps.resizes == 1);

        mgr.notify_resize(dim{320, 200});
        CHECK(menu.resizes == 2);
        CHECK(fps.resizes == 2);
        CHECK(game.resizes == 1);

        pop_now(mgr);
        CHECK(game.resizes == 2); // active again
        CHECK(fps.resizes == 2);
        mgr.finish();
    }

    TEST_CASE("finish exits the pinned scene first and clears the slot") {
        std::vector <int> exited;
        probe_counts again;
        const input_snapshot live{};
        neutrino::test::test_application app("scenes_manager finish with a pin");
        scenes_manager& mgr = empty_manager();
        mgr.push_scene_sync(std::make_unique <recording_scene>(exited, 0, /*throw=*/false));
        pin_now(mgr, std::make_unique <recording_scene>(exited, 1, /*throw=*/false));

        mgr.finish();
        CHECK(exited == std::vector <int>{1, 0});
        CHECK(mgr.empty());

        // Nothing stays pinned: a pushed scene is the top again and pop removes it.
        mgr.push_scene_sync(std::make_unique <flagged_scene>(both, again, &live));
        CHECK_NOTHROW(pop_now(mgr));
        CHECK(again.exits == 1);
        CHECK(mgr.empty());
    }
}
