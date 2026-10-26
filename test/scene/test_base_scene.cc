//
// Tests for base_scene's default hooks (ergonomics roadmap P1.4).
//
// A scene must implement only fixed_update and render; everything else has a default. These pin
// both halves of that contract: the minimal scene is instantiable (no hook is accidentally pure
// again), and the defaults are the documented ones -- opaque, and raw events ignored. The opaque
// default is load-bearing: flipping it would silently keep every covered scene updating and
// drawing beneath each full-screen scene.
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
    TEST_CASE("a scene is opaque unless it says otherwise") {
        const minimal_scene scene;
        CHECK(scene.is_opaque());
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
