//
// Tests for neutrino::texture and the texture draw API (roadmap P3.4).
//
// Every texture is drawn into, or read back from, a TARGET texture -- never the window, whose
// contents are undefined after present. Reading back is itself part of the API under test
// (texture::to_image, capture_frame).
//
#include <doctest/doctest.h>

#include <neutrino/video/draw.hh>
#include <neutrino/video/texture.hh>

#include "test_application.hh"

#include <array>
#include <cstdint>
#include <vector>

using namespace neutrino;

namespace {
    // RGBA8888 packs as 0xRRGGBBAA.
    constexpr std::uint32_t red_px = 0xFF0000FFu;
    constexpr std::uint32_t blue_px = 0x0000FFFFu;

    [[nodiscard]] color pixel_at(const image& img, int x, int y) {
        const auto c = img.get_pixel(x, y);
        REQUIRE(c.has_value());
        return *c;
    }

    [[nodiscard]] bool same_rgb(const color& a, const color& b) {
        return a.r == b.r && a.g == b.g && a.b == b.b;
    }

    [[nodiscard]] texture make_target(int w, int h) {
        auto t = texture::create(dim{w, h}, texture_access::target);
        REQUIRE(t.has_value());
        return std::move(*t);
    }

    // A 2x1 streaming texture: left pixel red, right pixel blue.
    [[nodiscard]] texture make_red_blue() {
        auto t = texture::create(dim{2, 1}, texture_access::streaming);
        REQUIRE(t.has_value());
        const std::array <std::uint32_t, 2> px{red_px, blue_px};
        t->update(px);
        return std::move(*t);
    }

    // Brace-init pin: a braced destination keeps selecting the integer-rect overload.
    [[maybe_unused]] void brace_init_selects_integer_rect(const texture& t) {
        draw_texture(t, {0, 0, 4, 4});
        draw_texture(t, render_rect{0.5f, 0.5f, 4.0f, 4.0f});
    }
}

TEST_SUITE("neutrino::video::texture") {
    TEST_CASE("create reports size and access, and rejects degenerate sizes") {
        test::test_application app("texture create");
        auto t = texture::create(dim{8, 4}, texture_access::target);
        REQUIRE(t.has_value());
        CHECK(t->size() == dim{8, 4});
        CHECK(t->access() == texture_access::target);
        CHECK_FALSE(texture::create(dim{0, 4}, texture_access::target).has_value());
    }

    TEST_CASE("compose draws into a target; to_image reads it back") {
        test::test_application app("texture compose");
        auto t = make_target(4, 4);
        t.compose([] { draw_rect_fill(rect{0, 0, 2, 4}, colors::red); }, colors::blue);
        const image img = t.to_image();
        CHECK(same_rgb(pixel_at(img, 0, 0), colors::red));
        CHECK(same_rgb(pixel_at(img, 3, 0), colors::blue));
    }

    TEST_CASE("composing with no clear colour keeps the existing contents") {
        test::test_application app("texture persistent compose");
        auto t = make_target(4, 4);
        t.compose([] { draw_rect_fill(rect{0, 0, 2, 4}, colors::red); }, colors::blue);
        t.compose([] { draw_rect_fill(rect{2, 0, 2, 4}, colors::lime_green); }, std::nullopt);
        const image img = t.to_image();
        CHECK(same_rgb(pixel_at(img, 0, 0), colors::red));  // survived the second compose
        CHECK(same_rgb(pixel_at(img, 3, 0), colors::lime_green));
    }

    TEST_CASE("bind scopes nest and restore the previous target") {
        test::test_application app("texture bind nesting");
        auto outer = make_target(4, 4);
        auto inner = make_target(4, 4);
        {
            auto a = outer.bind(colors::black);
            {
                auto b = inner.bind(colors::black);
                draw_rect_fill(rect{0, 0, 4, 4}, colors::red); // lands in inner
            }
            draw_rect_fill(rect{0, 0, 1, 1}, colors::blue);    // back in outer
        }
        CHECK(same_rgb(pixel_at(inner.to_image(), 0, 0), colors::red));
        const image o = outer.to_image();
        CHECK(same_rgb(pixel_at(o, 0, 0), colors::blue));
        CHECK(same_rgb(pixel_at(o, 3, 3), colors::black));    // inner's red never reached outer
    }

    TEST_CASE("a moved-from or ended scope does not restore twice") {
        test::test_application app("texture scope move");
        auto t = make_target(2, 2);
        std::optional <target_scope> held;
        held.emplace(t.bind(colors::black));
        target_scope moved = std::move(*held);
        held.reset();                                           // moved-from: no restore
        draw_rect_fill(rect{0, 0, 2, 2}, colors::red);          // still drawing into t
        moved.end();
        moved.end();                                            // idempotent
        CHECK(same_rgb(pixel_at(t.to_image(), 1, 1), colors::red));
    }

    TEST_CASE("streaming textures upload through update() and lock()") {
        test::test_application app("texture streaming");
        auto src = make_red_blue();
        auto out = make_target(2, 1);
        out.compose([&] { draw_texture(src); }, colors::black);
        CHECK(same_rgb(pixel_at(out.to_image(), 0, 0), colors::red));
        CHECK(same_rgb(pixel_at(out.to_image(), 1, 0), colors::blue));

        {
            auto px = src.lock();
            REQUIRE(px.width() == 2);
            px.row(0)[0] = blue_px;
            px.row(0)[1] = red_px;
        }                                                       // uploaded here
        out.compose([&] { draw_texture(src); }, colors::black);
        CHECK(same_rgb(pixel_at(out.to_image(), 0, 0), colors::blue));
        CHECK(same_rgb(pixel_at(out.to_image(), 1, 0), colors::red));
    }

    TEST_CASE("from_image uploads an image as a fixed texture") {
        test::test_application app("texture from_image");
        auto img = image::create_rgb(3, 2, pixel_format::RGBA8888);
        REQUIRE(img.has_value());
        REQUIRE(img->fill(colors::lime_green).has_value());
        auto t = texture::from_image(*img);
        REQUIRE(t.has_value());
        CHECK(t->size() == dim{3, 2});
        CHECK(t->access() == texture_access::fixed);
        auto out = make_target(3, 2);
        out.compose([&] { draw_texture(*t); }, colors::black);
        CHECK(same_rgb(pixel_at(out.to_image(), 2, 1), colors::lime_green));
    }

    TEST_CASE("operations for the wrong access mode are rejected") {
        test::test_application app("texture access misuse");
        auto target = make_target(2, 1);
        auto stream = make_red_blue();
        const std::array <std::uint32_t, 2> px{red_px, red_px};
        CHECK_THROWS(target.update(px));
        CHECK_THROWS((void) target.lock());
        CHECK_THROWS((void) stream.bind());
        CHECK_THROWS((void) stream.to_image());
        const std::array <std::uint32_t, 1> wrong_size{red_px};
        CHECK_THROWS(stream.update(wrong_size));
        CHECK_THROWS((void) texture::create(dim{2, 2}, texture_access::streaming, pixel_format::RGB24));
    }

    TEST_CASE("draw_texture honours src, flip and tint") {
        test::test_application app("texture draw params");
        auto src = make_red_blue();
        auto out = make_target(2, 1);

        out.compose([&] { draw_texture(src, rect{0, 0, 2, 1}, {.flip = sprite_flip::horizontal}); },
                    colors::black);
        CHECK(same_rgb(pixel_at(out.to_image(), 0, 0), colors::blue));
        CHECK(same_rgb(pixel_at(out.to_image(), 1, 0), colors::red));

        out.compose([&] { draw_texture(src, rect{0, 0, 2, 1}, {.src = rect{1, 0, 1, 1}}); }, colors::black);
        CHECK(same_rgb(pixel_at(out.to_image(), 0, 0), colors::blue)); // the right half, stretched
        CHECK(same_rgb(pixel_at(out.to_image(), 1, 0), colors::blue));

        out.compose([&] { draw_texture(src, rect{0, 0, 2, 1}, {.tint = colors::black}); }, colors::white);
        CHECK(same_rgb(pixel_at(out.to_image(), 0, 0), colors::black)); // colour-modulated to black

        out.compose([&] { draw_texture(src); }, colors::black);       // tint did not stick
        CHECK(same_rgb(pixel_at(out.to_image(), 0, 0), colors::red));

        CHECK_THROWS(draw_texture(src, rect{0, 0, 2, 1}, {.flip = sprite_flip::diagonal}));
    }

    TEST_CASE("draw_texture_quad maps the texture onto arbitrary corners") {
        test::test_application app("texture quad");
        auto src = make_red_blue();
        auto out = make_target(4, 2);
        // Corners in TL, TR, BR, BL order, mirrored horizontally: blue ends up on the left.
        out.compose([&] {
            draw_texture_quad(src, {render_pos{4.0f, 0.0f}, render_pos{0.0f, 0.0f},
                                    render_pos{0.0f, 2.0f}, render_pos{4.0f, 2.0f}});
        }, colors::black);
        const image img = out.to_image();
        CHECK(same_rgb(pixel_at(img, 0, 1), colors::blue));
        CHECK(same_rgb(pixel_at(img, 3, 1), colors::red));
    }

    TEST_CASE("capture_frame reads the currently bound target") {
        test::test_application app("texture capture");
        auto t = make_target(2, 2);
        image captured;
        t.compose([&] {
            draw_rect_fill(rect{0, 0, 2, 2}, colors::red);
            captured = capture_frame();
        }, colors::black);
        CHECK(same_rgb(pixel_at(captured, 1, 1), colors::red));
    }

    TEST_CASE("adopt wraps a raw texture without applying engine defaults") {
        test::test_application app("texture adopt");
        auto raw = sdlpp::texture::create(get_renderer(), pixel_format::RGBA8888, sdlpp::texture_access::target, 5, 3);
        REQUIRE(raw.has_value());
        const auto t = texture::adopt(std::move(*raw), texture_access::target);
        CHECK(t.size() == dim{5, 3});
        CHECK(t.access() == texture_access::target);
    }
}
