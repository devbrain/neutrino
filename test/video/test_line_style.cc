//
// Tests for neutrino's line_style and its internal mapping to sdlpp's (video/line_style_sdl.hh).
//
#include <doctest/doctest.h>

#include <neutrino/video/draw.hh>
#include <neutrino/video/line_style.hh>
#include <neutrino/video/texture.hh>

#include "test_application.hh"
#include "video/line_style_sdl.hh"

#include <array>

using namespace neutrino;
using neutrino::detail::from_sdl_line;
using neutrino::detail::to_sdl_line;

namespace {
    // One style of each pattern, with lengths that differ from the defaults.
    constexpr std::array STYLES{
        line_style::solid(),
        line_style::dashed(3.0f, 5.0f),
        line_style::dotted(6.0f),
    };

    // A horizontal line from x = 0 to x = 24 on row 1 of a transparent 32 x 4 target, drawn
    // in white with @p style; returns whether the pixel at (x, 1) is lit.
    [[nodiscard]] std::array <bool, 25> draw_row(const line_style& style) {
        auto t = texture::create(dim{32, 4}, texture_access::target);
        REQUIRE(t.has_value());
        t->compose([&] {
            set_line_style(style);
            draw_line(0, 1, 24, 1, colors::white);
        }, colors::transparent);
        const image img = t->to_image();
        std::array <bool, 25> lit{};
        for (int x = 0; x <= 24; ++x) {
            const auto px = img.get_pixel(x, 1);
            REQUIRE(px.has_value());
            lit[static_cast <std::size_t>(x)] = px->a != 0;
        }
        return lit;
    }
}

TEST_SUITE("neutrino::line_style") {
    TEST_CASE("the factories set the pattern and its lengths") {
        CHECK(line_style{} == line_style::solid());
        CHECK(line_style::solid().pattern == line_pattern::solid);

        const auto d = line_style::dashed(3.0f, 5.0f);
        CHECK(d.pattern == line_pattern::dashed);
        CHECK(d.dash == 3.0f);
        CHECK(d.gap == 5.0f);

        const auto o = line_style::dotted(6.0f);
        CHECK(o.pattern == line_pattern::dotted);
        CHECK(o.spacing == 6.0f);

        CHECK(line_style::dashed() == line_style::dashed(8.0f, 8.0f));   // sdlpp's defaults
        CHECK(line_style::dotted() == line_style::dotted(4.0f));
    }

    TEST_CASE("every pattern maps to sdlpp's and back with its lengths") {
        CHECK(to_sdl_line(line_style::solid()).type == sdlpp::line_style_type::solid);
        CHECK(to_sdl_line(line_style::dashed()).type == sdlpp::line_style_type::dashed);
        CHECK(to_sdl_line(line_style::dotted()).type == sdlpp::line_style_type::dotted);

        const auto sdl = to_sdl_line(line_style::dashed(3.0f, 5.0f));
        CHECK(sdl.dash == 3.0f);
        CHECK(sdl.gap == 5.0f);
        CHECK(to_sdl_line(line_style::dotted(6.0f)).spacing == 6.0f);

        for (const auto& style : STYLES) {
            CHECK(from_sdl_line(to_sdl_line(style)) == style);
        }
        CHECK(from_sdl_line(sdlpp::line_style::dashed(2.0f, 7.0f)) == line_style::dashed(2.0f, 7.0f));
    }

    TEST_CASE("set_line_style reaches the renderer; get_line_style reads it back") {
        test::test_application app("line style state");
        CHECK(get_line_style() == line_style::solid());                  // the renderer's default
        for (const auto& style : STYLES) {
            set_line_style(style);
            CHECK(get_line_style() == style);
        }
        set_line_style(line_style::solid());
    }

    TEST_CASE("draw_line draws the pattern that was set") {
        test::test_application app("line style pixels");

        const auto solid = draw_row(line_style::solid());
        CHECK(solid[2]);
        CHECK(solid[6]);

        // Dashes of 4 every 8 pixels: [0, 4], [8, 12], [16, 20], [24, 24].
        const auto dashed = draw_row(line_style::dashed(4.0f, 4.0f));
        CHECK(dashed[2]);
        CHECK_FALSE(dashed[6]);                                          // inside the first gap
        CHECK(dashed[10]);
        CHECK_FALSE(dashed[14]);

        // A dot every 4 pixels: 0, 4, 8, ...
        const auto dotted = draw_row(line_style::dotted(4.0f));
        CHECK(dotted[4]);
        CHECK(dotted[8]);
        CHECK_FALSE(dotted[2]);
        CHECK_FALSE(dotted[6]);

        set_line_style(line_style::solid());
    }
}
