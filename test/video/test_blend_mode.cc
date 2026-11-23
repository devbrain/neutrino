//
// Tests for neutrino's blend_mode and alpha_mode, and their internal mapping to SDL's
// (video/blend_mode_sdl.hh): the painter's blend for every draw, in the form the source's
// alpha mode needs (painter.md §2.8, R49).
//
#include <doctest/doctest.h>

#include <neutrino/video/alpha_mode.hh>
#include <neutrino/video/blend_mode.hh>
#include <neutrino/video/draw.hh>
#include <neutrino/video/texture.hh>

#include "test_application.hh"
#include "video/blend_mode_sdl.hh"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <set>
#include <tuple>
#include <utility>

using namespace neutrino;
using neutrino::detail::to_sdl_blend;

namespace {
    // Every neutrino blend mode with the SDL preset it stands for from a straight source and
    // from a premultiplied one, written out by name so the test does not depend on either
    // side's numbers.
    constexpr std::array MODES{
        std::tuple{blend_mode::none, sdlpp::blend_mode::none, sdlpp::blend_mode::none},
        std::tuple{blend_mode::blend, sdlpp::blend_mode::blend, sdlpp::blend_mode::blend_premultiplied},
        std::tuple{blend_mode::add, sdlpp::blend_mode::add, sdlpp::blend_mode::add_premultiplied},
        std::tuple{blend_mode::mul, sdlpp::blend_mode::mul, sdlpp::blend_mode::mul},
        std::tuple{blend_mode::mod, sdlpp::blend_mode::mod, sdlpp::blend_mode::mod},
    };

    [[nodiscard]] texture make_target(int w, int h) {
        auto t = texture::create(dim{w, h}, texture_access::target);
        REQUIRE(t.has_value());
        return std::move(*t);
    }

    // A 1x1 fixed texture holding @p c, straight.
    [[nodiscard]] texture make_fixed(const color& c) {
        auto img = image::create_rgb(1, 1, pixel_format::RGBA8888);
        REQUIRE(img.has_value());
        REQUIRE(img->fill(c).has_value());
        auto t = texture::from_image(*img);
        REQUIRE(t.has_value());
        return std::move(*t);
    }

    [[nodiscard]] sdlpp::blend_mode blend_of(const texture& t) {
        const auto got = t.native().get_blend_mode();
        REQUIRE(got.has_value());
        return *got;
    }

    [[nodiscard]] color pixel(const image& img, int x, int y) {
        const auto c = img.get_pixel(x, y);
        REQUIRE(c.has_value());
        return *c;
    }

    // Equal within @p tolerance per channel: the GPU rounds blended channels.
    [[nodiscard]] bool near(const color& a, const color& b, int tolerance = 1) {
        const auto close = [tolerance](std::uint8_t x, std::uint8_t y) {
            return std::abs(static_cast <int>(x) - static_cast <int>(y)) <= tolerance;
        };
        return close(a.r, b.r) && close(a.g, b.g) && close(a.b, b.b) && close(a.a, b.a);
    }
}

TEST_SUITE("neutrino::blend_mode") {
    TEST_CASE("every mode maps to its SDL preset, in the form its source needs") {
        std::set <sdlpp::blend_mode> seen;
        for (const auto& [mode, straight, premultiplied] : MODES) {
            CAPTURE(static_cast <std::uint32_t>(mode));
            CHECK(to_sdl_blend(mode) == straight);    // straight by default: primitives
            CHECK(to_sdl_blend(mode, alpha_mode::straight) == straight);
            CHECK(to_sdl_blend(mode, alpha_mode::premultiplied) == premultiplied);
            seen.insert(straight);
        }
        CHECK(seen.size() == MODES.size());           // no two modes share an SDL preset
    }

    TEST_CASE("a value outside the enum maps to SDL's invalid, in both forms") {
        CHECK(to_sdl_blend(static_cast <blend_mode>(99)) == sdlpp::blend_mode::invalid);
        CHECK(to_sdl_blend(static_cast <blend_mode>(99), alpha_mode::premultiplied) == sdlpp::blend_mode::invalid);
    }

    TEST_CASE("the default blend is blend") {
        test::test_application app("blend mode default");
        CHECK(painter_state{}.blend == blend_mode::blend);
        CHECK(get_draw_blend_mode() == blend_mode::blend);
    }

    TEST_CASE("set_draw_blend_mode reaches SDL at the next draw; get_draw_blend_mode reads it back") {
        test::test_application app("blend mode draw");
        for (const auto& [mode, straight, premultiplied] : MODES) {
            CAPTURE(static_cast <std::uint32_t>(mode));
            set_draw_blend_mode(mode);
            CHECK(get_draw_blend_mode() == mode);
            draw_point(0, 0);                         // a draw sets the blend it uses (R48)
            CHECK(get_renderer().get_draw_blend_mode() == straight);   // primitives are straight
        }
    }

    TEST_CASE("a value outside the enum fails, counted, and leaves the mode as it was") {
        test::test_application app("blend mode invalid");
        set_draw_blend_mode(blend_mode::add);
        const auto failures = draw_failure_count();
        set_draw_blend_mode(static_cast <blend_mode>(99));
        CHECK(draw_failure_count() == failures + 1);
        CHECK(get_draw_blend_mode() == blend_mode::add);
    }

    TEST_CASE("a textured draw sets the painter's blend on its source, in the form its alpha mode needs") {
        test::test_application app("blend mode texture");
        auto out = make_target(4, 4);
        auto target = make_target(2, 2);
        auto fixed = make_fixed(colors::red);
        auto binding = out.bind();
        for (const auto& [mode, straight, premultiplied] : MODES) {
            CAPTURE(static_cast <std::uint32_t>(mode));
            set_draw_blend_mode(mode);
            draw_texture(target);
            CHECK(blend_of(target) == premultiplied);
            draw_texture_quad(fixed, {render_pos{0, 0}, render_pos{1, 0}, render_pos{1, 1}, render_pos{0, 1}});
            CHECK(blend_of(fixed) == straight);
            target.set_alpha_mode(alpha_mode::straight);
            draw_texture(target);
            CHECK(blend_of(target) == straight);
            target.set_alpha_mode(alpha_mode::premultiplied);
        }
    }
}

TEST_SUITE("neutrino::alpha_mode") {
    TEST_CASE("a target texture is premultiplied, the others straight; adopt follows the access") {
        test::test_application app("alpha mode defaults");
        CHECK(make_target(2, 2).alpha_mode() == alpha_mode::premultiplied);
        auto streaming = texture::create(dim{2, 2}, texture_access::streaming);
        REQUIRE(streaming.has_value());
        CHECK(streaming->alpha_mode() == alpha_mode::straight);
        CHECK(make_fixed(colors::red).alpha_mode() == alpha_mode::straight);

        const std::array cases{
            std::pair{sdlpp::texture_access::static_access, alpha_mode::straight},
            std::pair{sdlpp::texture_access::streaming, alpha_mode::straight},
            std::pair{sdlpp::texture_access::target, alpha_mode::premultiplied},
        };
        for (const auto& [raw_access, mode] : cases) {
            auto raw = sdlpp::texture::create(get_renderer(), pixel_format::RGBA8888, raw_access, 2, 2);
            REQUIRE(raw.has_value());
            CHECK(texture::adopt(std::move(*raw)).alpha_mode() == mode);
        }

        auto t = make_target(2, 2);
        t.set_alpha_mode(alpha_mode::straight);
        CHECK(t.alpha_mode() == alpha_mode::straight);
    }

    TEST_CASE("a composed texture with translucent pixels keeps them when drawn again") {
        // Blended onto a transparent target, half-transparent red leaves (128, 0, 0, 128):
        // premultiplied. Drawn again with the straight blend, it was multiplied by alpha twice
        // and came out dark; the premultiplied form puts back what was drawn.
        test::test_application app("alpha mode compose");
        auto t = make_target(2, 2);
        t.compose([] { draw_rect_fill(rect{0, 0, 2, 2}, color{255, 0, 0, 128}); });
        auto out = make_target(2, 2);
        out.compose([&] { draw_texture(t); }, colors::black);
        CHECK(near(pixel(out.to_image(), 0, 0), color{128, 0, 0, 255}));

        // What drawing it straight onto black gives too: the same picture.
        auto direct = make_target(2, 2);
        direct.compose([] { draw_rect_fill(rect{0, 0, 2, 2}, color{255, 0, 0, 128}); }, colors::black);
        CHECK(near(pixel(direct.to_image(), 0, 0), pixel(out.to_image(), 0, 0)));
    }

    TEST_CASE("to_image of a premultiplied texture is straight; of a straight one, as it is") {
        test::test_application app("alpha mode to_image");
        auto t = make_target(2, 2);
        t.compose([] {
            draw_point(0, 0, color{255, 0, 0, 128});
            draw_point(1, 0, color{0, 255, 0, 255});
        });
        const image img = t.to_image();
        CHECK(near(pixel(img, 0, 0), color{255, 0, 0, 128}, 2));
        CHECK(pixel(img, 1, 0) == color{0, 255, 0, 255});   // opaque: unchanged
        CHECK(pixel(img, 0, 1).a == 0);                      // transparent stays transparent

        t.set_alpha_mode(alpha_mode::straight);              // read as the pixels are held
        CHECK(near(pixel(t.to_image(), 0, 0), color{128, 0, 0, 128}));
    }

    TEST_CASE("from_image of to_image draws as the texture does") {
        test::test_application app("alpha mode round trip");
        auto t = make_target(2, 2);
        t.compose([] { draw_rect_fill(rect{0, 0, 2, 2}, color{0, 0, 255, 96}); });
        auto copy = texture::from_image(t.to_image());
        REQUIRE(copy.has_value());
        CHECK(copy->alpha_mode() == alpha_mode::straight);
        auto a = make_target(2, 2);
        auto b = make_target(2, 2);
        a.compose([&] { draw_texture(t); }, colors::white);
        b.compose([&] { draw_texture(*copy); }, colors::white);
        CHECK(near(pixel(a.to_image(), 1, 1), pixel(b.to_image(), 1, 1), 2));
    }

    TEST_CASE("sprites and textures drawn with add add") {
        test::test_application app("alpha mode add");
        auto light = make_fixed(color{100, 0, 0, 255});
        auto out = make_target(1, 1);
        out.compose([&] {
            set_draw_blend_mode(blend_mode::add);
            draw_texture(light);
            draw_texture(light);
        }, color{0, 50, 0, 255});
        CHECK(near(pixel(out.to_image(), 0, 0), color{200, 50, 0, 255}));
    }

    TEST_CASE("translucent primitives blend by default") {
        test::test_application app("alpha mode primitives");
        auto out = make_target(1, 1);
        out.compose([] { draw_point(0, 0, color{0, 0, 255, 128}); }, colors::red);
        CHECK(near(pixel(out.to_image(), 0, 0), color{127, 0, 128, 255}));
    }
}
