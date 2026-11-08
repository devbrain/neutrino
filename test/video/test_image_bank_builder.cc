//
// Tests for image_bank_builder: pictures share the bank's format, palette and colour key;
// positions and ids (sparse, repeated); mismatched pictures are rejected; an indexed bank builds
// an indexed atlas.
//
#include <doctest/doctest.h>

#include "image_atlas_access.hh"
#include <neutrino/video/atlas/image_atlas.hh>
#include <neutrino/video/image/image_bank_builder.hh>

#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

using namespace neutrino;

namespace {
    palette grey_ramp(int entries) {
        std::vector <std::uint8_t> bytes;
        for (int i = 0; i < entries; i++) {
            const auto v = static_cast <std::uint8_t>(i * 10);
            bytes.insert(bytes.end(), {v, v, v});
        }
        return load_palette(bytes, palette_encoding::rgb8);
    }

    std::uint8_t index_at(const image& img, int x, int y) {
        return static_cast <const std::uint8_t*>(img.get_pixels())[static_cast <std::size_t>(y) * img.pitch() + x];
    }
}

TEST_SUITE("neutrino::image_bank_builder") {
    TEST_CASE("indexed pictures share the bank's palette and colour key") {
        const palette pal = grey_ramp(17);
        auto bank = indexed_image_bank_builder(pal, std::uint8_t{16});

        for (int i = 0; i < 3; i++) {
            auto img = bank.next({4 + i, 2});
            img.set(0, 0, static_cast <std::uint8_t>(i + 1));
            CHECK(bank.add(std::move(img), static_cast <std::uint32_t>(100 + i)) == static_cast <std::size_t>(i));
        }
        CHECK(bank.size() == 3);

        const image_bank result = std::move(bank).build();
        REQUIRE(result.entries.size() == 3);
        for (std::size_t i = 0; i < 3; i++) {
            CAPTURE(i);
            const auto& e = result.entries[i];
            CHECK(e.id == 100 + i);
            CHECK(e.pixels.format() == pixel_format::INDEX8);
            CHECK(e.pixels.width() == 4 + i);
            CHECK(e.pixels.get_palette().get() == pal.get());
            CHECK(e.pixels.color_key() == std::optional <std::uint32_t>{16u});
            CHECK(index_at(e.pixels, 0, 0) == i + 1);
        }
    }

    TEST_CASE("a palette taken over by the builder outlives its original handle") {
        image_bank result;
        {
            auto bank = indexed_image_bank_builder(grey_ramp(4));
            auto moved = std::move(bank); // the owned palette moves with the builder
            auto img = moved.next({2, 2});
            img.set(1, 1, std::uint8_t{3});
            moved.add(std::move(img), 0);
            result = std::move(moved).build();
        }
        const auto pal = result.entries[0].pixels.get_palette();
        REQUIRE(pal);
        CHECK(pal.size() == 4);
        CHECK(index_at(result.entries[0].pixels, 1, 1) == 3);
    }

    TEST_CASE("an rgba bank holds RGBA32 pictures") {
        auto bank = rgba_image_bank_builder();
        auto img = bank.next({3, 3});
        img.set(2, 2, color{1, 2, 3, 4});
        bank.add(std::move(img), 9);

        const image_bank result = std::move(bank).build();
        REQUIRE(result.entries.size() == 1);
        CHECK(result.entries[0].pixels.format() == pixel_format::RGBA32);
        CHECK(result.entries[0].id == 9);
        const auto px = result.entries[0].pixels.get_pixel(2, 2);
        REQUIRE(px.has_value());
        CHECK(*px == color{1, 2, 3, 4});
    }

    TEST_CASE("ids are the game's: sparse and repeated ids are kept as given") {
        auto bank = rgba_image_bank_builder();
        CHECK(bank.add(bank.next({2, 2}), 5) == 0);
        CHECK(bank.add(bank.next({2, 2}), 1) == 1);
        CHECK(bank.add(bank.next({2, 2}), 5) == 2);

        const image_bank result = std::move(bank).build();
        REQUIRE(result.entries.size() == 3);
        CHECK(result.entries[0].id == 5);
        CHECK(result.entries[1].id == 1);
        CHECK(result.entries[2].id == 5);

        const image_atlas a = build_atlas(result);
        CHECK(a.find_by_id(1).value().picture_idx == 1);
        CHECK(a.find_by_id(5).value().picture_idx == 0); // the first of the two
        CHECK_FALSE(a.find_by_id(2).has_value());
    }

    TEST_CASE("a picture that does not match the bank is rejected") {
        const palette pal = grey_ramp(8);
        const palette other = grey_ramp(8);

        auto indexed = indexed_image_bank_builder(pal, std::uint8_t{0});
        CHECK_THROWS_AS(indexed.add(indexed_image_builder({2, 2}, other, std::uint8_t{0}), 0), std::runtime_error);
        CHECK_THROWS_AS(indexed.add(indexed_image_builder({2, 2}, pal, std::uint8_t{1}), 0), std::runtime_error);
        CHECK_THROWS_AS(indexed.add(indexed_image_builder({2, 2}, pal), 0), std::runtime_error);
        CHECK(indexed.size() == 0);
        // the same palette and key, made by hand, is accepted
        CHECK(indexed.add(indexed_image_builder({2, 2}, pal, std::uint8_t{0}), 0) == 0);

        // an RGBA bank takes only RGBA builders, checked at compile time
        static_assert(!std::is_invocable_v <decltype(&rgba_image_bank_builder::add), rgba_image_bank_builder&,
                                            indexed_image_builder&&, std::uint32_t>);
    }

    TEST_CASE("an indexed bank builds an indexed atlas") {
        const palette pal = grey_ramp(17);
        auto bank = indexed_image_bank_builder(pal, std::uint8_t{16});
        bank.add(bank.next({8, 8}), 10);
        bank.add(bank.next({4, 12}), 30);

        const image_atlas a = build_atlas(std::move(bank).build());
        REQUIRE(atlas_test::page_count(a) == 1);
        CHECK(atlas_test::page(a, 0).format() == pixel_format::INDEX8);
        CHECK(a.picture_count() == 2);
        CHECK(a.find_by_id(30).value().picture_idx == 1);
        CHECK(a.picture_size(1) == dim{4, 12});
    }
}
