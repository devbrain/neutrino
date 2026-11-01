//
// Tests for build_atlas / image_atlas: regions follow bank positions, pixels land in their
// regions, gutters are extruded, empty space is transparent, indexed banks stay indexed,
// and output-format / size errors throw.
//
#include <doctest/doctest.h>

#include <neutrino/video/atlas/image_atlas.hh>
#include <neutrino/video/image/image_builder.hh>
#include <neutrino/video/palette.hh>

#include <cstdint>
#include <stdexcept>
#include <vector>

using namespace neutrino;

namespace {
    palette ramp(int entries, std::uint8_t step) {
        std::vector <std::uint8_t> bytes;
        for (int i = 0; i < entries; i++) {
            const auto v = static_cast <std::uint8_t>(i * step);
            bytes.insert(bytes.end(), {v, static_cast <std::uint8_t>(255 - v), v});
        }
        return load_palette(bytes, palette_encoding::rgb8);
    }

    image solid_rgba(dim size, color c) {
        auto b = image_builder::rgba(size);
        for (int y = 0; y < size.height; y++) {
            for (int x = 0; x < size.width; x++) {
                b.set(x, y, c);
            }
        }
        return std::move(b).build();
    }

    // Indexed picture whose pixel (x, y) is (x + y) % 4 + 1, with (0, 0) set to the key when given.
    image pattern_indexed(dim size, const palette& pal, std::optional <std::uint8_t> key) {
        auto b = image_builder::indexed(size, pal, key);
        for (int y = 0; y < size.height; y++) {
            for (int x = 0; x < size.width; x++) {
                b.set(x, y, static_cast <std::uint8_t>((x + y) % 4 + 1));
            }
        }
        if (key) {
            b.set(0, 0, *key);
        }
        return std::move(b).build();
    }

    color pixel(const image& img, int x, int y) {
        const auto c = img.get_pixel(x, y);
        REQUIRE(c.has_value());
        return *c;
    }

    std::uint8_t index_at(const image& img, int x, int y) {
        return static_cast <const std::uint8_t*>(img.get_pixels())[static_cast <std::size_t>(y) * img.pitch() + x];
    }

    bool inside_any_cell(const image_atlas& a, unsigned page, int x, int y, int gutter) {
        for (std::size_t i = 0; i < a.region_count(); i++) {
            const auto& r = a.region(i);
            if (r.page == page && r.dest.w > 0 &&
                x >= r.dest.x - gutter && x < r.dest.x + r.dest.w + gutter &&
                y >= r.dest.y - gutter && y < r.dest.y + r.dest.h + gutter) {
                return true;
            }
        }
        return false;
    }

    constexpr int gutter = 1; // build_atlas's gutter
}

TEST_SUITE("neutrino::build_atlas") {
    TEST_CASE("an empty bank gives an empty atlas") {
        const image_bank bank;
        const auto a = build_atlas(bank);
        CHECK(a.page_count() == 0);
        CHECK(a.region_count() == 0);
    }

    TEST_CASE("each region holds its bank entry's pixels and id") {
        image_bank bank;
        bank.entries.push_back({solid_rgba({10, 20}, color{255, 0, 0, 255}), 7});
        bank.entries.push_back({solid_rgba({4, 4}, color{0, 255, 0, 255}), 3});
        bank.entries.push_back({solid_rgba({6, 9}, color{0, 0, 255, 255}), 11});

        const auto a = build_atlas(bank);
        REQUIRE(a.region_count() == 3);
        REQUIRE(a.page_count() == 1);
        CHECK(a.page(0).format() == pixel_format::RGBA32);

        const color expected[] = {{255, 0, 0, 255}, {0, 255, 0, 255}, {0, 0, 255, 255}};
        const dim sizes[] = {{10, 20}, {4, 4}, {6, 9}};
        const std::uint32_t ids[] = {7, 3, 11};
        for (std::size_t i = 0; i < 3; i++) {
            CAPTURE(i);
            const auto& r = a.region(i);
            CHECK(r.id == ids[i]);
            CHECK(r.dest.w == sizes[i].width);
            CHECK(r.dest.h == sizes[i].height);
            const auto& page = a.page(r.page);
            CHECK(pixel(page, r.dest.x, r.dest.y) == expected[i]);
            CHECK(pixel(page, r.dest.x + r.dest.w - 1, r.dest.y + r.dest.h - 1) == expected[i]);
        }
    }

    TEST_CASE("the gutter repeats the picture's edge pixels") {
        auto b = image_builder::rgba({3, 3});
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                b.set(x, y, color{static_cast <std::uint8_t>(x * 50), static_cast <std::uint8_t>(y * 50), 9, 255});
            }
        }
        image_bank bank;
        bank.entries.push_back({std::move(b).build(), 0});

        const auto a = build_atlas(bank);
        const auto& r = a.region(0);
        const auto& page = a.page(0);
        for (int m = 1; m <= gutter; m++) {
            CAPTURE(m);
            CHECK(pixel(page, r.dest.x - m, r.dest.y + 1) == pixel(page, r.dest.x, r.dest.y + 1));
            CHECK(pixel(page, r.dest.x + 2 + m, r.dest.y + 1) == pixel(page, r.dest.x + 2, r.dest.y + 1));
            CHECK(pixel(page, r.dest.x + 1, r.dest.y - m) == pixel(page, r.dest.x + 1, r.dest.y));
            CHECK(pixel(page, r.dest.x + 1, r.dest.y + 2 + m) == pixel(page, r.dest.x + 1, r.dest.y + 2));
            CHECK(pixel(page, r.dest.x - m, r.dest.y - m) == pixel(page, r.dest.x, r.dest.y));
        }
    }

    TEST_CASE("space outside every cell is transparent") {
        image_bank bank;
        bank.entries.push_back({solid_rgba({10, 20}, color{255, 255, 255, 255}), 0});
        bank.entries.push_back({solid_rgba({4, 4}, color{255, 255, 255, 255}), 0});
        bank.entries.push_back({solid_rgba({4, 4}, color{255, 255, 255, 255}), 0});

        const auto a = build_atlas(bank);
        const auto& page = a.page(0);
        int gaps = 0;
        for (int y = 0; y < static_cast <int>(page.height()); y++) {
            for (int x = 0; x < static_cast <int>(page.width()); x++) {
                if (!inside_any_cell(a, 0, x, y, gutter)) {
                    ++gaps;
                    CHECK(pixel(page, x, y).a == 0);
                }
            }
        }
        CHECK(gaps > 0);
    }

    TEST_CASE("partial alpha is copied, not blended") {
        image_bank bank;
        bank.entries.push_back({solid_rgba({2, 2}, color{200, 100, 50, 128}), 0});
        const auto a = build_atlas(bank);
        const auto& r = a.region(0);
        CHECK(pixel(a.page(0), r.dest.x, r.dest.y) == color{200, 100, 50, 128});
    }

    TEST_CASE("an indexed bank sharing a palette gives indexed pages by default") {
        const palette pal = ramp(8, 30);
        image_bank bank;
        bank.entries.push_back({pattern_indexed({5, 4}, pal, std::uint8_t{0}), 0});
        bank.entries.push_back({pattern_indexed({3, 6}, pal, std::uint8_t{0}), 1});

        const auto a = build_atlas(bank);
        REQUIRE(a.page_count() == 1);
        const auto& page = a.page(0);
        CHECK(page.format() == pixel_format::INDEX8);
        CHECK(page.color_key() == std::optional <std::uint32_t>{0u});
        CHECK(page.get_palette().get() == bank.entries[0].pixels.get_palette().get());

        for (std::size_t i = 0; i < 2; i++) {
            CAPTURE(i);
            const auto& r = a.region(i);
            CHECK(index_at(page, r.dest.x, r.dest.y) == 0); // the keyed pixel keeps the key
            for (int y = 0; y < r.dest.h; y++) {
                for (int x = (y == 0 ? 1 : 0); x < r.dest.w; x++) {
                    CHECK(index_at(page, r.dest.x + x, r.dest.y + y) == (x + y) % 4 + 1);
                }
            }
        }
    }

    TEST_CASE("an indexed bank packed as RGBA expands through the palette; the key becomes alpha 0") {
        const palette pal = ramp(8, 30);
        image_bank bank;
        bank.entries.push_back({pattern_indexed({4, 4}, pal, std::uint8_t{0}), 0});

        const auto a = build_atlas(bank, pixel_format::RGBA32);
        const auto& page = a.page(0);
        CHECK(page.format() == pixel_format::RGBA32);
        const auto& r = a.region(0);
        CHECK(pixel(page, r.dest.x, r.dest.y).a == 0);
        // pixel (1, 0) is index 2: ramp colour {60, 195, 60}
        CHECK(pixel(page, r.dest.x + 1, r.dest.y) == color{60, 195, 60, 255});
    }

    TEST_CASE("a colour key on an RGB or RGBA picture becomes alpha 0") {
        for (const auto format : {pixel_format::RGB24, pixel_format::RGBA32}) {
            CAPTURE(static_cast <int>(format));
            auto created = image::create_rgb(4, 4, format);
            REQUIRE(created.has_value());
            image img = std::move(*created);
            REQUIRE(img.fill(color{10, 20, 30, 255}).has_value());
            REQUIRE(img.put_pixel(0, 0, color{255, 0, 255, 255}).has_value());
            REQUIRE(img.set_color_key(color{255, 0, 255, 255}).has_value());

            image_bank bank;
            bank.entries.push_back({std::move(img), 0});
            const auto a = build_atlas(bank);
            const auto& r = a.region(0);
            const auto& page = a.page(0);
            CHECK(page.format() == pixel_format::RGBA32);
            CHECK(pixel(page, r.dest.x, r.dest.y).a == 0);                        // keyed pixel
            CHECK(pixel(page, r.dest.x - 1, r.dest.y - 1).a == 0);                // its extruded corner
            CHECK(pixel(page, r.dest.x + 1, r.dest.y) == color{10, 20, 30, 255}); // the rest is opaque
        }
    }

    TEST_CASE("indexed output needs one palette and one colour key") {
        image_bank different_palettes;
        different_palettes.entries.push_back({pattern_indexed({4, 4}, ramp(8, 30), {}), 0});
        different_palettes.entries.push_back({pattern_indexed({4, 4}, ramp(8, 31), {}), 1});
        CHECK_THROWS_AS((void)build_atlas(different_palettes, pixel_format::INDEX8), std::runtime_error);
        CHECK(build_atlas(different_palettes).page(0).format() == pixel_format::RGBA32);

        const palette pal = ramp(8, 30);
        image_bank different_keys;
        different_keys.entries.push_back({pattern_indexed({4, 4}, pal, std::uint8_t{0}), 0});
        different_keys.entries.push_back({pattern_indexed({4, 4}, pal, std::uint8_t{7}), 1});
        CHECK_THROWS_AS((void)build_atlas(different_keys, pixel_format::INDEX8), std::runtime_error);

        image_bank rgba;
        rgba.entries.push_back({solid_rgba({4, 4}, color{1, 2, 3, 255}), 0});
        CHECK_THROWS_AS((void)build_atlas(rgba, pixel_format::INDEX8), std::runtime_error);
    }

    TEST_CASE("an empty entry keeps its position with an empty region") {
        image_bank bank;
        bank.entries.push_back({solid_rgba({4, 4}, color{9, 9, 9, 255}), 1});
        bank.entries.push_back({image{}, 2});
        bank.entries.push_back({solid_rgba({4, 4}, color{9, 9, 9, 255}), 3});

        const auto a = build_atlas(bank);
        REQUIRE(a.region_count() == 3);
        CHECK(a.region(1).dest.w == 0);
        CHECK(a.region(1).dest.h == 0);
        CHECK(a.region(1).id == 2);
        CHECK(pixel(a.page(a.region(2).page), a.region(2).dest.x, a.region(2).dest.y) == color{9, 9, 9, 255});
    }

    TEST_CASE("pictures that do not share a page go on several pages") {
        image_bank bank;
        for (std::uint32_t i = 0; i < 4; i++) {
            bank.entries.push_back({solid_rgba({30, 30}, color{static_cast <std::uint8_t>(i * 60), 0, 0, 255}), i});
        }
        const auto a = build_atlas(bank, {}, dim{40, 40});
        REQUIRE(a.page_count() == 4);
        for (std::size_t i = 0; i < 4; i++) {
            CAPTURE(i);
            const auto& r = a.region(i);
            CHECK(r.page == i);
            CHECK(a.page(r.page).width() == 32);
            CHECK(pixel(a.page(r.page), r.dest.x, r.dest.y).r == i * 60);
        }
    }

    TEST_CASE("a picture larger than the page throws") {
        image_bank bank;
        bank.entries.push_back({solid_rgba({40, 10}, color{1, 1, 1, 255}), 0});
        CHECK_THROWS_AS((void)build_atlas(bank, {}, dim{40, 40}), std::runtime_error);
    }

    TEST_CASE("find_by_id returns the first position with that id") {
        image_bank bank;
        bank.entries.push_back({solid_rgba({2, 2}, color{1, 1, 1, 255}), 7});
        bank.entries.push_back({solid_rgba({2, 2}, color{1, 1, 1, 255}), 3});
        bank.entries.push_back({solid_rgba({2, 2}, color{1, 1, 1, 255}), 7});
        const auto a = build_atlas(bank);
        CHECK(a.find_by_id(7) == std::optional <std::size_t>{0});
        CHECK(a.find_by_id(3) == std::optional <std::size_t>{1});
        CHECK_FALSE(a.find_by_id(9).has_value());
        CHECK_FALSE(a.find_by_id(0).has_value());
        CHECK_FALSE(a.find_by_id(UINT32_MAX).has_value());
    }

    TEST_CASE("find_by_id agrees with a linear search over sparse, unordered ids") {
        image_bank bank;
        std::uint32_t id = 1000;
        for (int i = 0; i < 200; i++) {
            id = (id * 37 + 11) % 997; // scrambled, with repeats
            bank.entries.push_back({solid_rgba({1, 1}, color{1, 1, 1, 255}), id});
        }
        const auto a = build_atlas(bank);
        for (std::uint32_t probe = 0; probe < 1000; probe++) {
            std::optional <std::size_t> expected;
            for (std::size_t i = 0; i < bank.entries.size(); i++) {
                if (bank.entries[i].id == probe) {
                    expected = i;
                    break;
                }
            }
            CAPTURE(probe);
            CHECK(a.find_by_id(probe) == expected);
        }
    }

    TEST_CASE("the default page cap fits a 2048-pixel cell") {
        image_bank bank;
        bank.entries.push_back({solid_rgba({2048 - 2 * gutter, 1}, color{1, 1, 1, 255}), 0});
        const auto a = build_atlas(bank);
        REQUIRE(a.page_count() == 1);
        CHECK(a.page(0).width() == 2048);
    }

    TEST_CASE("out-of-range page and region throw") {
        image_bank bank;
        bank.entries.push_back({solid_rgba({2, 2}, color{1, 1, 1, 255}), 0});
        const auto a = build_atlas(bank);
        CHECK_THROWS_AS((void)a.page(1), std::runtime_error);
        CHECK_THROWS_AS((void)a.region(1), std::runtime_error);
    }
}
