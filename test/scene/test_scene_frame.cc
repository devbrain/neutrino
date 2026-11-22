//
// Tests for a scene's layer frame: detail::scene_frame on its own, the free push_layer /
// pop_layer (video/layer_ops.hh), and the scene manager running the frame around each
// scene's render() (painter.md §3.2).
//
#include <doctest/doctest.h>

#include <initializer_list>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

#include <neutrino/scene/base_scene.hh>
#include <neutrino/video/draw.hh>
#include <neutrino/video/globals.hh>
#include <neutrino/video/layer_ops.hh>
#include <neutrino/video/texture.hh>

#include "scene/scene_context_access.hh"
#include "scene/scene_frame.hh"
#include "scene/scenes_manager.hh"
#include "services/service_locator.hh"
#include "services/service_access.hh"
#include "video/canvas.hh"
#include "video/layer_stack.hh"
#include "video/texture_allocator.hh"
#include "test_application.hh"

using namespace neutrino;
using neutrino::detail::canvas;
using neutrino::detail::layer_stack;
using neutrino::detail::scene_context_access;
using neutrino::detail::scene_frame;

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

    // The application's scenes_manager with an empty stack (as in test_scenes_manager.cc).
    scenes_manager& empty_manager() {
        scenes_manager& mgr = service_locator::instance().get_scenes_manager();
        mgr.finish();
        REQUIRE(mgr.empty());
        return mgr;
    }

    // What a layered scene saw during its render().
    struct render_probe {
        int renders = 0;
        bool rendering = false;            // scene_frame::rendering() inside render()
        SDL_Texture* at_start = nullptr;   // bound when render() began: the base layer
        SDL_Texture* in_hud = nullptr;     // bound while the HUD was pushed
    };

    // A scene with one layer, the HUD, declared in its constructor. Its render() pushes the
    // HUD, draws a red point on it and pops it; it can be told to throw, or to leave the HUD
    // pushed.
    class layered_scene : public base_scene {
        public:
            explicit layered_scene(render_probe& probe)
                : m_probe(probe), m_hud(declare_layer()) {
            }

            [[nodiscard]] layer_id hud() const { return m_hud; }
            void throw_in_render(bool v) { m_throw = v; }
            void leave_pushed(bool v) { m_leave_pushed = v; }

            void fixed_update(sim_duration, const input_snapshot&) override {}

            void render() override {
                ++m_probe.renders;
                m_probe.rendering = scene_frame::rendering();
                m_probe.at_start = bound();
                push_layer(m_hud);                  // the free function: there is no member
                m_probe.in_hud = bound();
                set_draw_color(colors::red);
                draw_point(0, 0);
                if (m_throw) {
                    throw std::runtime_error("render failed");
                }
                if (!m_leave_pushed) {
                    pop_layer();
                }
            }

        private:
            render_probe& m_probe;
            layer_id m_hud;
            bool m_throw{false};
            bool m_leave_pushed{false};
    };

    // A scene that declares no layers and records what was bound in its render().
    class plain_scene : public base_scene {
        public:
            explicit plain_scene(std::optional <SDL_Texture*>& seen)
                : m_seen(seen) {
            }

            void fixed_update(sim_duration, const input_snapshot&) override {}

            void render() override {
                m_seen = bound();
            }

        private:
            std::optional <SDL_Texture*>& m_seen;
    };

    // A scene that only declares layers, @p count of them; the frame tests run its frame by
    // hand. With none it is a scene without layers.
    class stack_scene : public base_scene {
        public:
            explicit stack_scene(std::size_t count = 1) {
                for (std::size_t i = 0; i < count; ++i) {
                    m_ids.push_back(declare_layer());
                }
            }

            explicit stack_scene(std::initializer_list <layer_settings> settings) {
                for (const layer_settings& ls : settings) {
                    m_ids.push_back(declare_layer(ls));
                }
            }

            [[nodiscard]] layer_id layer(std::size_t i) const { return m_ids.at(i); }

            [[nodiscard]] layer_stack& layers() {
                layer_stack* s = scene_context_access::of(this);
                REQUIRE(s != nullptr);
                return *s;
            }

            void fixed_update(sim_duration, const input_snapshot&) override {}
            void render() override {}

        private:
            std::vector <layer_id> m_ids;
    };

    // A scene without layers that fills a rectangle; an overlay unless told to hide below.
    class painting_scene : public base_scene {
        public:
            painting_scene(rect area, color fill, scene_flags flags)
                : m_area(area), m_fill(fill), m_flags(flags) {
            }

            int renders = 0;
            dim seen_size{0, 0};                   // render_size() inside render()

            void fixed_update(sim_duration, const input_snapshot&) override {}

            void render() override {
                ++renders;
                seen_size = render_size();
                set_draw_blend_mode(blend_mode::blend);  // the default, none, would not blend
                draw_rect_fill(m_area, m_fill);
            }

            [[nodiscard]] scene_flags flags() const override { return m_flags; }

        private:
            rect m_area;
            color m_fill;
            scene_flags m_flags;
    };

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

    // The pixel as the screen shows it: the target stands in for the window, which has no
    // alpha, and a full-screen scene's canvas is copied with none (painter.md §2.3).
    [[nodiscard]] color shown(const texture& t, int x, int y) {
        const color c = pixel(t, x, y);
        return color{c.r, c.g, c.b, 255};
    }
}

TEST_SUITE("neutrino::detail::scene_frame") {
    TEST_CASE("push_layer and pop_layer outside a scene's render() fail") {
        test::test_application app("scene frame outside");
        CHECK_FALSE(scene_frame::rendering());
        CHECK_THROWS_WITH(push_layer(layer_id{}), doctest::Contains("outside a scene's render()"));
        CHECK_THROWS_WITH(pop_layer(), doctest::Contains("outside a scene's render()"));
    }

    TEST_CASE("in the frame of a scene without layers they fail, naming that") {
        test::test_application app("scene frame no layers");
        SDL_Texture* before = bound();
        {
            scene_frame frame(nullptr);
            CHECK(scene_frame::rendering());
            CHECK(scene_frame::get_current_layers_stack() == nullptr);
            CHECK(bound() == before);                             // nothing pushed
            CHECK_THROWS_WITH(push_layer(layer_id{}), doctest::Contains("declared no layers"));
            CHECK_THROWS_WITH(pop_layer(), doctest::Contains("declared no layers"));
            frame.finish();
        }
        CHECK_FALSE(scene_frame::rendering());
    }

    TEST_CASE("a frame pushes the base layer; push_layer and pop_layer reach the scene's layers") {
        test::test_application app("scene frame push");
        SDL_Texture* before = bound();
        stack_scene scene;
        layer_stack& s = scene.layers();
        const layer_id hud = scene.layer(0);
        {
            scene_frame frame(&scene);
            CHECK(scene_frame::get_current_layers_stack() == &s);
            CHECK(bound() == raw(s.target(s.base_layer())));
            push_layer(hud);
            CHECK(bound() == raw(s.target(hud)));
            pop_layer();
            CHECK(bound() == raw(s.target(s.base_layer())));
            CHECK_THROWS(pop_layer());                            // the frame's own push is not popped
            frame.finish();
        }
        CHECK(bound() == before);
        CHECK(scene_frame::get_current_layers_stack() == nullptr);
        CHECK_THROWS_WITH(push_layer(hud), doctest::Contains("outside a scene's render()"));
    }

    TEST_CASE("a layer of another scene is refused") {
        test::test_application app("scene frame other");
        stack_scene mine;
        stack_scene other;
        const layer_id theirs = other.layer(0);
        scene_frame frame(&mine);
        CHECK_THROWS(push_layer(theirs));
        CHECK_THROWS(push_layer(layer_id{}));
        frame.finish();
    }

    TEST_CASE("a frame that ends without finish() - render() threw - ends every push") {
        test::test_application app("scene frame abort");
        SDL_Texture* before = bound();
        stack_scene scene(2);
        {
            scene_frame frame(&scene);
            push_layer(scene.layer(0));
            push_layer(scene.layer(1));
        }
        CHECK(bound() == before);
        CHECK_FALSE(scene_frame::rendering());
        scene_frame next(&scene);                                 // the next frame starts cleanly
        next.finish();
    }

    TEST_CASE("finish() with a layer still pushed fails, and the frame still ends") {
        test::test_application app("scene frame left pushed");
        SDL_Texture* before = bound();
        stack_scene scene;
        {
            scene_frame frame(&scene);
            push_layer(scene.layer(0));
            CHECK_THROWS_WITH(frame.finish(), doctest::Contains("still pushed"));
            CHECK(bound() == before);                             // end_frame() unwound
        }
        CHECK_FALSE(scene_frame::rendering());
        scene_frame next(&scene);
        next.finish();
    }

    TEST_CASE("frames do not nest, also under a scene without layers") {
        test::test_application app("scene frame nest");
        stack_scene scene;
        {
            scene_frame outer(nullptr);
            CHECK_THROWS_WITH([&] { scene_frame inner(&scene); }(),
                              doctest::Contains("already being rendered"));
            CHECK(scene_frame::rendering());                      // the failed one changed nothing
            outer.finish();
        }
        {
            scene_frame outer(&scene);
            CHECK_THROWS_WITH([] { scene_frame inner(nullptr); }(),
                              doctest::Contains("already being rendered"));
            CHECK(scene_frame::get_current_layers_stack() == &scene.layers());
            outer.finish();
        }
    }

    TEST_CASE("a scene without layers renders onto its canvas; finish() marks it drawn") {
        test::test_application app("scene frame canvas");
        SDL_Texture* before = bound();
        const detail::target_geometry target = detail::bound_target_geometry();
        stack_scene plain(0);
        canvas* c = scene_context_access::canvas_of(&plain);
        REQUIRE(c != nullptr);
        {
            scene_frame frame(&plain);
            REQUIRE(c->get() != nullptr);
            CHECK(bound() == raw(c->get()));
            CHECK(c->get()->size() == target.size);               // it mirrors the target
            frame.finish();
            CHECK(bound() == before);
        }
        CHECK(c->drawn());
    }

    TEST_CASE("a scene with layers binds its canvas only to combine") {
        test::test_application app("scene frame layered canvas");
        stack_scene scene;
        canvas* c = scene_context_access::canvas_of(&scene);
        REQUIRE(c != nullptr);
        SDL_Texture* before = bound();
        {
            scene_frame frame(&scene);
            CHECK(bound() == raw(scene.layers().target(scene.layers().base_layer())));
            CHECK(c->get() == nullptr);                           // not during render()
            frame.finish();
        }
        CHECK(c->get() != nullptr);                               // taken to combine (R41)
        CHECK(c->drawn());
        CHECK(bound() == before);
    }

    TEST_CASE("a frame whose render() threw ends the canvas's binding, not drawn") {
        test::test_application app("scene frame canvas abort");
        SDL_Texture* before = bound();
        stack_scene plain(0);
        {
            scene_frame frame(&plain);
            CHECK(bound() != before);
        }
        CHECK(bound() == before);
        CHECK_FALSE(scene_context_access::canvas_of(&plain)->drawn());
    }
}

TEST_SUITE("neutrino::scene layers") {
    TEST_CASE("declare_layer makes the scene's layers; a scene that declares none has none") {
        test::test_application app("scene layers declare");
        render_probe probe;
        std::optional <SDL_Texture*> seen;
        layered_scene layered(probe);
        plain_scene plain(seen);
        layer_stack* layers = scene_context_access::of(&layered);
        REQUIRE(layers != nullptr);
        CHECK(layers->target(layered.hud()) == nullptr);          // no texture before its first push
        CHECK(scene_context_access::of(&plain) == nullptr);
        CHECK(scene_context_access::of(nullptr) == nullptr);
    }

    TEST_CASE("the scene manager runs a layered scene's frame around render()") {
        render_probe probe;
        test::test_application app("scene layers render");
        scenes_manager& mgr = empty_manager();
        auto owned = std::make_unique <layered_scene>(probe);
        layered_scene* scene = owned.get();
        mgr.push_scene_sync(std::move(owned));
        SDL_Texture* before = bound();

        mgr.render();

        layer_stack* layers = scene_context_access::of(scene);
        REQUIRE(layers != nullptr);
        CHECK(probe.renders == 1);
        CHECK(probe.rendering);
        CHECK(probe.at_start == raw(layers->target(layers->base_layer())));
        CHECK(probe.in_hud == raw(layers->target(scene->hud())));
        REQUIRE(layers->target(scene->hud()) != nullptr);
        CHECK(pixel_is(*layers->target(scene->hud()), 0, 0, colors::red));
        CHECK(bound() == before);                                 // the window again
        CHECK_FALSE(scene_frame::rendering());

        mgr.render();                                             // and every frame after
        CHECK(probe.renders == 2);
        CHECK(bound() == before);
        mgr.finish();
    }

    TEST_CASE("a scene without layers renders onto its canvas, which is copied into the target") {
        const shortcuts_set through_canvas{false, true};          // not straight into the window
        std::optional <SDL_Texture*> seen;
        test::test_application app("scene layers plain");
        scenes_manager& mgr = empty_manager();
        auto owned = std::make_unique <plain_scene>(seen);
        plain_scene* scene = owned.get();
        mgr.push_scene_sync(std::move(owned));
        SDL_Texture* before = bound();

        mgr.render();

        REQUIRE(seen.has_value());
        CHECK(*seen == raw(scene_context_access::canvas_of(scene)->get()));
        CHECK(bound() == before);
        mgr.finish();
    }

    TEST_CASE("the target shows what the scene drew, as if drawn straight into it") {
        std::optional <shortcuts_set> path;
        SUBCASE("straight into the target") {}
        SUBCASE("through its canvas") { path.emplace(false, true); }
        test::test_application app("scene canvas copy");
        scenes_manager& mgr = empty_manager();
        texture out = make_output(dim{8, 8});
        auto scope = out.bind();
        mgr.push_scene_sync(std::make_unique <painting_scene>(rect{2, 2, 2, 2}, colors::red,
                                                              scene_flags::hides_below));
        mgr.render();
        CHECK(shown(out, 2, 2) == colors::red);
        CHECK(shown(out, 3, 3) == colors::red);
        CHECK(shown(out, 1, 1) == colors::black);
        CHECK(bound() == out.native().get());
        mgr.finish();
    }

    TEST_CASE("under a logical presentation the scene lands on the target's pixels as before") {
        std::optional <shortcuts_set> path;
        SUBCASE("straight into the target") {}
        SUBCASE("through its canvas") { path.emplace(false, true); }
        test::test_application app("scene canvas presentation");
        scenes_manager& mgr = empty_manager();
        texture out = make_output(dim{8, 8});
        auto scope = out.bind();
        REQUIRE(get_renderer().set_logical_presentation(4, 4, sdlpp::logical_presentation::letterbox));
        auto owned = std::make_unique <painting_scene>(rect{1, 1, 1, 1}, colors::red,
                                                       scene_flags::hides_below);
        painting_scene* scene = owned.get();
        mgr.push_scene_sync(std::move(owned));

        mgr.render();

        CHECK(scene->seen_size == dim{4, 4});                     // render space, as before
        CHECK(shown(out, 2, 2) == colors::red);                   // one render unit: 2 x 2 pixels
        CHECK(shown(out, 3, 3) == colors::red);
        CHECK(shown(out, 1, 1) == colors::black);
        CHECK(shown(out, 4, 4) == colors::black);
        const auto lp = get_renderer().get_logical_presentation();
        REQUIRE(lp.has_value());
        CHECK(lp->width == 4);                                    // put back after the copy
        CHECK(lp->mode == sdlpp::logical_presentation::letterbox);
        mgr.finish();
    }

    TEST_CASE("with letterbox bars the canvas is the target's whole size, so nothing moves") {
        const shortcuts_set through_canvas{false, true};
        test::test_application app("scene canvas letterbox");
        scenes_manager& mgr = empty_manager();
        texture out = make_output(dim{8, 8});
        auto scope = out.bind();
        // 4 x 2 render units on 8 x 8 pixels: the rows 2..5, two pixels per unit.
        REQUIRE(get_renderer().set_logical_presentation(4, 2, sdlpp::logical_presentation::letterbox));
        auto owned = std::make_unique <painting_scene>(rect{0, 0, 1, 1}, colors::red,
                                                       scene_flags::hides_below);
        painting_scene* scene = owned.get();
        mgr.push_scene_sync(std::move(owned));

        mgr.render();

        REQUIRE(scene_context_access::canvas_of(scene)->size() == dim{8, 8});
        // Read back without the presentation: under it, a readback sees only the render area.
        REQUIRE(get_renderer().set_logical_presentation(0, 0, sdlpp::logical_presentation::disabled));
        CHECK(shown(out, 0, 1) == colors::black);                 // the bar above
        CHECK(shown(out, 0, 2) == colors::red);
        CHECK(shown(out, 1, 3) == colors::red);
        CHECK(shown(out, 2, 2) == colors::black);
        mgr.finish();
    }

    TEST_CASE("an overlay's canvas is blended over the scene below") {
        test::test_application app("scene canvas overlay");
        scenes_manager& mgr = empty_manager();
        texture out = make_output(dim{8, 8});
        auto scope = out.bind();
        mgr.push_scene_sync(std::make_unique <painting_scene>(rect{0, 0, 8, 8}, colors::red,
                                                              scene_flags::hides_below));
        mgr.push_scene_sync(std::make_unique <painting_scene>(rect{0, 0, 4, 8}, color{0, 0, 255, 128},
                                                              scene_flags::none));
        mgr.render();
        const color mixed = pixel(out, 1, 1);                     // half blue over red
        CHECK(mixed.r >= 126);
        CHECK(mixed.r <= 128);
        CHECK(mixed.b >= 127);
        CHECK(mixed.b <= 129);
        CHECK(pixel(out, 6, 1) == colors::red);                   // the overlay left it alone
        mgr.finish();
    }

    TEST_CASE("a hidden scene gives its every-frame canvas back; an on-demand one keeps it") {
        const shortcuts_set through_canvas{false, true};
        test::test_application app("scene canvas hidden");
        scenes_manager& mgr = empty_manager();
        auto& allocator = require_texture_allocator();
        auto first = std::make_unique <painting_scene>(rect{0, 0, 2, 2}, colors::red, scene_flags::hides_below);
        painting_scene* bottom = first.get();
        mgr.push_scene_sync(std::move(first));
        mgr.render();
        canvas* c = scene_context_access::canvas_of(bottom);
        REQUIRE(c->get() != nullptr);

        mgr.push_scene_sync(std::make_unique <painting_scene>(rect{0, 0, 2, 2}, colors::green,
                                                              scene_flags::hides_below));
        const auto pooled = allocator.stats().pooled;
        mgr.render();
        CHECK(bottom->renders == 1);                              // hidden: not rendered
        CHECK(c->get() == nullptr);                               // its canvas went back...
        CHECK(allocator.stats().pooled == pooled);                // ...and the top scene took it

        mgr.finish();
    }

    TEST_CASE("a hidden scene with an on-demand canvas keeps its texture") {
        test::test_application app("scene canvas hidden on demand");
        scenes_manager& mgr = empty_manager();
        auto first = std::make_unique <painting_scene>(rect{0, 0, 2, 2}, colors::red, scene_flags::hides_below);
        painting_scene* bottom = first.get();
        canvas* c = scene_context_access::canvas_of(bottom);
        c->set_redraw(redraw_mode::on_demand);
        mgr.push_scene_sync(std::move(first));
        mgr.render();
        const texture* kept = c->get();
        REQUIRE(kept != nullptr);

        mgr.push_scene_sync(std::make_unique <painting_scene>(rect{0, 0, 2, 2}, colors::green,
                                                              scene_flags::hides_below));
        mgr.render();
        CHECK(bottom->renders == 1);
        CHECK(c->get() == kept);                                  // its contents are the point
        mgr.finish();
    }

    TEST_CASE("a hidden scene gives back its every-frame layers' textures; on-demand ones are kept") {
        test::test_application app("scene layers hidden");
        scenes_manager& mgr = empty_manager();
        auto first = std::make_unique <stack_scene>(
            std::initializer_list <layer_settings>{{}, {.redraw = redraw_mode::on_demand}});
        stack_scene* bottom = first.get();
        mgr.push_scene_sync(std::move(first));
        layer_stack* layers = &bottom->layers();
        const layer_id every = bottom->layer(0);
        const layer_id kept = bottom->layer(1);
        // One frame by hand, pushing both, so each takes a texture.
        {
            scene_frame frame(bottom);
            push_layer(every);
            pop_layer();
            push_layer(kept);
            pop_layer();
            frame.finish();
        }
        REQUIRE(layers->target(layers->base_layer()) != nullptr);
        REQUIRE(layers->target(every) != nullptr);
        REQUIRE(layers->target(kept) != nullptr);

        mgr.push_scene_sync(std::make_unique <painting_scene>(rect{0, 0, 2, 2}, colors::green,
                                                              scene_flags::hides_below));
        mgr.render();
        CHECK(layers->target(layers->base_layer()) == nullptr);
        CHECK(layers->target(every) == nullptr);
        CHECK(layers->target(kept) != nullptr);                   // its contents are the point
        mgr.finish();
    }

    TEST_CASE("a layer switched to mod is refilled opaque white, not transparent") {
        test::test_application app("scene layers clear colour");
        stack_scene scene({{.redraw = redraw_mode::on_demand}});
        layer_stack& layers = scene.layers();
        const layer_id tint = scene.layer(0);
        {
            texture out = make_output(dim{8, 8});
            auto scope = out.bind();
            scene_frame frame(&scene);
            push_layer(tint);                                     // taken, cleared transparent
            pop_layer();
            frame.finish();
        }
        REQUIRE(layers.target(tint) != nullptr);
        CHECK(pixel_is(*layers.target(tint), 0, 0, colors::transparent));

        layers.style(tint).blend = blend_mode::mod;           // the settings are live
        {
            texture out = make_output(dim{16, 16});               // a resize refills it
            auto scope = out.bind();
            scene_frame frame(&scene);
            frame.finish();
        }
        REQUIRE(layers.target(tint) != nullptr);
        CHECK(pixel_is(*layers.target(tint), 0, 0, colors::white));
    }

    TEST_CASE("a scene that throws still shows the scenes below it in that frame") {
        // An overlay whose render() fails.
        class failing_overlay : public base_scene {
            public:
                void fixed_update(sim_duration, const input_snapshot&) override {}
                void render() override { throw std::runtime_error("overlay failed"); }
                [[nodiscard]] scene_flags flags() const override { return scene_flags::none; }
        };
        test::test_application app("scene canvas throw below");
        scenes_manager& mgr = empty_manager();
        texture out = make_output(dim{8, 8});
        auto scope = out.bind();
        mgr.push_scene_sync(std::make_unique <painting_scene>(rect{0, 0, 8, 8}, colors::red,
                                                              scene_flags::hides_below));
        mgr.push_scene_sync(std::make_unique <failing_overlay>());
        CHECK_THROWS_WITH(mgr.render(), doctest::Contains("overlay failed"));
        CHECK(pixel(out, 2, 2) == colors::red);                   // not a black frame
        CHECK(bound() == out.native().get());
        mgr.finish();
    }

    TEST_CASE("a scene whose render() throws leaves the target as it was") {
        render_probe probe;
        test::test_application app("scene layers throw");
        scenes_manager& mgr = empty_manager();
        auto owned = std::make_unique <layered_scene>(probe);
        layered_scene* scene = owned.get();
        mgr.push_scene_sync(std::move(owned));
        SDL_Texture* before = bound();

        scene->throw_in_render(true);
        CHECK_THROWS_WITH(mgr.render(), doctest::Contains("render failed"));
        CHECK(bound() == before);
        CHECK_FALSE(scene_frame::rendering());

        scene->throw_in_render(false);
        CHECK_NOTHROW(mgr.render());                              // the next frame starts cleanly
        CHECK(bound() == before);
        mgr.finish();
    }

    TEST_CASE("a scene that leaves a layer pushed fails its frame, naming it") {
        render_probe probe;
        test::test_application app("scene layers left pushed");
        scenes_manager& mgr = empty_manager();
        auto owned = std::make_unique <layered_scene>(probe);
        layered_scene* scene = owned.get();
        mgr.push_scene_sync(std::move(owned));
        SDL_Texture* before = bound();

        scene->leave_pushed(true);
        CHECK_THROWS_WITH(mgr.render(), doctest::Contains("still pushed"));
        CHECK(bound() == before);
        CHECK_FALSE(scene_frame::rendering());

        scene->leave_pushed(false);
        CHECK_NOTHROW(mgr.render());
        mgr.finish();
    }
}
