//
// Tests for the neutrino colour vocabulary: the aliases, the free helpers, and the documented
// behaviour of the colour members (rounding, alpha handling, X11 palette values).
//
#include <doctest/doctest.h>

#include <neutrino/video/color.hh>

#include <cstdint>
#include <type_traits>

using namespace neutrino;

// The aliases are the sdlpp types themselves: no conversion at the sdlpp boundary.
static_assert(std::is_same_v <color, sdlpp::color>);
static_assert(std::is_same_v <fcolor, sdlpp::fcolor>);

// RGBA8888 packing, as texture::update() / lock() expect it.
static_assert(to_rgba32(color{0x12, 0x34, 0x56, 0x78}) == 0x12345678u);
static_assert(from_rgba32(0x12345678u) == color{0x12, 0x34, 0x56, 0x78});

static_assert(colors::transparent.a == 0);
static_assert(color{}.a == 255); // default: opaque black

TEST_SUITE("neutrino::color") {
    TEST_CASE("the palette uses X11 values, not CSS") {
        CHECK(colors::gray == color{190, 190, 190});
        CHECK(colors::dark_gray == color{169, 169, 169});
        CHECK(colors::green == color{0, 255, 0});
        CHECK(colors::f::gray == fcolor{0.5f, 0.5f, 0.5f});
    }

    TEST_CASE("mix and lerp interpolate every channel and truncate") {
        CHECK(colors::black.mix(colors::white, 0.5f) == color{127, 127, 127});
        CHECK(lerp(colors::black, colors::white, 0.0f) == colors::black);
        CHECK(lerp(colors::black, colors::white, 1.0f) == colors::white);
        CHECK(lerp(colors::transparent, colors::black, 0.5f).a == 127);
    }

    TEST_CASE("premultiply scales rgb by alpha and keeps alpha") {
        CHECK(color{200, 100, 50, 128}.premultiply() == color{100, 50, 25, 128});
        CHECK(colors::white.premultiply() == colors::white);
    }

    TEST_CASE("luminance and to_grayscale use BT.709 weights") {
        CHECK(colors::white.luminance() == 255);
        CHECK(colors::red.luminance() == 54);   // 0.2126 * 255
        CHECK(colors::lime_green.to_grayscale() == color{160, 160, 160}); // 160.86, truncated
        CHECK(color{10, 20, 30, 77}.to_grayscale().a == 77);
    }

    TEST_CASE("+ saturates every channel, alpha included") {
        CHECK(color{200, 10, 0, 255} + color{100, 10, 0, 255} == color{255, 20, 0, 255});
        CHECK((color{0, 0, 0, 100} + color{0, 0, 0, 100}).a == 200);
    }

    TEST_CASE("* scales brightness, clamps, and leaves alpha alone") {
        CHECK(color{100, 200, 50, 80} * 0.5f == color{50, 100, 25, 80});
        CHECK(color{100, 200, 50, 80} * 2.0f == color{200, 255, 100, 80});
    }

    TEST_CASE("color <-> fcolor conversion is explicit and truncates") {
        static_assert(!std::is_convertible_v <fcolor, color>);
        static_assert(!std::is_convertible_v <color, fcolor>);
        CHECK(color{fcolor{1.0f, 0.5f, 0.0f, 1.0f}} == color{255, 127, 0, 255});
        CHECK(color{fcolor{2.0f, -1.0f, 0.0f, 1.0f}} == color{255, 0, 0, 255}); // clamped
        const fcolor f{colors::white};
        CHECK(f == fcolor{1.0f, 1.0f, 1.0f, 1.0f});
    }

    TEST_CASE("alpha_blend composites src over dst") {
        CHECK(alpha_blend(colors::white, colors::black) == colors::white);
        CHECK(alpha_blend(colors::transparent, colors::red) == colors::red);
        const color half_white{255, 255, 255, 128};
        CHECK(alpha_blend(half_white, colors::black).r == 128);
    }
}
