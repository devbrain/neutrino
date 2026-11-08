//
// Tests for image_builder: set and fill (indexed and RGBA), row access, bounds, and the
// kind-specific API (palette and key only on indexed builders).
//
#include <doctest/doctest.h>

#include <neutrino/video/image/image_builder.hh>

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <vector>

using namespace neutrino;

namespace {
    palette grey_ramp(int entries) {
        std::vector <std::uint8_t> bytes;
        for (int i = 0; i < entries; i++) {
            const auto v = static_cast <std::uint8_t>(i);
            bytes.insert(bytes.end(), {v, v, v});
        }
        return load_palette(bytes, palette_encoding::rgb8);
    }
}

TEST_SUITE("neutrino::image_builder") {
    TEST_CASE("indexed set writes the index") {
        auto b = indexed_image_builder({4, 3}, grey_ramp(16));
        b.set(0, 0, std::uint8_t{5});
        b.set(3, 2, std::uint8_t{9});
        const image img = std::move(b).build();
        CHECK(img.format() == pixel_format::INDEX8);
        const auto* px = static_cast <const std::uint8_t*>(img.get_pixels());
        CHECK(px[0] == 5);
        CHECK(px[2 * img.pitch() + 3] == 9);
    }

    TEST_CASE("indexed row spans exactly the image width") {
        auto b = indexed_image_builder({7, 2}, grey_ramp(4));
        const auto r = b.row(1);
        CHECK(r.size() == 7);
        r[6] = 3;
        const image img = std::move(b).build();
        const auto* px = static_cast <const std::uint8_t*>(img.get_pixels());
        CHECK(px[img.pitch() + 6] == 3);
    }

    TEST_CASE("row and set address the same pixel") {
        auto b = indexed_image_builder({5, 5}, grey_ramp(8));
        b.set(2, 4, std::uint8_t{7});
        CHECK(b.row(4)[2] == 7);
    }

    TEST_CASE("indexed transparent index becomes the colour key") {
        auto b = indexed_image_builder({2, 2}, grey_ramp(17), std::uint8_t{16});
        const image img = std::move(b).build();
        const auto key = img.color_key();
        REQUIRE(key.has_value());
        CHECK(*key == 16u);
    }

    TEST_CASE("rgba set reads back through the image") {
        auto b = rgba_image_builder({3, 3});
        b.set(1, 2, color{10, 20, 30, 40});
        image img = std::move(b).build();
        CHECK(img.format() == pixel_format::RGBA32);
        const auto px = img.get_pixel(1, 2);
        REQUIRE(px.has_value());
        CHECK(*px == color{10, 20, 30, 40});
    }

    TEST_CASE("rgba row spans one colour per pixel") {
        auto b = rgba_image_builder({6, 1});
        CHECK(b.row(0).size() == 6);
    }

    TEST_CASE("rgba row colours are bytes R, G, B, A on every platform") {
        auto b = rgba_image_builder({2, 1});
        b.row(0)[1] = color{50, 60, 70, 80};
        image img = std::move(b).build();
        const auto* bytes = static_cast <const std::uint8_t*>(img.get_pixels());
        CHECK(bytes[4] == 50);
        CHECK(bytes[5] == 60);
        CHECK(bytes[6] == 70);
        CHECK(bytes[7] == 80);
        const auto px = img.get_pixel(1, 0);
        REQUIRE(px.has_value());
        CHECK(*px == color{50, 60, 70, 80});
    }

    TEST_CASE("indexed fill sets every pixel, skipping row padding") {
        auto b = indexed_image_builder({5, 3}, grey_ramp(8), std::uint8_t{7});
        b.fill(7);
        b.set(2, 1, std::uint8_t{3});
        CHECK(b.size() == dim{5, 3});
        const image img = std::move(b).build();
        const auto* px = static_cast <const std::uint8_t*>(img.get_pixels());
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 5; x++) {
                CAPTURE(x);
                CAPTURE(y);
                CHECK(px[static_cast <std::size_t>(y) * img.pitch() + x] == (x == 2 && y == 1 ? 3 : 7));
            }
        }
    }

    TEST_CASE("rgba fill sets every pixel") {
        auto b = rgba_image_builder({3, 2});
        b.fill(color{255, 255, 255, 0});
        b.set(0, 1, color{1, 2, 3, 4});
        image img = std::move(b).build();
        for (int y = 0; y < 2; y++) {
            for (int x = 0; x < 3; x++) {
                CAPTURE(x);
                CAPTURE(y);
                const auto px = img.get_pixel(x, y);
                REQUIRE(px.has_value());
                CHECK(*px == (x == 0 && y == 1 ? color{1, 2, 3, 4} : color{255, 255, 255, 0}));
            }
        }
    }

    TEST_CASE("out-of-range pixels and rows throw") {
        auto b = indexed_image_builder({4, 4}, grey_ramp(2));
        CHECK_THROWS_AS(b.set(4, 0, std::uint8_t{1}), std::runtime_error);
        CHECK_THROWS_AS(b.set(0, -1, std::uint8_t{1}), std::runtime_error);
        CHECK_THROWS_AS((void)b.row(4), std::runtime_error);
    }

    TEST_CASE("palette and colour key exist only for indexed builders") {
        // Compile-time checks; the test case only gives them a name in the report.
        static_assert(std::is_constructible_v <indexed_image_builder, dim>);
        static_assert(std::is_constructible_v <indexed_image_builder, dim, const palette&>);
        static_assert(std::is_constructible_v <indexed_image_builder, dim, const palette&, std::uint8_t>);
        static_assert(std::is_constructible_v <rgba_image_builder, dim>);
        static_assert(!std::is_constructible_v <rgba_image_builder, dim, const palette&>);
        static_assert(!std::is_constructible_v <rgba_image_builder, dim, std::optional <std::uint8_t>>);
        static_assert(std::is_same_v <indexed_image_builder::pixel_type, std::uint8_t>);
        static_assert(std::is_same_v <rgba_image_builder::pixel_type, color>);
        CHECK(true);
    }
}
