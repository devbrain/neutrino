//
// Tests for base_scene's default hooks.
//
// A scene must implement only fixed_update and render; everything else has a default. These pin
// both halves of that contract: the minimal scene is instantiable (no hook is accidentally pure
// again), and the defaults are the documented ones -- hiding and pausing everything below, and raw
// events ignored. The flags default is load-bearing: dropping either bit would silently keep every
// covered scene drawing or updating beneath each full-screen scene.
//
#include <doctest/doctest.h>

#include <type_traits>

#include <sdlpp/events/events.hh>
#include <neutrino/scene/base_scene.hh>

using namespace neutrino;

namespace {
    struct minimal_scene final : base_scene {
        void fixed_update(sim_duration, const input_snapshot&) override {}
        void render() override {}
    };

    static_assert(!std::is_abstract_v <minimal_scene>,
                  "a scene overriding only fixed_update and render must be instantiable");
}

TEST_SUITE("neutrino::scene base_scene defaults") {
    TEST_CASE("a scene hides and pauses the scenes below unless it says otherwise") {
        const minimal_scene scene;
        CHECK(scene.flags() == (scene_flags::hides_below | scene_flags::pauses_below));
    }

    TEST_CASE("the default event and lifecycle hooks are callable no-ops") {
        minimal_scene scene;
        SDL_Event raw{};
        raw.type = SDL_EVENT_KEY_DOWN;
        CHECK_NOTHROW(scene.handle_action(sdlpp::event(raw)));
        CHECK_NOTHROW(scene.on_enter());
        CHECK_NOTHROW(scene.on_pause());
        CHECK_NOTHROW(scene.on_resume());
        CHECK_NOTHROW(scene.on_resize(dim{640, 360}));
        CHECK_NOTHROW(scene.on_exit());
    }
}
