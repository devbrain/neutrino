//
// Tests for the painter state functions (painter_state.hh): get_painter_state, push_state,
// pop_state and scoped_state, connected to what the draw calls use.
//
#include <doctest/doctest.h>

#include <neutrino/video/draw.hh>
#include <neutrino/video/globals.hh>
#include <neutrino/video/painter_state.hh>
#include <neutrino/video/texture.hh>

#include "test_application.hh"

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
        }                                                      // the save is dropped, and logged
        CHECK_NOTHROW(held.reset());
    }
}
