//
// Tests for the internal layer stack (painter.md §3.4): declare, push and target, so far.
//
#include <doctest/doctest.h>

#include <neutrino/video/draw.hh>
#include <neutrino/video/globals.hh>
#include <neutrino/video/painter_state.hh>
#include <neutrino/video/texture.hh>

#include "test_application.hh"
#include "services/service_access.hh"
#include "video/layer_stack.hh"
#include "video/target_geometry.hh"
#include "video/texture_allocator.hh"

using namespace neutrino;
using neutrino::detail::layer_stack;

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

    [[nodiscard]] bool pixel_is(const texture& t, int x, int y, const color& c) {
        const image img = t.to_image();
        const auto px = img.get_pixel(x, y);
        REQUIRE(px.has_value());
        return px->r == c.r && px->g == c.g && px->b == c.b && px->a == c.a;
    }

    [[nodiscard]] layer_settings sized(dim size) {
        layer_settings s;
        s.size = size;
        return s;
    }
}

TEST_SUITE("neutrino::detail::layer_stack") {
    TEST_CASE("push takes a target at the stack's size and binds it") {
        test::test_application app("layer stack push");
        const auto held = require_texture_allocator().stats().held;
        layer_stack s(dim{8, 8});
        const layer_id id = s.declare();
        CHECK(s.target(id) == nullptr);                       // nothing taken at declare

        s.push(id);
        const texture* t = s.target(id);
        REQUIRE(t != nullptr);
        CHECK(t->size() == dim{8, 8});
        CHECK(bound() == raw(t));
        CHECK(require_texture_allocator().stats().held == held + 1);
    }

    TEST_CASE("a layer's own size wins; a stack without a size uses the render size") {
        test::test_application app("layer stack size");
        layer_stack s(dim{8, 8});
        const layer_id own = s.declare(sized(dim{4, 2}));
        s.push(own);
        REQUIRE(s.target(own) != nullptr);
        CHECK(s.target(own)->size() == dim{4, 2});

        layer_stack unsized(dim{0, 0});
        const layer_id id = unsized.declare();
        unsized.push(id);
        REQUIRE(unsized.target(id) != nullptr);
        CHECK(unsized.target(id)->size() == render_size());
    }

    TEST_CASE("a layer starts transparent; a mod layer starts opaque white") {
        test::test_application app("layer stack clear colour");
        layer_stack s(dim{4, 4});
        const layer_id normal = s.declare();
        layer_settings mod_settings;
        mod_settings.composite.blend = blend_mode::mod;
        const layer_id mod = s.declare(mod_settings);

        s.push(normal);
        s.push(mod);
        REQUIRE(s.target(normal) != nullptr);
        REQUIRE(s.target(mod) != nullptr);
        CHECK(pixel_is(*s.target(normal), 1, 1, colors::transparent));
        CHECK(pixel_is(*s.target(mod), 1, 1, colors::white));
    }

    TEST_CASE("pushing a layer again continues drawing on it: no clear, no new texture") {
        test::test_application app("layer stack push again");
        layer_stack s(dim{4, 4});
        const layer_id id = s.declare();
        s.push(id);
        draw_rect_fill(rect{0, 0, 4, 4}, colors::red);
        const auto held = require_texture_allocator().stats().held;

        s.push(id);                                           // same frame, still pushed
        REQUIRE(s.target(id) != nullptr);
        CHECK(bound() == raw(s.target(id)));
        CHECK(pixel_is(*s.target(id), 2, 2, colors::red));
        CHECK(require_texture_allocator().stats().held == held);
    }

    TEST_CASE("a layer whose texture cannot be made is pushed with nothing bound, once") {
        test::test_application app("layer stack failed");
        layer_stack s(dim{4, 4});
        const layer_id id = s.declare(sized(dim{0, 8}));     // degenerate: kept, not replaced
        SDL_Texture* before = bound();
        const auto failures = draw_failure_count();

        CHECK_NOTHROW(s.push(id));
        CHECK(s.target(id) == nullptr);
        CHECK(bound() == before);
        CHECK(draw_failure_count() == failures + 1);

        CHECK_NOTHROW(s.push(id));                           // not retried on push
        CHECK(draw_failure_count() == failures + 1);
    }

    TEST_CASE("push and target reject another stack's layer and an empty id") {
        test::test_application app("layer stack reject");
        layer_stack a(dim{4, 4});
        layer_stack b(dim{4, 4});
        (void) a.declare();
        const layer_id other = b.declare();
        CHECK_THROWS(a.push(other));
        CHECK_THROWS((void) a.target(other));
        CHECK_THROWS(a.push(layer_id{}));
        CHECK_THROWS((void) a.target(layer_id{}));
    }

    TEST_CASE("destroying the stack binds again the target from before the first push") {
        test::test_application app("layer stack unwind");
        auto canvas = texture::create(dim{4, 4}, texture_access::target);
        REQUIRE(canvas.has_value());
        auto scope = canvas->bind();                          // stands in for the scene's canvas
        SDL_Texture* before = bound();
        REQUIRE(before == canvas->native().get());
        {
            layer_stack s(dim{4, 4});
            const layer_id first = s.declare();
            const layer_id second = s.declare();
            s.push(first);
            s.push(second);
            CHECK(bound() == raw(s.target(second)));
        }
        CHECK(bound() == before);
    }

    TEST_CASE("the first declare makes the base layer; base_layer() needs a layer") {
        test::test_application app("layer stack base");
        layer_stack s(dim{4, 4});
        CHECK_THROWS((void) s.base_layer());
        const layer_id first = s.declare();
        const layer_id base = s.base_layer();
        CHECK(base != first);
        CHECK(s.target(base) == nullptr);                     // no texture until a frame pushes it
        const layer_id second = s.declare();
        CHECK(s.base_layer() == base);                        // made once
        CHECK(second != first);
    }

    TEST_CASE("a frame pushes the base layer, cleared, under everything; end_frame pops it") {
        test::test_application app("layer stack frame");
        auto canvas = texture::create(dim{4, 4}, texture_access::target);
        REQUIRE(canvas.has_value());
        auto scope = canvas->bind();
        SDL_Texture* before = bound();

        layer_stack s(dim{4, 4});
        (void) s.declare();
        s.begin_frame(dim{4, 4});
        REQUIRE(s.target(s.base_layer()) != nullptr);
        CHECK(bound() == raw(s.target(s.base_layer())));
        draw_rect_fill(rect{0, 0, 4, 4}, colors::red);        // no layer pushed: onto the base
        s.end_frame();
        CHECK(bound() == before);
        CHECK(pixel_is(*s.target(s.base_layer()), 1, 1, colors::red));   // kept: pushed this frame

        s.begin_frame(dim{4, 4});                             // next frame: cleared again
        CHECK(pixel_is(*s.target(s.base_layer()), 1, 1, colors::transparent));
        s.end_frame();
    }

    TEST_CASE("pop binds the previous target again and leaves the painter state alone") {
        test::test_application app("layer stack pop");
        layer_stack s(dim{4, 4});
        const layer_id hud = s.declare();
        s.begin_frame(dim{4, 4});
        set_draw_color(colors::red);

        s.push(hud);
        CHECK(bound() == raw(s.target(hud)));
        CHECK(get_painter_state().draw_color == colors::red); // the push changed only the target
        set_draw_color(colors::blue);
        draw_point(0, 0);
        s.pop();

        CHECK(bound() == raw(s.target(s.base_layer())));
        CHECK(get_painter_state().draw_color == colors::blue);// a layer is where, not how (R39)
        CHECK(pixel_is(*s.target(hud), 0, 0, colors::blue));
        s.end_frame();
    }

    TEST_CASE("state and layer pushes are independent: interleaved pops restore each its own") {
        test::test_application app("layer stack independent");
        SDL_Texture* before = bound();
        layer_stack s(dim{4, 4});
        const layer_id hud = s.declare();
        s.begin_frame(dim{4, 4});
        SDL_Texture* base = bound();

        set_draw_color(colors::red);
        push_state();                                         // saves red
        set_draw_color(colors::green);
        s.push(hud);                                          // binds hud
        pop_state();                                          // red again, hud still bound
        CHECK(get_painter_state().draw_color == colors::red);
        CHECK(bound() == raw(s.target(hud)));
        s.pop();                                              // base again, still red
        CHECK(get_painter_state().draw_color == colors::red);
        CHECK(bound() == base);
        s.end_frame();
        CHECK(bound() == before);
    }

    TEST_CASE("the painter's clip follows the bound target: layers, pops and bind()") {
        test::test_application app("layer stack clip");
        layer_stack s(dim{8, 8});
        const layer_id hud = s.declare();
        s.begin_frame(dim{8, 8});
        painter_state clipped = get_painter_state();
        clipped.clip = rect{1, 1, 2, 2};
        push_state(clipped);
        CHECK(get_renderer().is_clip_enabled());              // on the base layer

        s.push(hud);                                          // a fresh target: the clip goes along
        CHECK(get_renderer().is_clip_enabled());
        CHECK(get_painter_state().clip == clipped.clip);
        draw_rect_fill(rect{0, 0, 8, 8}, colors::red);
        s.pop();
        CHECK(get_renderer().is_clip_enabled());              // back on the base layer

        pop_state();
        CHECK_FALSE(get_renderer().is_clip_enabled());
        CHECK_FALSE(get_painter_state().clip.has_value());

        s.push(hud);                                          // no clip now, on hud either
        CHECK_FALSE(get_renderer().is_clip_enabled());
        s.pop();
        s.end_frame();

        CHECK(pixel_is(*s.target(hud), 1, 1, colors::red));  // inside the clip
        CHECK(pixel_is(*s.target(hud), 4, 4, colors::transparent));
    }

    TEST_CASE("pop refuses the frame's base push and an empty stack; an explicit base push pops") {
        test::test_application app("layer stack pop refuse");
        layer_stack s(dim{4, 4});
        CHECK_THROWS(s.pop());                                // nothing pushed
        (void) s.declare();
        s.begin_frame(dim{4, 4});
        CHECK_THROWS(s.pop());                                // only the frame's own push
        s.push(s.base_layer());                               // current again, over nothing
        CHECK_NOTHROW(s.pop());
        s.end_frame();
    }

    TEST_CASE("pop fails while a bind() scope that outlived the layer is still bound") {
        test::test_application app("layer stack pop bind");
        auto other = texture::create(dim{2, 2}, texture_access::target);
        REQUIRE(other.has_value());
        layer_stack s(dim{4, 4});
        const layer_id id = s.declare();
        s.begin_frame(dim{4, 4});
        s.push(id);
        auto leaked = other->bind();
        CHECK_THROWS(s.pop());
        leaked.end();
        CHECK_NOTHROW(s.pop());                               // the failed pop changed nothing
        s.end_frame();
    }

    TEST_CASE("a frame that ends with a layer pushed fails, naming it, and unwinds") {
        test::test_application app("layer stack end pushed");
        SDL_Texture* before = bound();
        layer_stack s(dim{4, 4});
        const layer_id id = s.declare();
        s.begin_frame(dim{4, 4});
        s.push(id);
        CHECK_THROWS_WITH(s.end_frame(), doctest::Contains("still pushed"));
        CHECK(bound() == before);
        s.begin_frame(dim{4, 4});                             // the next frame starts cleanly
        s.end_frame();
    }

    TEST_CASE("abort_frame ends every push and binds the target from before the frame") {
        test::test_application app("layer stack abort");
        SDL_Texture* before = bound();
        layer_stack s(dim{4, 4});
        const layer_id a = s.declare();
        const layer_id b = s.declare();
        s.begin_frame(dim{4, 4});
        s.push(a);
        s.push(b);
        s.abort_frame();
        CHECK(bound() == before);
        s.begin_frame(dim{4, 4});
        s.end_frame();
    }

    TEST_CASE("an every-frame layer not pushed gives its target back; an on-demand one keeps it") {
        test::test_application app("layer stack give back");
        auto& allocator = require_texture_allocator();
        layer_stack s(dim{4, 4});
        const layer_id every = s.declare();
        layer_settings kept;
        kept.redraw = redraw_mode::on_demand;
        const layer_id demand = s.declare(kept);

        s.begin_frame(dim{4, 4});
        s.push(every);
        s.pop();
        s.push(demand);
        s.pop();
        s.end_frame();
        CHECK(s.target(every) != nullptr);
        CHECK(s.target(demand) != nullptr);
        const auto pooled = allocator.stats().pooled;

        s.begin_frame(dim{4, 4});                             // neither is pushed this frame
        s.end_frame();
        CHECK(s.target(every) == nullptr);                    // back in the pool
        CHECK(s.target(demand) != nullptr);
        CHECK(allocator.stats().pooled == pooled + 1);
    }

    TEST_CASE("begin_frame resizes the layers without a size of their own") {
        test::test_application app("layer stack resize");
        layer_stack s(dim{4, 4});
        const layer_id follows = s.declare();
        const layer_id fixed = s.declare(sized(dim{2, 2}));
        s.begin_frame(dim{4, 4});
        s.push(follows);
        s.pop();
        s.push(fixed);
        s.pop();
        s.end_frame();

        s.begin_frame(dim{8, 6});
        REQUIRE(s.target(follows) != nullptr);
        CHECK(s.target(follows)->size() == dim{8, 6});
        CHECK(s.target(s.base_layer())->size() == dim{8, 6});
        CHECK(s.target(fixed)->size() == dim{2, 2});
        s.end_frame();
    }

    TEST_CASE("a layer mirrors the stack's presentation; one with its own size is drawn in its own pixels") {
        test::test_application app("layer stack presentation");
        detail::target_geometry doubled{dim{8, 8}};          // a 4 x 4 render space on 8 x 8 pixels
        doubled.logical = dim{4, 4};
        doubled.mode = sdlpp::logical_presentation::letterbox;
        layer_stack s(doubled);
        const layer_id follows = s.declare();
        const layer_id own = s.declare(sized(dim{8, 8}));

        s.push(follows);
        CHECK(render_size() == dim{4, 4});                    // the same inside as outside
        draw_rect_fill(rect{1, 1, 1, 1}, colors::red);        // one render unit: 2 x 2 pixels
        s.pop();
        s.push(own);
        draw_rect_fill(rect{1, 1, 1, 1}, colors::red);        // one pixel
        s.pop();

        REQUIRE(s.target(follows) != nullptr);
        REQUIRE(s.target(own) != nullptr);
        CHECK(s.target(follows)->size() == dim{8, 8});
        CHECK(pixel_is(*s.target(follows), 2, 2, colors::red));
        CHECK(pixel_is(*s.target(follows), 3, 3, colors::red));
        CHECK(pixel_is(*s.target(follows), 1, 1, colors::transparent));
        CHECK(pixel_is(*s.target(own), 1, 1, colors::red));
        CHECK(pixel_is(*s.target(own), 2, 2, colors::transparent));
    }

    TEST_CASE("without layers the frame hooks do nothing") {
        test::test_application app("layer stack no layers");
        SDL_Texture* before = bound();
        layer_stack s(dim{4, 4});
        s.begin_frame(dim{4, 4});
        CHECK(bound() == before);
        s.end_frame();
        CHECK(bound() == before);
    }
}
