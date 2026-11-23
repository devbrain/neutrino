//
// Tests for the painter state functions (painter_state.hh): get_painter_state, push_state,
// pop_state and scoped_state, connected to what the draw calls use.
//
#include <doctest/doctest.h>

#include <SDL3/SDL.h>
#include <sdlpp/events/events.hh>

#include <neutrino/scene/base_scene.hh>
#include <neutrino/video/draw.hh>
#include <neutrino/video/globals.hh>
#include <neutrino/video/painter_state.hh>
#include <neutrino/video/texture.hh>

#include "scene/scenes_manager.hh"
#include "services/service_locator.hh"
#include "test_application.hh"

#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

using namespace neutrino;

namespace {
    [[nodiscard]] bool same(const painter_state& a, const painter_state& b) {
        return a.draw_color == b.draw_color && a.blend == b.blend && a.line == b.line
               && a.clip == b.clip;
    }

    // A state that differs from the defaults in every field.
    [[nodiscard]] painter_state unusual() {
        painter_state s;
        s.draw_color = colors::green;
        s.blend = blend_mode::add;
        s.line = line_style::dashed(2.0f, 3.0f);
        s.clip = rect{1, 1, 2, 2};
        return s;
    }

    [[nodiscard]] bool lit(const image& img, int x, int y, const color& c) {
        const auto px = img.get_pixel(x, y);
        REQUIRE(px.has_value());
        return px->r == c.r && px->g == c.g && px->b == c.b && px->a == c.a;
    }

    [[nodiscard]] bool empty(const image& img, int x, int y) {
        const auto px = img.get_pixel(x, y);
        REQUIRE(px.has_value());
        return px->a == 0;
    }

    // The application's scenes_manager with an empty stack, so each test builds its own.
    scenes_manager& empty_manager() {
        scenes_manager& mgr = service_locator::instance().get_scenes_manager();
        mgr.finish();
        REQUIRE(mgr.empty());
        return mgr;
    }

    // A scene whose every hook runs what the test gives it.
    class probe_scene final : public base_scene {
        public:
            explicit probe_scene(scene_flags flags = scene_flags::hides_below)
                : m_flags(flags) {
            }

            std::function <void()> enter = [] {};
            std::function <void()> update = [] {};
            std::function <void()> action = [] {};
            std::function <void()> draw = [] {};

            void on_enter() override { enter(); }
            void fixed_update(sim_duration, const input_snapshot&) override { update(); }
            void handle_action(const event&) override { action(); }
            void render() override { draw(); }
            [[nodiscard]] scene_flags flags() const override { return m_flags; }

        private:
            scene_flags m_flags;
    };

    constexpr color PINK{255, 0, 255, 255};

    void one_tick(scenes_manager& mgr) {
        mgr.fixed_update(sim_duration{1.0f / 60.0f}, input_snapshot{});
    }

    void key_event(scenes_manager& mgr) {
        SDL_Event raw{};
        raw.type = SDL_EVENT_KEY_DOWN;
        mgr.handle_action(sdlpp::event(raw));
    }
}

TEST_SUITE("neutrino::painter_state") {
    TEST_CASE("get_painter_state shows what the setters set") {
        test::test_application app("painter state get");
        set_draw_color(colors::red);
        set_draw_blend_mode(blend_mode::blend);
        set_line_style(line_style::dotted(5.0f));

        const painter_state s = get_painter_state();
        CHECK(s.draw_color == colors::red);
        CHECK(s.blend == blend_mode::blend);
        CHECK(s.line == line_style::dotted(5.0f));
        CHECK_FALSE(s.clip.has_value());
    }

    TEST_CASE("push_state saves without changing; pop_state undoes what came after") {
        test::test_application app("painter state push");
        set_draw_color(colors::red);
        const painter_state before = get_painter_state();

        push_state();
        CHECK(same(get_painter_state(), before));              // nothing changed yet
        set_draw_color(colors::blue);
        set_draw_blend_mode(blend_mode::add);
        set_line_style(line_style::dashed());
        CHECK(get_painter_state().draw_color == colors::blue);
        pop_state();

        CHECK(same(get_painter_state(), before));
    }

    TEST_CASE("push_state(s) replaces every field, clip included; pop_state restores them") {
        test::test_application app("painter state replace");
        set_draw_color(colors::red);
        const painter_state before = get_painter_state();

        push_state(unusual());
        CHECK(same(get_painter_state(), unusual()));
        CHECK(get_renderer().is_clip_enabled());
        pop_state();

        CHECK(same(get_painter_state(), before));
        CHECK_FALSE(get_renderer().is_clip_enabled());
    }

    TEST_CASE("pushes nest") {
        test::test_application app("painter state nest");
        set_draw_color(colors::red);
        push_state();
        set_draw_color(colors::green);
        push_state(painter_state{});
        CHECK(same(get_painter_state(), painter_state{}));     // the defaults
        pop_state();
        CHECK(get_painter_state().draw_color == colors::green);
        pop_state();
        CHECK(get_painter_state().draw_color == colors::red);
    }

    TEST_CASE("pop_state with nothing pushed fails") {
        test::test_application app("painter state empty");
        CHECK_THROWS(pop_state());
        push_state();
        pop_state();
        CHECK_THROWS(pop_state());
    }

    TEST_CASE("scoped_state restores at the end of the scope, also on an exception") {
        test::test_application app("painter state scope");
        set_draw_color(colors::red);
        {
            auto s = scoped_state();
            set_draw_color(colors::blue);
        }
        CHECK(get_painter_state().draw_color == colors::red);
        {
            auto s = scoped_state(unusual());
            CHECK(same(get_painter_state(), unusual()));
        }
        CHECK(get_painter_state().draw_color == colors::red);
        CHECK_FALSE(get_renderer().is_clip_enabled());

        CHECK_THROWS([] {
            auto s = scoped_state();
            set_draw_color(colors::blue);
            throw std::runtime_error("helper failed");
        }());
        CHECK(get_painter_state().draw_color == colors::red);
    }

    TEST_CASE("a moved scope ends once; a scope is replaced by ending it first") {
        // Assigning over a scope would end it after the new one had pushed, popping the new
        // save: so it cannot be assigned.
        static_assert(!std::is_move_assignable_v <painter_state_scope>);
        test::test_application app("painter state move");
        set_draw_color(colors::red);
        std::optional <painter_state_scope> held;
        held.emplace(scoped_state());
        {
            painter_state_scope moved = std::move(*held);      // takes the push
            held.reset();                                      // the moved-from scope ends nothing
            set_draw_color(colors::green);
        }
        CHECK(get_painter_state().draw_color == colors::red);
        CHECK_THROWS(pop_state());                             // popped once

        painter_state blue = get_painter_state();
        blue.draw_color = colors::blue;
        held.emplace(scoped_state());
        held.reset();                                          // the old scope first...
        held.emplace(scoped_state(blue));                      // ...then the new one
        CHECK(get_painter_state().draw_color == colors::blue); // in effect while held
        held.reset();
        CHECK(get_painter_state().draw_color == colors::red);
        CHECK_THROWS(pop_state());
    }

    TEST_CASE("a helper's colour and clip do not leak into the caller's drawing") {
        test::test_application app("painter state pixels");
        auto t = texture::create(dim{4, 4}, texture_access::target);
        REQUIRE(t.has_value());
        t->compose([] {
            set_draw_color(colors::blue);
            {
                auto s = scoped_state();                       // a helper
                set_draw_color(colors::red);
                draw_point(0, 0);
            }
            draw_point(1, 0);                                  // the caller's colour again
            {
                painter_state clipped = get_painter_state();
                clipped.clip = rect{0, 2, 2, 2};
                auto s = scoped_state(clipped);
                draw_rect_fill(rect{0, 2, 4, 2});              // only the clipped half lands
            }
            draw_point(3, 1);                                  // no clip any more
        }, colors::transparent);

        const image img = t->to_image();
        CHECK(lit(img, 0, 0, colors::red));
        CHECK(lit(img, 1, 0, colors::blue));
        CHECK(lit(img, 0, 2, colors::blue));
        CHECK(lit(img, 1, 3, colors::blue));
        CHECK(empty(img, 2, 2));
        CHECK(empty(img, 3, 3));
        CHECK(lit(img, 3, 1, colors::blue));
    }

    TEST_CASE("a clip set straight on the renderer survives neutrino's target switches") {
        // What a UI backend does: it clips the window itself, then draws, and its drawing may
        // bind and unbind neutrino textures (text caches, render textures, layers).
        test::test_application app("painter state own clip");
        auto window = texture::create(dim{8, 8}, texture_access::target);
        auto cache = texture::create(dim{4, 4}, texture_access::target);
        REQUIRE(window.has_value());
        REQUIRE(cache.has_value());
        auto outer = window->bind();
        const rect own{0, 0, 2, 2};
        REQUIRE(get_renderer().set_clip_rect(own));
        {
            auto inner = cache->bind();
            CHECK_FALSE(get_renderer().get_clip_rect <rect>()->has_value());   // a bound texture: none
        }
        CHECK(get_renderer().get_clip_rect <rect>().value() == own);           // kept
        push_state();
        set_draw_color(colors::red);
        pop_state();                                           // the clip was not part of it
        CHECK(get_renderer().get_clip_rect <rect>().value() == own);
    }

    TEST_CASE("a texture bound again does not keep the clip it held") {
        test::test_application app("painter state stale clip");
        auto t = texture::create(dim{4, 4}, texture_access::target);
        REQUIRE(t.has_value());
        {
            auto s = t->bind();
            REQUIRE(get_renderer().set_clip_rect(rect{0, 0, 1, 1}));
        }
        auto s = t->bind();
        CHECK_FALSE(get_renderer().get_clip_rect <rect>()->has_value());
    }

    TEST_CASE("the painter's clip leaves a target it was on once it is popped elsewhere") {
        test::test_application app("painter state clip popped inside");
        auto window = texture::create(dim{8, 8}, texture_access::target);
        auto t = texture::create(dim{4, 4}, texture_access::target);
        REQUIRE(window.has_value());
        REQUIRE(t.has_value());
        auto outer = window->bind();
        painter_state clipped = get_painter_state();
        clipped.clip = rect{1, 1, 2, 2};
        push_state(clipped);                                   // on the window
        CHECK(get_renderer().get_clip_rect <rect>().value() == *clipped.clip);
        {
            auto inner = t->bind();
            CHECK(get_renderer().get_clip_rect <rect>().value() == *clipped.clip);   // follows
            pop_state();                                       // popped on the texture
        }
        CHECK_FALSE(get_renderer().get_clip_rect <rect>()->has_value());          // gone from the window
    }

    TEST_CASE("a scope that outlives the application ends without a fault") {
        std::optional <painter_state_scope> held;
        {
            test::test_application app("painter state outlives");
            held.emplace(scoped_state());
        }                                                      // the harness's scene undoes the push
        CHECK_NOTHROW(held.reset());                           // nothing to pop: logged
    }
}

TEST_SUITE("neutrino::painter_state per scene") {
    TEST_CASE("a draw colour set in one scene does not reach another, from its render(), update or events") {
        test::test_application app("painter state per scene");
        scenes_manager& mgr = empty_manager();
        auto out = texture::create(dim{4, 4}, texture_access::target);
        REQUIRE(out.has_value());
        auto binding = out->bind();

        auto bottom = std::make_unique <probe_scene>(scene_flags::hides_below);
        auto top = std::make_unique <probe_scene>(scene_flags::passes_input);
        probe_scene* low = bottom.get();
        probe_scene* high = top.get();
        int checked = 0;
        const auto sees_defaults = [&] {
            CHECK(get_painter_state().draw_color == painter_state{}.draw_color);
            CHECK(get_painter_state().blend == painter_state{}.blend);
            ++checked;
        };
        low->enter = [] { set_draw_color(colors::red); };
        low->update = [] { set_draw_color(colors::green); };
        low->action = [] { set_draw_color(colors::blue); };
        low->draw = [] { set_draw_blend_mode(blend_mode::add); };
        high->enter = sees_defaults;
        high->update = sees_defaults;
        high->action = sees_defaults;
        high->draw = sees_defaults;
        mgr.push_scene_sync(std::move(bottom));
        mgr.push_scene_sync(std::move(top));

        one_tick(mgr);                             // the bottom scene's update runs first
        key_event(mgr);
        mgr.render();
        key_event(mgr);                            // after the bottom scene's render() and events
        one_tick(mgr);
        CHECK(checked == 6);

        // The bottom scene keeps what it set, across its calls.
        std::optional <painter_state> kept;
        low->draw = [&] { kept = get_painter_state(); };
        mgr.render();
        REQUIRE(kept.has_value());
        CHECK(kept->draw_color == colors::green);  // its last update set it after its last event
        CHECK(kept->blend == blend_mode::add);
        // And the test's own scene saw none of it.
        CHECK(get_painter_state().draw_color == painter_state{}.draw_color);
        mgr.finish();
    }

    TEST_CASE("a scene's painter state lasts from its on_enter to its frames") {
        test::test_application app("painter state from on_enter");
        scenes_manager& mgr = empty_manager();
        auto out = texture::create(dim{4, 4}, texture_access::target);
        REQUIRE(out.has_value());
        out->fill(colors::black);
        auto binding = out->bind();

        auto owned = std::make_unique <probe_scene>();
        owned->enter = [] { set_draw_color(PINK); };
        owned->draw = [] { draw_point(1, 1); };
        mgr.push_scene_sync(std::move(owned));
        mgr.render();
        binding.end();
        CHECK(lit(out->to_image(), 1, 1, PINK));
        mgr.finish();
    }

    TEST_CASE("a draw or a painter state function with no scene current fails") {
        test::test_application app("painter state no scene");
        auto t = texture::create(dim{4, 4}, texture_access::target);
        REQUIRE(t.has_value());
        app.leave_scene();                         // as in ready()

        CHECK_THROWS_WITH(set_draw_color(colors::red), doctest::Contains("no scene current"));
        CHECK_THROWS((void) get_draw_color());
        CHECK_THROWS(set_draw_blend_mode(blend_mode::blend));
        CHECK_THROWS(set_line_style(line_style::dashed()));
        CHECK_THROWS((void) get_painter_state());
        CHECK_THROWS(push_state());
        CHECK_THROWS(pop_state());
        CHECK_THROWS([] { auto s = scoped_state(); }());
        CHECK_THROWS(draw_point(0, 0));
        CHECK_THROWS(draw_rect_fill(rect{0, 0, 2, 2}, colors::red));
        CHECK_THROWS(draw_texture(*t));

        // Textures work anywhere; a bake fails at its first draw.
        CHECK_NOTHROW(t->fill(colors::red));
        CHECK_NOTHROW(t->compose([] {}, colors::blue));
        CHECK(lit(t->to_image(), 0, 0, colors::blue));
        CHECK_THROWS(t->compose([] { draw_point(0, 0); }));
    }

    TEST_CASE("a push left at the end of an update fails, after it is undone") {
        test::test_application app("painter state update push");
        scenes_manager& mgr = empty_manager();
        auto owned = std::make_unique <probe_scene>();
        probe_scene* scene = owned.get();
        scene->update = [] {
            push_state();
            set_draw_color(colors::red);
        };
        mgr.push_scene_sync(std::move(owned));
        CHECK_THROWS_WITH(one_tick(mgr), doctest::Contains("fixed_update ended with 1 push_state() not popped"));

        std::optional <painter_state> next;
        bool nothing_pushed = false;
        scene->update = [&] {
            next = get_painter_state();
            try {
                pop_state();
            } catch (...) {
                nothing_pushed = true;
            }
        };
        one_tick(mgr);
        REQUIRE(next.has_value());
        CHECK(next->draw_color == painter_state{}.draw_color);
        CHECK(nothing_pushed);
        mgr.finish();
    }

    TEST_CASE("state set straight on the renderer does not reach a neutrino draw") {
        test::test_application app("painter state native");
        auto t = texture::create(dim{4, 4}, texture_access::target);
        REQUIRE(t.has_value());
        constexpr color half_blue{0, 0, 255, 128};
        set_draw_color(half_blue);                 // blend none: written as it is
        t->compose([] {
            REQUIRE(get_renderer().set_draw_color(colors::red));
            REQUIRE(get_renderer().set_draw_blend_mode(sdlpp::blend_mode::blend));
            REQUIRE(get_renderer().set_line_style(sdlpp::line_style::dashed(1.0f, 1.0f)));
            draw_point(0, 0);
            draw_line(0, 1, 3, 1);
        }, colors::transparent);

        const image img = t->to_image();
        CHECK(lit(img, 0, 0, half_blue));
        for (int x = 0; x < 4; ++x) {
            CAPTURE(x);
            CHECK(lit(img, x, 1, half_blue));      // solid, not dashed
        }
    }

    TEST_CASE("a scope that ends after its push was undone pops nothing of another") {
        test::test_application app("painter state stale scope");
        scenes_manager& mgr = empty_manager();
        auto owned = std::make_unique <probe_scene>();
        probe_scene* scene = owned.get();
        std::optional <painter_state_scope> held;
        scene->update = [&] { held.emplace(scoped_state()); };
        mgr.push_scene_sync(std::move(owned));
        CHECK_THROWS(one_tick(mgr));               // the scope outlived the call: undone

        push_state();                              // the test's own scene's save
        set_draw_color(colors::red);
        CHECK_NOTHROW(held.reset());               // not this scene's save: nothing popped
        CHECK(get_painter_state().draw_color == colors::red);
        pop_state();
        CHECK(get_painter_state().draw_color == painter_state{}.draw_color);
        mgr.finish();
    }
}
