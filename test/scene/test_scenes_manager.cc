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

#include <memory>
#include <stdexcept>
#include <vector>

#include <neutrino/scene/base_scene.hh>

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
            void handle_action(const sdlpp::event&) override {}
            [[nodiscard]] bool is_opaque() const override { return true; }

        private:
            std::vector <int>& m_log;
            int m_id;
            bool m_throw;
    };
} // namespace

TEST_SUITE("neutrino::scene scenes_manager shutdown") {
    // scenes_manager is a singleton (its ctor ENFORCEs that no other exists), so these drive the
    // one the test application already owns rather than building a second.
    TEST_CASE("finish drains the whole stack even when a scene's on_exit throws") {
        neutrino::test::test_application app("scenes_manager finish test");
        scenes_manager& mgr = service_locator::instance().get_scenes_manager();

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
        scenes_manager& mgr = service_locator::instance().get_scenes_manager();
        REQUIRE(mgr.empty());
        CHECK_NOTHROW(mgr.finish());
        CHECK(mgr.empty());
    }
}
