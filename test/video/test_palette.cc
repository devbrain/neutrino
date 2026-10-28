//
// Tests for load_palette: input validation, the vga6 expansion, and rgb8 passthrough.
//
#include <doctest/doctest.h>

#include <neutrino/video/palette.hh>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

using namespace neutrino;

namespace {
    std::vector <std::uint8_t> filled(std::size_t entries, std::uint8_t value) {
        return std::vector <std::uint8_t>(entries * 3, value);
    }
}

TEST_SUITE("neutrino::load_palette") {
    TEST_CASE("rejects an empty table") {
        const std::vector <std::uint8_t> bytes;
        CHECK_THROWS_AS((void)load_palette(bytes, palette_encoding::rgb8), std::runtime_error);
    }

    TEST_CASE("rejects a length that is not a multiple of 3") {
        const std::vector <std::uint8_t> bytes{1, 2, 3, 4};
        CHECK_THROWS_AS((void)load_palette(bytes, palette_encoding::rgb8), std::runtime_error);
    }

    TEST_CASE("accepts exactly 256 entries") {
        const auto bytes = filled(256, 10);
        const palette pal = load_palette(bytes, palette_encoding::rgb8);
        CHECK(pal.size() == 256);
    }

    TEST_CASE("rejects more than 256 entries") {
        const auto bytes = filled(257, 10);
        CHECK_THROWS_AS((void)load_palette(bytes, palette_encoding::rgb8), std::runtime_error);
    }

    TEST_CASE("the entry count is bytes / 3") {
        const auto bytes = filled(16, 0);
        CHECK(load_palette(bytes, palette_encoding::rgb8).size() == 16);
    }

    TEST_CASE("vga6 rejects a channel above 63") {
        const std::vector <std::uint8_t> bytes{0, 64, 0};
        CHECK_THROWS_AS((void)load_palette(bytes, palette_encoding::vga6), std::runtime_error);
    }

    TEST_CASE("vga6 expands 0..63 to the full 0..255 range") {
        const std::vector <std::uint8_t> bytes{0, 32, 63};
        const palette pal = load_palette(bytes, palette_encoding::vga6);
        CHECK(pal.get_color(0) == color{0, 130, 255, 255});
    }

    TEST_CASE("rgb8 stores channels as-is, opaque") {
        const std::vector <std::uint8_t> bytes{12, 200, 255, 0, 64, 128};
        const palette pal = load_palette(bytes, palette_encoding::rgb8);
        CHECK(pal.get_color(0) == color{12, 200, 255, 255});
        CHECK(pal.get_color(1) == color{0, 64, 128, 255});
    }
}
