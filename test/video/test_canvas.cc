//
// Tests for the internal canvas (painter.md §2.6, §3.1, §3.2; render_model.md R41, R42): its
// texture, clearing, redraw modes, the drawn flag, resizing, the target's presentation, and
// the copy into the target.
//
#include <doctest/doctest.h>

#include <limits>

#include <neutrino/video/draw.hh>
#include <neutrino/video/globals.hh>
#include <neutrino/video/texture.hh>

#include "test_application.hh"
#include "services/service_access.hh"
#include "video/canvas.hh"
#include "video/target_geometry.hh"
#include "video/texture_allocator.hh"

using namespace neutrino;
using neutrino::detail::canvas;
using neutrino::detail::target_geometry;

namespace {
    // The texture the renderer draws into now; null for the window.
    [[nodiscard]] SDL_Texture* bound() {
        const auto t = get_renderer().get_target();
        REQUIRE(t.has_value());
        return *t;
    }

    [[nodiscard]] SDL_Texture* raw(const texture* t) {
        return t ? t->native().get() : nullptr;
    }

    [[nodiscard]] color pixel(const texture& t, int x, int y) {
        const image img = t.to_image();
        const auto px = img.get_pixel(x, y);
        REQUIRE(px.has_value());
        return color{px->r, px->g, px->b, px->a};
    }

    [[nodiscard]] bool pixel_is(const texture& t, int x, int y, const color& c) {
        const color px = pixel(t, x, y);
        return px.r == c.r && px.g == c.g && px.b == c.b && px.a == c.a;
    }

    // A target of 8 x 8 pixels showing a 4 x 4 render space: one render unit is 2 x 2 pixels.
    [[nodiscard]] target_geometry doubled() {
        target_geometry g{dim{8, 8}};
        g.logical = dim{4, 4};
        g.mode = sdlpp::logical_presentation::letterbox;
        return g;
    }

    // A target texture to copy canvases into, filled with black.
    [[nodiscard]] texture make_output(dim size) {
        auto out = texture::create(size, texture_access::target);
        REQUIRE(out.has_value());
        out->fill(colors::black);
        return std::move(*out);
    }

    // One frame of a scene that draws a red square at (0, 0, 2, 2) on the canvas.
    void draw_frame(canvas& c, const target_geometry& g) {
        c.begin_frame(g);
        {
            auto scope = c.bind();
            REQUIRE(scope.has_value());
            draw_rect_fill(rect{0, 0, 2, 2}, colors::red);
            c.note_drawn();
        }
        c.end_frame();
    }
}

TEST_SUITE("neutrino::detail::canvas") {
    TEST_CASE("a canvas takes nothing before its first bind") {
        test::test_application app("canvas before");
        const auto held = require_texture_allocator().stats().held;
        canvas c;
        CHECK(c.get() == nullptr);
        CHECK(c.size() == detail::texture_handle::EMPTY_DIM);
        CHECK_FALSE(c.bind().has_value());                    // no frame begun: nothing to bind
        CHECK_FALSE(c.drawn());
        CHECK_FALSE(c.needs_redraw());                        // every frame: never
        c.set_redraw(redraw_mode::on_demand);
        CHECK(c.needs_redraw());                              // on demand: nothing drawn yet

        c.begin_frame(dim{8, 4});                             // a frame alone takes nothing
        CHECK(c.get() == nullptr);
        c.end_frame();
        CHECK(require_texture_allocator().stats().held == held);
    }

    TEST_CASE("the first bind takes a transparent texture of the frame's size") {
        test::test_application app("canvas take");
        const auto held = require_texture_allocator().stats().held;
        canvas c;
        c.begin_frame(dim{8, 4});
        {
            auto scope = c.bind();
            REQUIRE(scope.has_value());
        }
        const texture* t = c.get();
        REQUIRE(t != nullptr);
        CHECK(t->size() == dim{8, 4});
        CHECK(c.size() == dim{8, 4});
        CHECK(pixel_is(*t, 3, 3, colors::transparent));
        CHECK(require_texture_allocator().stats().held == held + 1);
    }

    TEST_CASE("bind makes the canvas the target until its scope ends") {
        test::test_application app("canvas bind");
        canvas c;
        c.begin_frame(dim{4, 4});
        SDL_Texture* before = bound();
        {
            auto scope = c.bind();
            REQUIRE(scope.has_value());
            CHECK(bound() == raw(c.get()));
        }
        CHECK(bound() == before);
    }

    TEST_CASE("bind sets the frame's presentation: render space maps onto the canvas as onto its target") {
        test::test_application app("canvas presentation");
        canvas c;
        c.begin_frame(doubled());
        {
            auto scope = c.bind();
            REQUIRE(scope.has_value());
            CHECK(render_size() == dim{4, 4});                // the same inside as outside
            draw_rect_fill(rect{1, 1, 1, 1}, colors::red);    // one render unit: 2 x 2 pixels
        }
        REQUIRE(c.get() != nullptr);
        CHECK(c.get()->size() == dim{8, 8});
        CHECK(pixel_is(*c.get(), 2, 2, colors::red));
        CHECK(pixel_is(*c.get(), 3, 3, colors::red));
        CHECK(pixel_is(*c.get(), 1, 1, colors::transparent));
        CHECK(pixel_is(*c.get(), 4, 4, colors::transparent));
    }

    TEST_CASE("an every-frame canvas is cleared at its first bind in a frame, not at the next") {
        test::test_application app("canvas every frame");
        canvas c;
        draw_frame(c, dim{4, 4});
        REQUIRE(c.get() != nullptr);
        CHECK(pixel_is(*c.get(), 1, 1, colors::red));

        c.begin_frame(dim{4, 4});
        {
            auto scope = c.bind();                            // first bind: cleared
            CHECK(pixel_is(*c.get(), 1, 1, colors::transparent));
            draw_rect_fill(rect{0, 0, 2, 2}, colors::green);
        }
        {
            auto scope = c.bind();                            // second bind: kept
            CHECK(pixel_is(*c.get(), 1, 1, colors::green));
        }
        c.end_frame();
    }

    TEST_CASE("an on-demand canvas keeps its contents between frames") {
        test::test_application app("canvas on demand");
        canvas c;
        c.set_redraw(redraw_mode::on_demand);
        CHECK(c.redraw() == redraw_mode::on_demand);
        draw_frame(c, dim{4, 4});

        c.begin_frame(dim{4, 4});
        {
            auto scope = c.bind();
            REQUIRE(scope.has_value());
            CHECK(pixel_is(*c.get(), 1, 1, colors::red));
        }
        c.end_frame();
    }

    TEST_CASE("an on-demand canvas needs a redraw until a frame draws on it") {
        test::test_application app("canvas needs redraw");
        canvas c;
        c.set_redraw(redraw_mode::on_demand);

        c.begin_frame(dim{4, 4});                             // a frame that draws nothing
        CHECK(c.needs_redraw());
        c.end_frame();
        CHECK(c.needs_redraw());

        draw_frame(c, dim{4, 4});
        CHECK_FALSE(c.needs_redraw());

        c.begin_frame(dim{4, 4});                             // kept: nothing to redraw
        CHECK_FALSE(c.needs_redraw());
        c.end_frame();
        CHECK_FALSE(c.needs_redraw());
    }

    TEST_CASE("an every-frame canvas never needs a redraw") {
        test::test_application app("canvas every frame redraw");
        canvas c;
        c.begin_frame(dim{4, 4});
        CHECK_FALSE(c.needs_redraw());
        c.end_frame();
        draw_frame(c, dim{4, 4});
        CHECK_FALSE(c.needs_redraw());
    }

    TEST_CASE("drawn() lasts from note_drawn() to the next begin_frame()") {
        test::test_application app("canvas drawn");
        canvas c;
        c.begin_frame(dim{4, 4});
        CHECK_FALSE(c.drawn());
        c.note_drawn();
        CHECK(c.drawn());
        c.end_frame();
        CHECK(c.drawn());                                     // the scene manager reads it now
        c.begin_frame(dim{4, 4});
        CHECK_FALSE(c.drawn());
        c.end_frame();
        CHECK_FALSE(c.drawn());
    }

    TEST_CASE("a new size gives a new texture, cleared, that needs a redraw") {
        test::test_application app("canvas resize");
        canvas c;
        c.set_redraw(redraw_mode::on_demand);
        draw_frame(c, dim{4, 4});
        REQUIRE_FALSE(c.needs_redraw());

        c.begin_frame(dim{6, 3});
        REQUIRE(c.get() != nullptr);
        CHECK(c.get()->size() == dim{6, 3});
        CHECK(pixel_is(*c.get(), 1, 1, colors::transparent));
        CHECK(c.needs_redraw());
        c.end_frame();
    }

    TEST_CASE("a canvas whose texture cannot be made binds nothing; another size retries") {
        test::test_application app("canvas failed");
        const auto failures = draw_failure_count();
        canvas c;
        c.begin_frame(dim{0, 4});                             // degenerate
        SDL_Texture* before = bound();
        CHECK_FALSE(c.bind().has_value());
        CHECK(draw_failure_count() == failures + 1);
        CHECK(c.get() == nullptr);
        CHECK(c.size() == dim{0, 4});                         // the size it asked for
        CHECK(bound() == before);                             // drawing lands below
        c.end_frame();

        c.begin_frame(dim{0, 4});                             // the same size: not retried
        CHECK_FALSE(c.bind().has_value());
        CHECK(draw_failure_count() == failures + 1);
        c.end_frame();

        c.begin_frame(dim{4, 4});                             // another size: retried
        REQUIRE(c.get() != nullptr);
        CHECK(c.get()->size() == dim{4, 4});
        c.end_frame();
    }

    TEST_CASE("release gives the texture back; the next bind takes one again") {
        test::test_application app("canvas release");
        auto& allocator = require_texture_allocator();
        canvas c;
        draw_frame(c, dim{4, 4});
        REQUIRE(c.get() != nullptr);
        const auto pooled = allocator.stats().pooled;

        c.release();
        CHECK(c.get() == nullptr);
        CHECK(allocator.stats().pooled == pooled + 1);

        draw_frame(c, dim{4, 4});
        REQUIRE(c.get() != nullptr);
        CHECK(allocator.stats().pooled == pooled);            // the pooled one, reused
        CHECK(pixel_is(*c.get(), 1, 1, colors::red));
    }

    TEST_CASE("a canvas is copied with blend until told otherwise") {
        test::test_application app("canvas blend");
        canvas c;
        CHECK(c.blend() == blend_mode::blend);
        c.set_blend(blend_mode::none);
        const canvas& view = c;
        CHECK(view.blend() == blend_mode::none);
    }

    TEST_CASE("destroying a canvas gives its texture back to the pool") {
        test::test_application app("canvas give back");
        auto& allocator = require_texture_allocator();
        const auto pooled = allocator.stats().pooled;
        {
            canvas c;
            draw_frame(c, dim{4, 4});
            REQUIRE(c.get() != nullptr);
        }
        CHECK(allocator.stats().pooled == pooled + 1);
    }

    TEST_CASE("a moved canvas keeps its texture and settings") {
        test::test_application app("canvas move");
        canvas a;
        a.set_redraw(redraw_mode::on_demand);
        a.set_blend(blend_mode::none);
        draw_frame(a, dim{4, 4});
        const texture* t = a.get();
        REQUIRE(t != nullptr);

        canvas b(std::move(a));
        CHECK(b.get() == t);
        CHECK(b.redraw() == redraw_mode::on_demand);
        CHECK(b.blend() == blend_mode::none);
        CHECK(a.get() == nullptr);                            // NOLINT(bugprone-use-after-move)
    }
}

TEST_SUITE("neutrino::detail::apply_presentation") {
    TEST_CASE("a presentation is set when it changes, and setting it again keeps it") {
        test::test_application app("apply presentation");
        texture out = make_output(dim{8, 8});
        auto scope = out.bind();
        target_geometry g{dim{8, 8}};
        g.logical = dim{4, 4};
        g.mode = sdlpp::logical_presentation::letterbox;
        detail::apply_presentation(g);
        detail::apply_presentation(g);                        // the same: skipped
        auto lp = get_renderer().get_logical_presentation();
        REQUIRE(lp.has_value());
        CHECK(lp->width == 4);
        CHECK(lp->mode == sdlpp::logical_presentation::letterbox);
        detail::apply_presentation(target_geometry{dim{8, 8}});   // off
        lp = get_renderer().get_logical_presentation();
        REQUIRE(lp.has_value());
        CHECK(lp->mode == sdlpp::logical_presentation::disabled);
    }
}

TEST_SUITE("neutrino::detail::draw_canvases") {
    TEST_CASE("a canvas that mirrors the target lands 1:1 on it") {
        test::test_application app("canvases copy");
        texture out = make_output(dim{8, 8});
        auto scope = out.bind();
        canvas c;
        draw_frame(c, detail::bound_target_geometry());
        canvas* list[] = {&c};
        detail::draw_canvases(list);
        CHECK(pixel_is(out, 1, 1, colors::red));
        CHECK(pixel_is(out, 2, 2, colors::black));
    }

    TEST_CASE("canvases go in order, the first at the bottom; an undrawn every-frame one is skipped") {
        test::test_application app("canvases order");
        texture out = make_output(dim{8, 8});
        auto scope = out.bind();
        const target_geometry g = detail::bound_target_geometry();
        canvas below;
        draw_frame(below, g);                                 // red at (0, 0, 2, 2)
        canvas above;
        above.begin_frame(g);
        {
            auto bind = above.bind();
            draw_rect_fill(rect{1, 1, 2, 2}, colors::green);
            above.note_drawn();
        }
        above.end_frame();
        canvas idle;
        idle.begin_frame(g);
        {
            auto bind = idle.bind();
            draw_rect_fill(rect{0, 0, 8, 8}, colors::blue);   // drawn, but not noted
        }
        idle.end_frame();

        canvas* list[] = {&below, nullptr, &above, &idle};
        detail::draw_canvases(list);
        CHECK(pixel_is(out, 0, 0, colors::red));
        CHECK(pixel_is(out, 1, 1, colors::green));            // above wins where they overlap
        CHECK(pixel_is(out, 5, 5, colors::black));            // the idle canvas was skipped
    }

    TEST_CASE("drawing the canvases leaves the target's own clip as it was") {
        test::test_application app("canvases own clip");
        texture out = make_output(dim{8, 8});
        auto scope = out.bind();
        const target_geometry g = detail::bound_target_geometry();
        canvas c;
        draw_frame(c, g);
        const rect own{0, 0, 3, 3};
        REQUIRE(get_renderer().set_clip_rect(own));            // set straight on the renderer
        canvas* list[] = {&c};
        detail::draw_canvases(list);
        CHECK(get_renderer().get_clip_rect <rect>().value() == own);
    }

    TEST_CASE("a canvas copied with none overwrites the target; with blend it is blended") {
        test::test_application app("canvases blend");
        texture out = make_output(dim{8, 8});
        auto scope = out.bind();
        const target_geometry g = detail::bound_target_geometry();
        canvas c;
        c.begin_frame(g);
        {
            auto bind = c.bind();
            set_draw_blend_mode(blend_mode::blend);
            draw_rect_fill(rect{0, 0, 8, 8}, color{255, 0, 0, 128});
            set_draw_blend_mode(blend_mode::none);
            c.note_drawn();
        }
        c.end_frame();
        out.fill(colors::blue);
        canvas* list[] = {&c};
        detail::draw_canvases(list);                          // blend: half red over blue
        const color blended = pixel(out, 1, 1);
        CHECK(blended.r >= 126);
        CHECK(blended.r <= 129);
        CHECK(blended.b >= 126);
        CHECK(blended.b <= 129);

        out.fill(colors::blue);
        c.set_blend(blend_mode::none);
        detail::draw_canvases(list);                          // none: the canvas, blue gone
        const color copied = pixel(out, 1, 1);
        CHECK(copied.b == 0);
        CHECK(copied.r >= 126);
        CHECK(copied.r <= 129);
    }

    TEST_CASE("the target's presentation is off only while copying; the copy lands 1:1") {
        test::test_application app("canvases presentation");
        texture out = make_output(dim{8, 8});
        auto scope = out.bind();
        REQUIRE(get_renderer().set_logical_presentation(4, 4, sdlpp::logical_presentation::letterbox));
        const target_geometry g = detail::bound_target_geometry();
        REQUIRE(g == doubled());

        canvas c;
        c.begin_frame(g);
        {
            auto bind = c.bind();
            draw_rect_fill(rect{0, 0, 1, 1}, colors::red);    // 2 x 2 pixels
            c.note_drawn();
        }
        c.end_frame();
        canvas* list[] = {&c};
        detail::draw_canvases(list);

        CHECK(pixel_is(out, 0, 0, colors::red));              // render unit (0, 0): 2 x 2 pixels
        CHECK(pixel_is(out, 1, 1, colors::red));
        CHECK(pixel_is(out, 2, 2, colors::black));
        CHECK(detail::bound_target_geometry() == g);          // put back
    }
}
