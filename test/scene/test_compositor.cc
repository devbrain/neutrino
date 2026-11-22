//
// Tests for combining a scene's layers onto its canvas: the default compositor, a scene's own
// compositor through composition, and the placement of a layer by its composite settings
// (painter.md §2.7–2.9, §3.4, §3.5; render_model.md R43).
//
#include <doctest/doctest.h>

#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include <SDL3/SDL.h>
#include <sdlpp/events/events.hh>
#include <neutrino/scene/scene_transitions.hh>
#include <neutrino/scene/base_scene.hh>
#include <neutrino/video/composition.hh>
#include <neutrino/video/draw.hh>
#include <neutrino/video/globals.hh>
#include <neutrino/video/layer_ops.hh>
#include <neutrino/video/painter_state.hh>
#include <neutrino/video/screenshot.hh>
#include <neutrino/video/texture.hh>

#include "scene/scene_context_access.hh"
#include "scene/scene_frame.hh"
#include "scene/scenes_manager.hh"
#include "services/service_locator.hh"
#include "video/canvas.hh"
#include "video/target_geometry.hh"
#include "video/composite_draw.hh"
#include "video/layer_stack.hh"
#include "test_application.hh"

using namespace neutrino;
using neutrino::detail::scene_context_access;
using neutrino::detail::scene_frame;

namespace {
    constexpr color RED{255, 0, 0, 255};
    constexpr color GREEN{0, 255, 0, 255};
    constexpr color BLUE{0, 0, 255, 255};
    constexpr color BLACK{0, 0, 0, 255};

    [[nodiscard]] SDL_Texture* bound() {
        return SDL_GetRenderTarget(get_renderer().get());
    }

    // Sets the render shortcuts for one test and puts them back after it (R41, R44).
    struct shortcuts_set {
        neutrino::detail::render_shortcuts saved = neutrino::detail::shortcuts();
        shortcuts_set(bool into_target, bool base_as_canvas) {
            neutrino::detail::shortcuts() = {into_target, base_as_canvas};
        }
        ~shortcuts_set() { neutrino::detail::shortcuts() = saved; }
        shortcuts_set(const shortcuts_set&) = delete;
        shortcuts_set& operator=(const shortcuts_set&) = delete;
    };

    // The application's scenes_manager with an empty stack.
    scenes_manager& empty_manager() {
        scenes_manager& mgr = service_locator::instance().get_scenes_manager();
        mgr.finish();
        REQUIRE(mgr.empty());
        return mgr;
    }

    // A target texture standing in for the window, filled with black.
    [[nodiscard]] texture make_output(dim size) {
        auto out = texture::create(size, texture_access::target);
        REQUIRE(out.has_value());
        out->fill(colors::black);
        return std::move(*out);
    }

    [[nodiscard]] color pixel(const texture& t, int x, int y) {
        const image img = t.to_image();
        const auto px = img.get_pixel(x, y);
        REQUIRE(px.has_value());
        return color{px->r, px->g, px->b, px->a};
    }

    [[nodiscard]] color pixel_of(const image& img, int x, int y) {
        const auto px = img.get_pixel(x, y);
        REQUIRE(px.has_value());
        return color{px->r, px->g, px->b, px->a};
    }

    [[nodiscard]] bool near(std::uint8_t v, int expected) {
        return v >= expected - 1 && v <= expected + 1;
    }

    // A scene whose layers the test declares and whose render() the test writes. Full-screen
    // unless told otherwise.
    class layer_scene : public base_scene {
        public:
            explicit layer_scene(scene_flags flags = scene_flags::hides_below)
                : m_flags(flags) {
            }

            [[nodiscard]] layer_id add(const layer_settings& s = {}) { return declare_layer(s); }
            [[nodiscard]] layer_style& style(layer_id id) { return style_of(id); }

            [[nodiscard]] layer_id base() const {
                const detail::layer_stack* s = scene_context_access::of(const_cast <layer_scene*>(this));
                REQUIRE(s != nullptr);
                return s->base_layer();
            }

            std::function <void()> draw;

            void fixed_update(sim_duration, const input_snapshot&) override {}

            void render() override {
                if (draw) {
                    draw();
                }
            }

            [[nodiscard]] scene_flags flags() const override { return m_flags; }

        private:
            scene_flags m_flags;
    };

    // Fills @p area of layer @p id; @p mode for translucent colours.
    void paint(layer_id id, rect area, color c, blend_mode mode = blend_mode::none) {
        push_layer(id);
        set_draw_blend_mode(mode);
        draw_rect_fill(area, c);
        pop_layer();
    }

    // A scene without layers that fills its whole render area.
    class plain_scene : public base_scene {
        public:
            plain_scene(color fill, scene_flags flags)
                : m_fill(fill), m_flags(flags) {
            }

            void fixed_update(sim_duration, const input_snapshot&) override {}

            void render() override {
                const dim size = render_size();
                draw_rect_fill(rect{0, 0, size.width, size.height}, m_fill);
            }

            [[nodiscard]] scene_flags flags() const override { return m_flags; }

        private:
            color m_fill;
            scene_flags m_flags;
    };

    // Everything a test needs: an 8×8 black target bound in place of the window, and a
    // layered scene on an empty stack.
    struct fixture {
        test::test_application app{"compositor"};
        scenes_manager& mgr{empty_manager()};
        texture out{make_output(dim{8, 8})};
        renderer_target_scope binding{out.bind()};
        layer_scene* scene{nullptr};

        explicit fixture(scene_flags flags = scene_flags::hides_below) {
            auto owned = std::make_unique <layer_scene>(flags);
            scene = owned.get();
            mgr.push_scene_sync(std::move(owned));
        }

        ~fixture() {
            mgr.finish();
        }

        void frame() {
            out.fill(colors::black);           // the application clears the window each frame
            mgr.render();
        }

        /// The pixel as the screen shows it: the target stands in for the window, which has no
        /// alpha, and a full-screen scene's canvas is copied with none (painter.md §2.3).
        [[nodiscard]] color at(int x, int y) const {
            const color c = pixel(out, x, y);
            return color{c.r, c.g, c.b, 255};
        }
    };
}

TEST_SUITE("neutrino::compositor") {
    TEST_CASE("the layers are combined in declaration order, whatever order they were drawn in") {
        fixture f;
        const layer_id low = f.scene->add();
        const layer_id high = f.scene->add();
        f.scene->draw = [&] {
            paint(high, rect{0, 0, 4, 4}, BLUE);
            paint(low, rect{0, 0, 8, 8}, RED);
        };
        f.frame();
        CHECK(f.at(1, 1) == BLUE);
        CHECK(f.at(6, 6) == RED);
    }

    TEST_CASE("drawing with no layer pushed lands on the base layer, under every declared layer") {
        fixture f;
        const layer_id a = f.scene->add();
        f.scene->draw = [&] {
            paint(a, rect{0, 0, 4, 8}, RED);
            draw_rect_fill(rect{0, 0, 8, 8}, GREEN);   // after the layer, still under it
        };
        f.frame();
        CHECK(f.at(1, 1) == RED);
        CHECK(f.at(6, 1) == GREEN);
    }

    TEST_CASE("an every-frame layer not pushed shows nothing; an on-demand layer survives the frame") {
        fixture f;
        const layer_id every = f.scene->add();
        const layer_id kept = f.scene->add({.redraw = redraw_mode::on_demand});
        bool first = true;
        f.scene->draw = [&] {
            if (first) {
                paint(every, rect{0, 0, 4, 4}, RED);
                paint(kept, rect{4, 4, 4, 4}, BLUE);
            }
        };
        f.frame();
        CHECK(f.at(1, 1) == RED);
        CHECK(f.at(5, 5) == BLUE);

        first = false;
        f.frame();
        CHECK(f.at(1, 1) == BLACK);                // not pushed: not shown, and the canvas was cleared
        CHECK(f.at(5, 5) == BLUE);                 // kept
    }

    TEST_CASE("a compositor of the scene's own draws a layer twice and hides another") {
        fixture f;
        const layer_id a = f.scene->add();
        const layer_id hidden = f.scene->add();
        f.scene->draw = [&] {
            paint(a, rect{0, 0, 2, 2}, RED);
            paint(hidden, rect{0, 0, 8, 8}, BLUE);
        };
        f.scene->set_compositor([&](composition& c) {
            c.draw_layer(a);
            c.draw_layer(a, {.offset = {4, 4}});
        });
        f.frame();
        CHECK(f.at(1, 1) == RED);
        CHECK(f.at(5, 5) == RED);
        CHECK(f.at(3, 3) == BLACK);                // the blue layer is not drawn
    }

    TEST_CASE("composition lists the base layer, then the declared ones; draw_layers takes a range") {
        fixture f;
        const layer_id a = f.scene->add();
        const layer_id b = f.scene->add();
        const layer_id c3 = f.scene->add();
        std::vector <layer_id> seen;
        f.scene->draw = [&] {
            paint(a, rect{0, 0, 8, 8}, RED);
            paint(b, rect{0, 0, 4, 8}, GREEN);
            paint(c3, rect{0, 0, 2, 8}, BLUE);
        };
        f.scene->set_compositor([&](composition& c) {
            seen.assign(c.layers().begin(), c.layers().end());
            c.draw_layers(a, b);                   // not c3
            CHECK(c.texture(c3) != nullptr);
            CHECK(c.has_content(c3));
        });
        f.frame();
        REQUIRE(seen.size() == 4);
        CHECK(seen[0] == f.scene->base());
        CHECK(seen[1] == a);
        CHECK(seen[3] == c3);
        CHECK(f.at(1, 1) == GREEN);
        CHECK(f.at(6, 1) == RED);
    }

    TEST_CASE("set_compositor(default_compositor) puts the default back; an empty one is refused") {
        fixture f;
        using fn = void (*)(composition&);
        REQUIRE(f.scene->get_compositor().target <fn>() != nullptr);
        CHECK(*f.scene->get_compositor().target <fn>() == &default_compositor);
        f.scene->set_compositor([](composition&) {});
        CHECK(f.scene->get_compositor().target <fn>() == nullptr);
        f.scene->set_compositor(default_compositor);
        REQUIRE(f.scene->get_compositor().target <fn>() != nullptr);
        CHECK(*f.scene->get_compositor().target <fn>() == &default_compositor);
        CHECK_THROWS_WITH(f.scene->set_compositor(compositor{}), doctest::Contains("empty compositor"));
    }

    TEST_CASE("destination stretches a layer with its own size onto a rectangle: brix's plasma") {
        fixture f;
        const layer_id cells = f.scene->add({.size = dim{2, 2}, .style = {.destination = rect{2, 2, 4, 4}}});
        f.scene->draw = [&] {
            push_layer(cells);                     // in the layer's own pixels: one per cell
            draw_rect_fill(rect{0, 0, 1, 1}, RED);
            draw_rect_fill(rect{1, 1, 1, 1}, BLUE);
            pop_layer();
        };
        f.frame();
        CHECK(f.at(2, 2) == RED);                  // each texel a 2×2 block, hard-edged
        CHECK(f.at(3, 3) == RED);
        CHECK(f.at(4, 4) == BLUE);
        CHECK(f.at(5, 5) == BLUE);
        CHECK(f.at(4, 2) == BLACK);                // transparent texels show what is below
        CHECK(f.at(1, 1) == BLACK);                // outside the destination
        CHECK(f.at(6, 6) == BLACK);
    }

    TEST_CASE("a layer with its own size and no destination covers the render area, not the bars") {
        fixture f;
        // 8×8 pixels, 4×2 render units: letterboxed into the rows 2..5, two pixels per unit.
        REQUIRE(get_renderer().set_logical_presentation(4, 2, sdlpp::logical_presentation::letterbox));
        const layer_id small = f.scene->add({.size = dim{2, 1}});
        f.scene->draw = [&] {
            push_layer(small);
            draw_rect_fill(rect{0, 0, 1, 1}, RED);
            draw_rect_fill(rect{1, 0, 1, 1}, BLUE);
            pop_layer();
        };
        f.frame();
        // Read back without the presentation: under it, a readback sees only the render area.
        REQUIRE(get_renderer().set_logical_presentation(0, 0, sdlpp::logical_presentation::disabled));
        CHECK(f.at(1, 1) == BLACK);                // the bar above
        CHECK(f.at(1, 2) == RED);
        CHECK(f.at(3, 5) == RED);
        CHECK(f.at(4, 2) == BLUE);
        CHECK(f.at(7, 5) == BLUE);
        CHECK(f.at(5, 6) == BLACK);                // the bar below
    }

    TEST_CASE("under a logical presentation a layer lands 1:1; its offset is in render units") {
        fixture f;
        REQUIRE(get_renderer().set_logical_presentation(4, 4, sdlpp::logical_presentation::letterbox));
        const layer_id a = f.scene->add({.style = {.offset = {1, 0}}});
        f.scene->draw = [&] { paint(a, rect{1, 1, 1, 1}, RED); };   // render unit (1, 1): pixels 2..3
        f.frame();
        CHECK(f.at(2, 2) == BLACK);
        CHECK(f.at(4, 2) == RED);                  // moved one render unit: two pixels
        CHECK(f.at(5, 3) == RED);
        CHECK(f.at(6, 2) == BLACK);
        const auto lp = get_renderer().get_logical_presentation();
        REQUIRE(lp.has_value());
        CHECK(lp->width == 4);                     // put back
        CHECK(lp->mode == sdlpp::logical_presentation::letterbox);
    }

    TEST_CASE("scale per axis about the centre; a pivot moves the centre; 0 draws nothing") {
        fixture f;
        const layer_id a = f.scene->add();
        layer_style s;
        f.scene->draw = [&] { paint(a, rect{0, 0, 8, 8}, RED); };
        f.scene->set_compositor([&](composition& c) { c.draw_layer(a, s); });

        s.scale = layer_scale{0.5f, 0.25f};        // 4×2 about (4, 4): columns 2..5, rows 3..4
        f.frame();
        CHECK(f.at(2, 3) == RED);
        CHECK(f.at(5, 4) == RED);
        CHECK(f.at(1, 3) == BLACK);
        CHECK(f.at(2, 2) == BLACK);
        CHECK(f.at(2, 5) == BLACK);

        s.scale = 0.5f;                            // one float: both axes
        s.pivot = point{0, 0};
        f.frame();
        CHECK(f.at(3, 3) == RED);
        CHECK(f.at(4, 4) == BLACK);

        s.scale = 0.0f;
        f.frame();
        CHECK(f.at(0, 0) == BLACK);
    }

    TEST_CASE("rotation about the centre and a flip mirror the layer") {
        fixture f;
        const layer_id a = f.scene->add();
        layer_style s;
        f.scene->draw = [&] { paint(a, rect{0, 0, 4, 8}, RED); };    // the left half
        f.scene->set_compositor([&](composition& c) { c.draw_layer(a, s); });

        s.rotation_degrees = 180.0f;
        f.frame();
        CHECK(f.at(6, 4) == RED);
        CHECK(f.at(1, 4) == BLACK);

        s.rotation_degrees = 0.0f;
        s.flip = flip_mode::horizontal;
        f.frame();
        CHECK(f.at(6, 4) == RED);
        CHECK(f.at(1, 4) == BLACK);

        s.flip = flip_mode::diagonal;
        CHECK_THROWS_WITH(f.frame(), doctest::Contains("diagonal"));
    }

    TEST_CASE("a source part stays where it was; with a destination it is stretched onto it") {
        fixture f;
        const layer_id a = f.scene->add();
        layer_style s{.source = rect{4, 0, 4, 8}};      // the right half
        f.scene->draw = [&] {
            paint(a, rect{0, 0, 4, 8}, RED);
            paint(a, rect{4, 0, 4, 8}, BLUE);
        };
        f.scene->set_compositor([&](composition& c) { c.draw_layer(a, s); });
        f.frame();
        CHECK(f.at(5, 1) == BLUE);
        CHECK(f.at(1, 1) == BLACK);

        s.destination = rect{0, 0, 8, 8};
        f.frame();
        CHECK(f.at(1, 1) == BLUE);
        CHECK(f.at(6, 6) == BLUE);
    }

    TEST_CASE("a layer that mirrors the canvas takes its source in render units") {
        fixture f;
        // 8×8 pixels, 4×2 render units: the picture in the rows 2..5, two pixels per unit.
        REQUIRE(get_renderer().set_logical_presentation(4, 2, sdlpp::logical_presentation::letterbox));
        const layer_id a = f.scene->add({.style = {.source = rect{0, 0, 2, 2}}});   // the left half
        f.scene->draw = [&] { paint(a, rect{0, 0, 4, 2}, RED); };
        f.frame();
        REQUIRE(get_renderer().set_logical_presentation(0, 0, sdlpp::logical_presentation::disabled));
        CHECK(f.at(1, 3) == RED);                  // the left half of the picture, in place
        CHECK(f.at(3, 5) == RED);
        CHECK(f.at(5, 3) == BLACK);                // the right half is not drawn
        CHECK(f.at(1, 1) == BLACK);                // nor the bar
    }

    TEST_CASE("half opacity halves colour as well as alpha; blended drawing leaves no dark fringe") {
        fixture f;
        const layer_id faded = f.scene->add({.style = {.opacity = 0.5f}});
        const layer_id translucent = f.scene->add();
        f.scene->draw = [&] {
            paint(faded, rect{0, 0, 4, 8}, RED);
            paint(translucent, rect{4, 0, 4, 8}, color{0, 0, 255, 128}, blend_mode::blend);
        };
        f.frame();
        const color half = f.at(1, 1);
        CHECK(near(half.r, 128));
        CHECK(half.g == 0);
        CHECK(half.b == 0);
        const color blue = f.at(6, 1);             // premultiplied once: 128, not 64
        CHECK(blue.r == 0);
        CHECK(near(blue.b, 128));
    }

    TEST_CASE("a mod layer leaves uncovered pixels unchanged; a mul layer sees only itself") {
        fixture f;
        const layer_id tint = f.scene->add({.style = {.blend = blend_mode::mod}});
        const layer_id shade = f.scene->add({.style = {.blend = blend_mode::mul}});
        f.scene->draw = [&] {
            draw_rect_fill(rect{0, 0, 8, 8}, GREEN);                 // the base layer
            paint(tint, rect{0, 0, 2, 8}, RED);                      // green × red: black
            paint(shade, rect{4, 0, 2, 8}, color{128, 128, 128, 255});
        };
        f.frame();
        CHECK(f.at(1, 1) == BLACK);
        CHECK(f.at(3, 1) == GREEN);                // neither layer covers it
        const color halved = f.at(5, 1);
        CHECK(halved.r == 0);
        CHECK(near(halved.g, 128));
        CHECK(f.at(7, 1) == GREEN);
    }

    TEST_CASE("a mod layer below full opacity fades its tint; at 0 it leaves the picture unchanged") {
        fixture f;
        const layer_id tint = f.scene->add({.style = {.blend = blend_mode::mod}});
        layer_style s{.blend = blend_mode::mod, .opacity = 0.5f};
        f.scene->draw = [&] {
            draw_rect_fill(rect{0, 0, 8, 8}, GREEN);                 // the base layer
            paint(tint, rect{0, 0, 4, 8}, RED);                      // green × red: black, at full
        };
        f.scene->set_compositor([&](composition& c) {
            c.draw_layer(c.layers()[0]);
            c.draw_layer(tint, s);
        });
        f.frame();
        const color half = f.at(1, 1);             // halfway to black
        CHECK(half.r == 0);
        CHECK(near(half.g, 128));
        CHECK(f.at(6, 1) == GREEN);                // the white part leaves it unchanged

        s.opacity = 0.0f;
        f.frame();
        CHECK(f.at(1, 1) == GREEN);
        CHECK(f.at(6, 1) == GREEN);
    }

    TEST_CASE("with letterbox bars a pivot is measured from the picture's corner, not the bar's") {
        fixture f;
        // 8×8 pixels, 4×2 render units: the picture in the rows 2..5.
        REQUIRE(get_renderer().set_logical_presentation(4, 2, sdlpp::logical_presentation::letterbox));
        const layer_id a = f.scene->add({.style = {.scale = 0.5f, .pivot = point{0, 0}}});
        f.scene->draw = [&] { paint(a, rect{0, 0, 4, 2}, RED); };   // the whole picture
        f.frame();
        REQUIRE(get_renderer().set_logical_presentation(0, 0, sdlpp::logical_presentation::disabled));
        CHECK(f.at(1, 2) == RED);                  // halved about the picture's top-left
        CHECK(f.at(3, 3) == RED);
        CHECK(f.at(1, 1) == BLACK);                // not about the bar's corner
        CHECK(f.at(1, 4) == BLACK);
        CHECK(f.at(4, 2) == BLACK);
    }

    TEST_CASE("with letterbox bars a destination receives the picture, not the bars") {
        fixture f;
        // 8×8 pixels, 4×2 render units: the picture in the rows 2..5, two pixels per unit.
        REQUIRE(get_renderer().set_logical_presentation(4, 2, sdlpp::logical_presentation::letterbox));
        const layer_id a = f.scene->add({.style = {.destination = rect{0, 0, 2, 1}}});
        f.scene->draw = [&] { paint(a, rect{0, 0, 4, 2}, RED); };   // the whole picture
        f.frame();
        REQUIRE(get_renderer().set_logical_presentation(0, 0, sdlpp::logical_presentation::disabled));
        CHECK(f.at(0, 2) == RED);                  // the destination, pixels (0, 2, 4, 2), filled
        CHECK(f.at(3, 3) == RED);
        CHECK(f.at(0, 4) == BLACK);
        CHECK(f.at(4, 2) == BLACK);
    }

    TEST_CASE("a non-finite opacity fails the frame") {
        fixture f;
        const layer_id a = f.scene->add({.style = {.opacity = std::numeric_limits <float>::quiet_NaN()}});
        f.scene->draw = [&] { paint(a, rect{0, 0, 8, 8}, RED); };
        CHECK_THROWS_WITH(f.frame(), doctest::Contains("opacity must be finite"));
    }

    TEST_CASE("a frame or a compositor that leaves a push_state() fails, and the push is undone") {
        {
            fixture f;
            set_draw_color(GREEN);
            f.scene->draw = [&] {
                push_state();
                set_draw_color(BLUE);
            };
            CHECK_THROWS_WITH(f.frame(), doctest::Contains("push_state() not popped"));
            CHECK(get_painter_state().draw_color == GREEN);
            CHECK_THROWS(pop_state());             // nothing left pushed
        }
        {
            fixture f;
            (void)f.scene->add();
            f.scene->set_compositor([](composition& c) {
                push_state();
                default_compositor(c);
            });
            CHECK_THROWS_WITH(f.frame(), doctest::Contains("the compositor ended with 1"));
            CHECK_THROWS(pop_state());
        }
    }

    TEST_CASE("the compositor starts with the default painter state and draws onto the canvas") {
        fixture f;
        const layer_id a = f.scene->add();
        std::optional <painter_state> inside;
        f.scene->draw = [&] {
            paint(a, rect{0, 0, 8, 8}, RED);
            set_draw_color(BLUE);                  // left behind by render()
            set_draw_blend_mode(blend_mode::blend);
        };
        f.scene->set_compositor([&](composition& c) {
            inside = get_painter_state();
            default_compositor(c);
            draw_rect_fill(rect{0, 0, 2, 2}, GREEN);   // a free function: onto the canvas
        });
        f.frame();
        REQUIRE(inside.has_value());
        CHECK(inside->draw_color == painter_state{}.draw_color);
        CHECK(inside->blend == blend_mode::none);
        CHECK_FALSE(inside->clip.has_value());
        CHECK(f.at(1, 1) == GREEN);
        CHECK(f.at(4, 4) == RED);
        CHECK(get_painter_state().draw_color == BLUE);   // render()'s state again afterwards
    }

    TEST_CASE("pushing a layer inside a compositor fails; the frame unwinds and the next one works") {
        fixture f;
        const layer_id a = f.scene->add();
        bool push = true;
        f.scene->draw = [&] { paint(a, rect{0, 0, 8, 8}, RED); };
        f.scene->set_compositor([&](composition& c) {
            if (push) {
                push_layer(a);
            }
            default_compositor(c);
        });
        SDL_Texture* before = bound();
        CHECK_THROWS_WITH(f.frame(), doctest::Contains("inside a compositor"));
        CHECK(bound() == before);
        CHECK_FALSE(scene_frame::rendering());

        push = false;
        f.frame();
        CHECK(f.at(4, 4) == RED);
    }

    TEST_CASE("set_compositor inside the scene's own compositor fails") {
        fixture f;
        (void)f.scene->add();
        f.scene->set_compositor([&](composition&) { f.scene->set_compositor(default_compositor); });
        CHECK_THROWS_WITH(f.frame(), doctest::Contains("inside the scene's own compositor"));
    }

    TEST_CASE("below() gives the canvases of the visible scenes below, bottom to top") {
        test::test_application app("compositor below");
        scenes_manager& mgr = empty_manager();
        texture out = make_output(dim{8, 8});
        auto binding = out.bind();

        auto hidden = std::make_unique <plain_scene>(BLUE, scene_flags::hides_below);
        auto bottom = std::make_unique <plain_scene>(RED, scene_flags::hides_below);
        auto middle = std::make_unique <plain_scene>(color{0, 255, 0, 128}, scene_flags::none);
        auto top = std::make_unique <layer_scene>(scene_flags::none);
        base_scene* bottom_scene = bottom.get();
        base_scene* middle_scene = middle.get();
        layer_scene* top_scene = top.get();
        (void)top_scene->add();
        std::vector <const texture*> seen;
        top_scene->set_compositor([&](composition& c) {
            seen.assign(c.below().begin(), c.below().end());
        });
        mgr.push_scene_sync(std::move(hidden));
        mgr.push_scene_sync(std::move(bottom));
        mgr.push_scene_sync(std::move(middle));
        mgr.push_scene_sync(std::move(top));

        mgr.render();
        REQUIRE(seen.size() == 2);                 // not the hidden scene
        CHECK(seen[0] == scene_context_access::canvas_of(bottom_scene)->get());
        CHECK(seen[1] == scene_context_access::canvas_of(middle_scene)->get());
        CHECK(seen[0] != nullptr);
        mgr.finish();
    }
}

TEST_SUITE("neutrino::render shortcuts") {
    TEST_CASE("the bottom full-screen scene draws straight into the target; its canvas takes no texture") {
        fixture f;
        f.scene->draw = [&] { draw_rect_fill(rect{2, 2, 2, 2}, RED); };
        f.frame();
        CHECK(scene_context_access::canvas_of(f.scene)->get() == nullptr);
        CHECK_FALSE(f.mgr.covers_target());        // so the window is cleared under it
        CHECK(pixel(f.out, 2, 2) == RED);
        CHECK(pixel(f.out, 1, 1) == BLACK);        // as drawn into the window: opaque
    }

    TEST_CASE("a compositor of a scene's own above keeps the bottom scene's canvas") {
        {
            fixture f;
            f.scene->draw = [&] { draw_rect_fill(rect{2, 2, 2, 2}, RED); };
            auto owned = std::make_unique <layer_scene>(scene_flags::none);
            layer_scene* above = owned.get();
            (void)above->add();
            std::size_t seen_below = 0;
            above->set_compositor([&](composition& c) { seen_below = c.below().size(); });
            f.mgr.push_scene_sync(std::move(owned));
            f.frame();
            CHECK(scene_context_access::canvas_of(f.scene)->get() != nullptr);   // below() may read it
            CHECK(seen_below == 1);
            CHECK(f.at(2, 2) == RED);
        }
    }

    TEST_CASE("the base layer's texture serves as the canvas, and gives the pixels the long way gives") {
        auto draw = [](layer_scene& scene, layer_id a) {
            scene.draw = [a] {
                draw_rect_fill(rect{0, 0, 8, 8}, GREEN);       // the base layer
                paint(a, rect{0, 0, 4, 4}, RED);
                paint(a, rect{4, 4, 4, 4}, color{0, 0, 255, 128}, blend_mode::blend);
            };
        };
        std::vector <color> shared;
        {
            fixture f;
            const layer_id a = f.scene->add();
            draw(*f.scene, a);
            f.frame();
            const detail::canvas* c = scene_context_access::canvas_of(f.scene);
            const detail::layer_stack* layers = scene_context_access::of(f.scene);
            CHECK(c->shared());
            CHECK(c->get() == layers->target(layers->base_layer()));
            for (int y = 0; y < 8; ++y) {
                for (int x = 0; x < 8; ++x) {
                    shared.push_back(f.at(x, y));
                }
            }
        }
        {
            const shortcuts_set long_way{true, false};
            fixture f;
            const layer_id a = f.scene->add();
            draw(*f.scene, a);
            f.frame();
            const detail::canvas* c = scene_context_access::canvas_of(f.scene);
            CHECK_FALSE(c->shared());
            REQUIRE(c->get() != nullptr);
            CHECK(c->get() != scene_context_access::of(f.scene)->target(scene_context_access::of(f.scene)->base_layer()));
            std::size_t i = 0;
            for (int y = 0; y < 8; ++y) {
                for (int x = 0; x < 8; ++x) {
                    const color now = f.at(x, y);
                    const color then = shared[i++];
                    CHECK(near(now.r, then.r));
                    CHECK(near(now.g, then.g));
                    CHECK(near(now.b, then.b));
                }
            }
        }
    }

    TEST_CASE("the base layer is not shared when it is moved or a compositor of the scene's own runs") {
        fixture f;
        const layer_id a = f.scene->add();
        f.scene->draw = [&] { paint(a, rect{0, 0, 2, 2}, RED); };
        detail::layer_stack* layers = scene_context_access::of(f.scene);
        layers->style(layers->base_layer()).opacity = 0.5f;
        f.frame();
        CHECK_FALSE(scene_context_access::canvas_of(f.scene)->shared());

        layers->style(layers->base_layer()).opacity = 1.0f;
        f.scene->set_compositor([](composition& c) { default_compositor(c); });
        f.frame();
        CHECK_FALSE(scene_context_access::canvas_of(f.scene)->shared());
        CHECK(f.at(1, 1) == RED);
    }

    TEST_CASE("the flags alone give the blend: none for a full screen, blend for an overlay") {
        const shortcuts_set through_canvas{false, true};
        fixture f;
        auto owned = std::make_unique <layer_scene>(scene_flags::none);
        layer_scene* overlay = owned.get();
        f.mgr.push_scene_sync(std::move(owned));
        f.frame();
        CHECK(scene_context_access::canvas_of(f.scene)->blend() == blend_mode::none);
        CHECK(scene_context_access::canvas_of(overlay)->blend() == blend_mode::blend);
    }

    TEST_CASE("a covering canvas replaces the window's clear; one that shows nothing is cleared late") {
        fixture f;
        (void)f.scene->add();                      // a layered scene: through its canvas
        bool fail = false;
        f.scene->draw = [&] {
            if (fail) {
                throw std::runtime_error("render failed");
            }
        };
        CHECK(f.mgr.covers_target());
        f.out.fill(GREEN);                         // stale pixels, as a window has after present
        f.mgr.render();
        const color covered = pixel(f.out, 1, 1);
        CHECK(covered.r == 0);                     // written over by the canvas, not left green
        CHECK(covered.g == 0);

        fail = true;
        f.out.fill(GREEN);
        CHECK_THROWS_WITH(f.mgr.render(), doctest::Contains("render failed"));
        CHECK(pixel(f.out, 1, 1) == BLACK);        // nothing to show: cleared after all
    }
}

TEST_SUITE("neutrino::render shortcuts, edges") {
    TEST_CASE("the scene manager clears the target itself, also with no scene") {
        fixture f;
        f.mgr.finish();                            // no scene at all
        f.out.fill(GREEN);
        f.mgr.render();
        CHECK(pixel(f.out, 1, 1) == BLACK);
    }

    TEST_CASE("in a transparent window a full-screen canvas blends over the clear") {
        // A none copy would leave see-through what the canvas left empty; the clear is kept.
        neutrino::application_config cfg;
        cfg.title = "compositor transparent window";
        cfg.width = 8;
        cfg.height = 8;
        cfg.flags = sdlpp::window_flags::hidden | sdlpp::window_flags::transparent;
        test::test_application app(cfg);
        scenes_manager& mgr = empty_manager();
        auto owned = std::make_unique <layer_scene>();
        layer_scene* scene = owned.get();
        (void)scene->add();                        // through its canvas
        mgr.push_scene_sync(std::move(owned));
        CHECK_FALSE(mgr.covers_target());
        mgr.render();
        CHECK(scene_context_access::canvas_of(scene)->blend() == blend_mode::blend);
        mgr.finish();
    }

    TEST_CASE("a replaced scene's blend comes from its flags too") {
        const shortcuts_set through_canvas{false, true};
        fixture f;
        auto owned = std::make_unique <layer_scene>(scene_flags::none);
        layer_scene* overlay = owned.get();
        neutrino::replace_scene(std::move(owned));
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            f.mgr.handle_action(sdlpp::event(ev));
        }
        f.frame();
        CHECK(scene_context_access::canvas_of(overlay)->blend() == blend_mode::blend);
    }

    TEST_CASE("a frame says when render() draws straight into the target") {
        test::test_application app("frame draws into target");
        layer_scene plain;
        {
            scene_frame frame(&plain);
            CHECK_FALSE(frame.draws_into_target());   // onto its canvas
            frame.finish();
        }
        {
            scene_frame frame(&plain, {}, detail::frame_target::bound_target);
            CHECK(frame.draws_into_target());
            frame.finish();
        }
        layer_scene layered;
        (void)layered.add();
        {
            scene_frame frame(&layered);
            CHECK_FALSE(frame.draws_into_target());   // into its base layer
            frame.finish();
        }
    }

    TEST_CASE("add is not an identity copy: it keeps the target's alpha") {
        CHECK(detail::is_identity_style({}));
        CHECK(detail::is_identity_style({.blend = blend_mode::none}));
        CHECK_FALSE(detail::is_identity_style({.blend = blend_mode::add}));
        CHECK_FALSE(detail::is_identity_style({.opacity = 0.5f}));
        CHECK_FALSE(detail::is_identity_style({.destination = rect{0, 0, 1, 1}}));
    }

    TEST_CASE("an on-demand canvas drawn under another presentation needs a redraw") {
        test::test_application app("canvas presentation change");
        texture out = make_output(dim{8, 8});
        auto scope = out.bind();
        detail::canvas c;
        c.set_redraw(redraw_mode::on_demand);
        detail::target_geometry g = detail::bound_target_geometry();
        c.begin_frame(g);
        {
            auto bound = c.bind();
            draw_rect_fill(rect{0, 0, 8, 8}, RED);
            c.note_drawn();
        }
        c.end_frame();
        c.begin_frame(g);
        CHECK_FALSE(c.needs_redraw());             // the same presentation: kept
        c.end_frame();

        g.logical = dim{4, 4};
        g.mode = sdlpp::logical_presentation::letterbox;
        c.begin_frame(g);                          // same pixels, another mapping
        CHECK(c.needs_redraw());
        REQUIRE(c.get() != nullptr);
        CHECK(pixel(*c.get(), 1, 1).a == 0);       // cleared: no old picture at a wrong scale
    }
}

TEST_SUITE("neutrino::layer members") {
    TEST_CASE("style_of changes a layer's style live; the base layer has none") {
        fixture f;
        const layer_id a = f.scene->add();
        f.scene->draw = [&] { paint(a, rect{0, 0, 2, 2}, RED); };
        f.frame();
        CHECK(f.at(1, 1) == RED);

        f.scene->style(a).offset = point{4, 4};    // between frames, as fixed_update would
        f.frame();
        CHECK(f.at(5, 5) == RED);
        CHECK(f.at(1, 1) == BLACK);
        CHECK_THROWS_WITH((void)f.scene->style(f.scene->base()), doctest::Contains("base layer"));
    }

    TEST_CASE("style_of in a scene without layers fails") {
        fixture f;
        detail::layer_stack other(detail::target_geometry{});
        const layer_id foreign = other.declare();
        CHECK_THROWS_WITH((void)f.scene->style(foreign), doctest::Contains("declared no layers"));
    }

    TEST_CASE("scoped_layer pushes now and pops at the end of the scope, also on an exception") {
        fixture f;
        const layer_id top = f.scene->add();
        bool fail = false;
        f.scene->draw = [&] {
            {
                auto pushed = scoped_layer(top);
                draw_rect_fill(rect{0, 0, 2, 2}, RED);
                if (fail) {
                    throw std::runtime_error("render failed");
                }
            }
            draw_rect_fill(rect{0, 0, 8, 8}, BLUE);   // the base layer again: under the top layer
        };
        f.frame();
        CHECK(f.at(1, 1) == RED);
        CHECK(f.at(5, 5) == BLUE);

        fail = true;
        CHECK_THROWS_WITH(f.frame(), doctest::Contains("render failed"));
        CHECK_FALSE(scene_frame::rendering());
        fail = false;
        f.frame();                                 // nothing was left pushed
        CHECK(f.at(1, 1) == RED);
    }

    TEST_CASE("clear() empties the current layer, or the scene's picture without layers") {
        {
            fixture f;
            const layer_id kept = f.scene->add({.redraw = redraw_mode::on_demand});
            bool restart = false;
            f.scene->draw = [&] {
                push_layer(kept);
                if (restart) {
                    clear();                       // start the kept layer again
                    draw_rect_fill(rect{6, 6, 1, 1}, BLUE);
                } else {
                    draw_rect_fill(rect{0, 0, 8, 8}, RED);
                }
                pop_layer();
            };
            f.frame();
            CHECK(f.at(1, 1) == RED);
            restart = true;
            f.frame();
            CHECK(f.at(1, 1) == BLACK);
            CHECK(f.at(6, 6) == BLUE);
        }
        {
            fixture f;                             // no layers: straight into the target
            f.scene->draw = [&] {
                draw_rect_fill(rect{0, 0, 8, 8}, RED);
                clear();
                draw_rect_fill(rect{0, 0, 1, 1}, BLUE);
            };
            f.frame();
            CHECK(f.at(0, 0) == BLUE);
            CHECK(f.at(4, 4) == BLACK);
        }
        CHECK_THROWS_WITH(clear(), doctest::Contains("outside a scene's render()"));
    }

    TEST_CASE("needs_redraw: true for an on-demand layer until a frame pushes it; never for every-frame") {
        fixture f;
        const layer_id kept = f.scene->add({.redraw = redraw_mode::on_demand});
        const layer_id every = f.scene->add();
        std::vector <bool> kept_seen;
        std::vector <bool> every_seen;
        f.scene->draw = [&] {
            kept_seen.push_back(needs_redraw(kept));
            every_seen.push_back(needs_redraw(every));
            if (kept_seen.size() == 1) {
                paint(kept, rect{0, 0, 2, 2}, RED);    // drawn in full once
            }
        };
        f.frame();
        f.frame();
        REQUIRE(kept_seen.size() == 2);
        CHECK(kept_seen[0]);
        CHECK_FALSE(kept_seen[1]);
        CHECK_FALSE(every_seen[0]);
        CHECK_FALSE(every_seen[1]);
        CHECK(f.at(1, 1) == RED);                  // kept without being drawn again
    }
}

TEST_SUITE("neutrino::compositor wrapping") {
    // What a transition does (painter.md §2.9): wrap whatever compositor the scene has, the
    // default or the game's own, draw over what it composes, and put it back when done.
    [[nodiscard]] compositor wrap_with(const compositor& inner, rect mark, color c) {
        return [inner, mark, c](composition& comp) {
            inner(comp);                                   // the scene, composed as it is
            draw_rect_fill(mark, c);                       // then the wrapper's own drawing
        };
    }

    TEST_CASE("a wrapper calls the compositor it wraps, draws over it, and is put back") {
        fixture f;
        const layer_id a = f.scene->add();
        f.scene->draw = [&] { paint(a, rect{0, 0, 8, 8}, RED); };
        const compositor inner = f.scene->get_compositor();
        f.scene->set_compositor(wrap_with(inner, rect{0, 0, 2, 2}, BLUE));
        f.frame();
        CHECK(f.at(1, 1) == BLUE);                         // the wrapper's, over the layer
        CHECK(f.at(5, 5) == RED);                          // the wrapped default compositor's

        f.scene->set_compositor(inner);                    // the transition ends
        f.frame();
        CHECK(f.at(1, 1) == RED);
        using fn = void (*)(composition&);
        REQUIRE(f.scene->get_compositor().target <fn>() != nullptr);
        CHECK(*f.scene->get_compositor().target <fn>() == &default_compositor);
    }

    TEST_CASE("a wrapper keeps a compositor of the game's own as the game wrote it") {
        fixture f;
        const layer_id a = f.scene->add();
        f.scene->draw = [&] { paint(a, rect{0, 0, 2, 2}, RED); };
        f.scene->set_compositor([a](composition& c) {      // the game's: the layer twice
            c.draw_layer(a);
            c.draw_layer(a, {.offset = {4, 4}});
        });
        f.scene->set_compositor(wrap_with(f.scene->get_compositor(), rect{6, 0, 2, 2}, BLUE));
        f.frame();
        CHECK(f.at(1, 1) == RED);                          // the game's first copy
        CHECK(f.at(5, 5) == RED);                          // and its second
        CHECK(f.at(7, 1) == BLUE);                         // the wrapper's
    }

    TEST_CASE("wrappers nest: the outer one draws last") {
        fixture f;
        const layer_id a = f.scene->add();
        f.scene->draw = [&] { paint(a, rect{0, 0, 8, 8}, RED); };
        const compositor original = f.scene->get_compositor();
        f.scene->set_compositor(wrap_with(original, rect{0, 0, 4, 4}, GREEN));
        f.scene->set_compositor(wrap_with(f.scene->get_compositor(), rect{2, 2, 4, 4}, BLUE));
        f.frame();
        CHECK(f.at(1, 1) == GREEN);                        // the inner wrapper only
        CHECK(f.at(3, 3) == BLUE);                         // the outer wrapper, over the inner
        CHECK(f.at(7, 7) == RED);                          // the scene
        f.scene->set_compositor(original);
        f.frame();
        CHECK(f.at(3, 3) == RED);
    }

    TEST_CASE("a wrapper reads the scenes below, as a transition over the old scene would") {
        fixture f;                                         // the bottom scene, without layers
        f.scene->draw = [&] { draw_rect_fill(rect{0, 0, 8, 8}, RED); };
        auto owned = std::make_unique <layer_scene>(scene_flags::none);
        layer_scene* incoming = owned.get();
        const layer_id a = incoming->add();
        incoming->draw = [&] { paint(a, rect{0, 0, 4, 4}, BLUE); };
        std::size_t seen_below = 0;
        const texture* first_below = nullptr;
        const compositor inner = incoming->get_compositor();
        incoming->set_compositor([inner, &seen_below, &first_below](composition& c) {
            seen_below = c.below().size();
            first_below = seen_below > 0 ? c.below().front() : nullptr;
            inner(c);
        });
        f.mgr.push_scene_sync(std::move(owned));
        f.frame();
        REQUIRE(seen_below == 1);                          // not drawn straight into the window
        CHECK(first_below == scene_context_access::canvas_of(f.scene)->get());
        CHECK(f.at(1, 1) == BLUE);
        CHECK(f.at(6, 6) == RED);

        incoming->set_compositor(inner);                   // back to the default
        f.frame();
        CHECK(scene_context_access::canvas_of(f.scene)->get() == nullptr);   // straight in again (R44)
        CHECK(f.at(1, 1) == BLUE);
        CHECK(f.at(6, 6) == RED);
    }
}

TEST_SUITE("neutrino::screenshot") {
    TEST_CASE("screenshot() shows every visible scene at the window's size, opaque") {
        test::test_application app("screenshot scenes");
        scenes_manager& mgr = empty_manager();
        auto bottom = std::make_unique <plain_scene>(RED, scene_flags::hides_below);
        auto overlay = std::make_unique <layer_scene>(scene_flags::none);
        layer_scene* top = overlay.get();
        const layer_id a = top->add();
        top->draw = [&] { paint(a, rect{0, 0, 10, 10}, BLUE); };
        mgr.push_scene_sync(std::move(bottom));
        mgr.push_scene_sync(std::move(overlay));

        const image shot = screenshot();
        const auto size = get_renderer().get_output_size();
        REQUIRE(size.has_value());
        CHECK(shot.width() == size->width);
        CHECK(shot.height() == size->height);
        CHECK(pixel_of(shot, 5, 5) == BLUE);       // the overlay
        CHECK(pixel_of(shot, 50, 50) == RED);      // the full-screen scene under it
        CHECK(pixel_of(shot, 50, 50).a == 255);
        CHECK(bound() == nullptr);                 // the window is bound again
        mgr.finish();
    }

    TEST_CASE("under a letterbox presentation the shot is the whole window, bars included") {
        test::test_application app("screenshot letterbox");
        scenes_manager& mgr = empty_manager();
        const auto size = get_renderer().get_output_size();
        REQUIRE(size.has_value());
        // A render space twice as wide as tall: bars above and below.
        REQUIRE(get_renderer().set_logical_presentation(20, 10, sdlpp::logical_presentation::letterbox));
        mgr.push_scene_sync(std::make_unique <plain_scene>(RED, scene_flags::hides_below));
        const image shot = screenshot();
        CHECK(shot.width() == size->width);
        CHECK(shot.height() == size->height);
        CHECK(pixel_of(shot, size->width / 2, 0) == BLACK);                // a bar
        CHECK(pixel_of(shot, size->width / 2, size->height / 2) == RED);   // the picture
        const auto lp = get_renderer().get_logical_presentation();
        REQUIRE(lp.has_value());
        CHECK(lp->width == 20);                    // the window's presentation untouched
        mgr.finish();
    }

    TEST_CASE("screenshot() fails inside a scene's render(), or with a texture bound") {
        test::test_application app("screenshot refused");
        scenes_manager& mgr = empty_manager();
        auto owned = std::make_unique <layer_scene>();
        layer_scene* scene = owned.get();
        bool refused = false;
        scene->draw = [&] {
            try {
                (void)screenshot();
            } catch (const std::exception&) {
                refused = true;
            }
        };
        mgr.push_scene_sync(std::move(owned));
        mgr.render();
        CHECK(refused);

        texture t = make_output(dim{4, 4});
        auto scope = t.bind();
        CHECK_THROWS_WITH((void)screenshot(), doctest::Contains("texture is bound"));
        mgr.finish();
    }

    TEST_CASE("screenshot() needs a running application") {
        CHECK_THROWS_WITH((void)screenshot(), doctest::Contains("running application"));
    }
}
