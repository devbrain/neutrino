//
// Tests for image_builder: set (indexed and RGBA), row access, bounds and format checks.
//
#include <doctest/doctest.h>

#include <neutrino/video/image/image_builder.hh>

#include <cstdint>
#include <stdexcept>
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
        auto b = image_builder::indexed({4, 3}, grey_ramp(16));
        b.set(0, 0, std::uint8_t{5});
        b.set(3, 2, std::uint8_t{9});
        const image img = std::move(b).build();
        CHECK(img.format() == pixel_format::INDEX8);
        const auto* px = static_cast <const std::uint8_t*>(img.get_pixels());
        CHECK(px[0] == 5);
        CHECK(px[2 * img.pitch() + 3] == 9);
    }

    TEST_CASE("indexed row spans exactly the image width") {
        auto b = image_builder::indexed({7, 2}, grey_ramp(4));
        const auto r = b.row(1);
        CHECK(r.size() == 7);
        r[6] = 3;
        const image img = std::move(b).build();
        const auto* px = static_cast <const std::uint8_t*>(img.get_pixels());
        CHECK(px[img.pitch() + 6] == 3);
    }

    TEST_CASE("row and set address the same pixel") {
        auto b = image_builder::indexed({5, 5}, grey_ramp(8));
        b.set(2, 4, std::uint8_t{7});
        CHECK(b.row(4)[2] == 7);
    }

    TEST_CASE("indexed transparent index becomes the colour key") {
        auto b = image_builder::indexed({2, 2}, grey_ramp(17), std::uint8_t{16});
        const image img = std::move(b).build();
        const auto key = img.color_key();
        REQUIRE(key.has_value());
        CHECK(*key == 16u);
    }

    TEST_CASE("rgba set reads back through the image") {
        auto b = image_builder::rgba({3, 3});
        b.set(1, 2, color{10, 20, 30, 40});
        image img = std::move(b).build();
        CHECK(img.format() == pixel_format::RGBA32);
        const auto px = img.get_pixel(1, 2);
        REQUIRE(px.has_value());
        CHECK(*px == color{10, 20, 30, 40});
    }

    TEST_CASE("rgba row spans four bytes per pixel") {
        auto b = image_builder::rgba({6, 1});
        CHECK(b.row(0).size() == 24);
    }

    TEST_CASE("rgba row bytes are R, G, B, A on every platform") {
        auto b = image_builder::rgba({2, 1});
        const auto r = b.row(0);
        r[4] = 50;
        r[5] = 60;
        r[6] = 70;
        r[7] = 80;
        image img = std::move(b).build();
        const auto px = img.get_pixel(1, 0);
        REQUIRE(px.has_value());
        CHECK(*px == color{50, 60, 70, 80});
    }

    TEST_CASE("out-of-range pixels and rows throw") {
        auto b = image_builder::indexed({4, 4}, grey_ramp(2));
        CHECK_THROWS_AS(b.set(4, 0, std::uint8_t{1}), std::runtime_error);
        CHECK_THROWS_AS(b.set(0, -1, std::uint8_t{1}), std::runtime_error);
        CHECK_THROWS_AS((void)b.row(4), std::runtime_error);
    }

    TEST_CASE("the setter must match the image format") {
        auto ib = image_builder::indexed({2, 2}, grey_ramp(2));
        CHECK_THROWS_AS(ib.set(0, 0, color{1, 2, 3, 4}), std::runtime_error);
        auto rb = image_builder::rgba({2, 2});
        CHECK_THROWS_AS(rb.set(0, 0, std::uint8_t{1}), std::runtime_error);
    }
}
