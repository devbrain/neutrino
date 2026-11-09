//
// Tests for glyph_atlas_builder: pixels relative to the pen are cropped to their bounds and give
// bearing_x / y_offset; the picture per kind (alpha: white + coverage, rgba, indexed with palette
// and key); empty glyphs keep their position with an empty picture; entry ids are glyph ids;
// out-of-order calls and out-of-range glyphs throw; a font of only empty glyphs builds;
// register_font / unregister_font.
//
#include <doctest/doctest.h>

#include <neutrino/video/text/glyph_atlas.hh>
#include <neutrino/video/text/font.hh>

#include "image_atlas_access.hh"
#include "test_application.hh"
#include "services/service_access.hh"
#include "video/sprite/texture_registry.hh"
#include "video/text/fonts_manager.hh"
#include "video/text/glyph_atlas_access.hh"

#include <chrono>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace neutrino;

namespace {
    const font_properties font{.line_height = 8, .baseline = 7};

    using alpha_builder = glyph_atlas_builder <glyph_atlas_format::alpha>;
    using rgba_builder = glyph_atlas_builder <glyph_atlas_format::rgba>;
    using indexed_builder = glyph_atlas_builder <glyph_atlas_format::indexed>;

    const image_atlas& atlas_of(const glyph_atlas& g) {
        return details::glyph_atlas_access::atlas(g);
    }

    // Pixel (x, y) of picture i, read from its atlas page.
    color picture_pixel(const glyph_atlas& g, std::size_t i, int x, int y) {
        const auto& a = atlas_of(g);
        const auto p = atlas_test::place(a, i);
        const auto px = atlas_test::page(a, p.page).get_pixel(p.dest.x + x, p.dest.y + y);
        REQUIRE(px.has_value());
        return *px;
    }

    std::uint8_t picture_index(const glyph_atlas& g, std::size_t i, int x, int y) {
        const auto& a = atlas_of(g);
        const auto p = atlas_test::place(a, i);
        const image& page = atlas_test::page(a, p.page);
        REQUIRE(page.format() == pixel_format::INDEX8);
        const auto* bytes = static_cast <const std::uint8_t*>(page.get_pixels());
        return bytes[static_cast <std::size_t>(p.dest.y + y) * page.pitch() + static_cast <std::size_t>(p.dest.x + x)];
    }

    palette four_colours() {
        const std::vector <std::uint8_t> rgb{0, 0, 0, 200, 0, 0, 0, 200, 0, 255, 0, 255};
        return load_palette(rgb, palette_encoding::rgb8);
    }
}

TEST_SUITE("neutrino::glyph_atlas_builder") {
    TEST_CASE("pixels are cropped to their bounds; bearing and y offset are measured from the pen") {
        alpha_builder b(font);
        b.start_glyph(65, glyph_properties{.advance = 5});
        b.put(1, -7, 255); // top of the cell (the baseline is 7 below it)
        b.put(3, -1, 255);
        b.put(2, -4, 255);
        b.end_glyph();
        const glyph_atlas g = std::move(b).build();

        REQUIRE(g.glyph_count() == 1);
        const auto& p = g.get_glyph_properties(0);
        CHECK(p.glyph == 65);
        CHECK(p.advance == 5);
        CHECK(p.bearing_x == 1);
        CHECK(p.y_offset == 0);
        CHECK(atlas_of(g).picture_size(0) == dim{3, 7});
    }

    TEST_CASE("ink left of the pen and below the baseline") {
        alpha_builder b(font);
        b.start_glyph(103, glyph_properties{.advance = 4});
        b.put(-2, 1, 255);
        b.put(0, 2, 255);
        b.end_glyph();
        const glyph_atlas g = std::move(b).build();

        const auto& p = g.get_glyph_properties(0);
        CHECK(p.bearing_x == -2);
        CHECK(p.y_offset == 8); // one row below the baseline, 7 + 1 from the cell top
        CHECK(atlas_of(g).picture_size(0) == dim{3, 2});
    }

    TEST_CASE("alpha glyphs are white with the coverage as alpha; unwritten pixels are transparent white") {
        alpha_builder b(font);
        b.start_glyph(1);
        b.put(0, 0, 255);
        b.put(2, 0, 128);
        b.end_glyph();
        const glyph_atlas g = std::move(b).build();

        CHECK(picture_pixel(g, 0, 0, 0) == color{255, 255, 255, 255});
        CHECK(picture_pixel(g, 0, 1, 0) == color{255, 255, 255, 0});
        CHECK(picture_pixel(g, 0, 2, 0) == color{255, 255, 255, 128});
    }

    TEST_CASE("put_span writes a run; every pixel counts, coverage 0 too") {
        alpha_builder b(font);
        b.start_glyph(1);
        const std::uint8_t run[] = {0, 255, 128, 0};
        b.put_span(-1, -3, run);
        b.put_span(5, 0, std::span <const std::uint8_t>{}); // empty: nothing, no bounds
        b.end_glyph();
        const glyph_atlas g = std::move(b).build();

        const auto& p = g.get_glyph_properties(0);
        CHECK(p.bearing_x == -1);
        CHECK(p.y_offset == 4);
        CHECK(atlas_of(g).picture_size(0) == dim{4, 1});
        CHECK(picture_pixel(g, 0, 0, 0) == color{255, 255, 255, 0});
        CHECK(picture_pixel(g, 0, 1, 0) == color{255, 255, 255, 255});
        CHECK(picture_pixel(g, 0, 2, 0) == color{255, 255, 255, 128});
        CHECK(picture_pixel(g, 0, 3, 0) == color{255, 255, 255, 0});
    }

    TEST_CASE("put_span and put mix in order: the last write wins") {
        rgba_builder b(font);
        b.start_glyph(1);
        const color run[] = {colors::red, colors::green, colors::blue};
        b.put(1, 0, colors::white);
        b.put_span(0, 0, run);
        b.put(2, 0, colors::yellow);
        b.end_glyph();
        const glyph_atlas g = std::move(b).build();

        CHECK(picture_pixel(g, 0, 0, 0) == colors::red);
        CHECK(picture_pixel(g, 0, 1, 0) == colors::green);
        CHECK(picture_pixel(g, 0, 2, 0) == colors::yellow);
    }

    TEST_CASE("put_mask puts the solid pixels; the others write nothing") {
        // .##....#
        // #.......
        bitmask mask(8, 2);
        for (const int x : {1, 2, 7}) {
            mask.set(x, 0, true);
        }
        mask.set(0, 1, true);

        alpha_builder masked(font);
        masked.start_glyph(1);
        masked.put(4, -2, 77);           // under a clear mask pixel: kept
        masked.put(5, -3, 99);           // under a solid one: replaced
        masked.put_mask(3, -3, mask, 200);
        masked.end_glyph();
        alpha_builder pixels(font);
        pixels.start_glyph(1);
        pixels.put(4, -2, 77);
        for (const int x : {4, 5, 10}) {
            pixels.put(x, -3, 200);
        }
        pixels.put(3, -2, 200);
        pixels.end_glyph();
        const glyph_atlas a = std::move(masked).build();
        const glyph_atlas b = std::move(pixels).build();

        CHECK(a.get_glyph_properties(0).bearing_x == 3);
        CHECK(a.get_glyph_properties(0).bearing_x == b.get_glyph_properties(0).bearing_x);
        CHECK(a.get_glyph_properties(0).y_offset == b.get_glyph_properties(0).y_offset);
        const dim size = atlas_of(a).picture_size(0);
        REQUIRE(size == atlas_of(b).picture_size(0));
        CHECK(size == dim{8, 2});
        for (int y = 0; y < size.height; y++) {
            for (int x = 0; x < size.width; x++) {
                CAPTURE(x);
                CAPTURE(y);
                CHECK(picture_pixel(a, 0, x, y) == picture_pixel(b, 0, x, y));
            }
        }
    }

    TEST_CASE("put_mask of a clear mask leaves the glyph empty") {
        alpha_builder b(font);
        b.start_glyph(' ', {.advance = 4});
        b.put_mask(0, 0, bitmask(8, 8), 255);
        b.put_mask(0, 0, bitmask(), 255);
        b.end_glyph();
        const glyph_atlas g = std::move(b).build();
        CHECK(atlas_of(g).picture_size(0) == dim{0, 0});
    }

    TEST_CASE("indexed spans and fills replay into the picture") {
        const palette pal = four_colours();
        indexed_builder b(font, pal, std::uint8_t{0});
        b.start_glyph(1);
        const std::uint8_t run[] = {1, 2, 3};
        b.put_span(0, 0, run);
        bitmask mask(2, 1);
        mask.set(0, 0, true);
        b.put_mask(1, 0, mask, 3); // replaces the span's middle pixel, leaves the last
        b.end_glyph();
        const glyph_atlas g = std::move(b).build();
        CHECK(picture_index(g, 0, 0, 0) == 1);
        CHECK(picture_index(g, 0, 1, 0) == 3);
        CHECK(picture_index(g, 0, 2, 0) == 3);
    }

    TEST_CASE("rgba glyphs keep their colours; unwritten pixels are transparent black") {
        rgba_builder b(font);
        b.start_glyph(1);
        b.put(0, 0, color{10, 20, 30, 255});
        b.put(2, 0, color{40, 50, 60, 70});
        b.end_glyph();
        const glyph_atlas g = std::move(b).build();

        CHECK(picture_pixel(g, 0, 0, 0) == color{10, 20, 30, 255});
        CHECK(picture_pixel(g, 0, 1, 0) == color{0, 0, 0, 0});
        CHECK(picture_pixel(g, 0, 2, 0) == color{40, 50, 60, 70});
    }

    TEST_CASE("indexed glyphs pack as INDEX8; unwritten pixels are the transparent index") {
        const palette pal = four_colours();
        indexed_builder b(font, pal, std::uint8_t{3});
        b.start_glyph(1);
        b.put(0, 0, 1);
        b.put(2, 1, 2);
        b.end_glyph();
        const glyph_atlas g = std::move(b).build();

        REQUIRE(atlas_test::page_count(atlas_of(g)) == 1);
        CHECK(picture_index(g, 0, 0, 0) == 1);
        CHECK(picture_index(g, 0, 1, 0) == 3);
        CHECK(picture_index(g, 0, 0, 1) == 3);
        CHECK(picture_index(g, 0, 2, 1) == 2);
    }

    TEST_CASE("an empty glyph keeps its position with an empty picture") {
        alpha_builder b(font);
        b.start_glyph('A', glyph_properties{.advance = 5});
        b.put(0, -1, 255);
        b.end_glyph();
        b.empty_glyph(' ', glyph_properties{.advance = 3});
        b.start_glyph('!', glyph_properties{.advance = 2}); // started and ended without pixels
        b.end_glyph();
        b.start_glyph('B', glyph_properties{.advance = 5});
        b.put(0, -1, 255);
        b.put(1, -1, 255);
        b.end_glyph();
        const glyph_atlas g = std::move(b).build();

        REQUIRE(g.glyph_count() == 4);
        const auto& a = atlas_of(g);
        REQUIRE(a.picture_count() == 4);
        CHECK(a.picture_size(0) == dim{1, 1});
        CHECK(a.picture_size(1) == dim{0, 0});
        CHECK(a.picture_size(2) == dim{0, 0});
        CHECK(a.picture_size(3) == dim{2, 1});
        CHECK(g.get_glyph_properties(1).glyph == ' ');
        CHECK(g.get_glyph_properties(1).advance == 3);
        CHECK(g.get_glyph_properties(1).bearing_x == 0);
        CHECK(g.get_glyph_properties(1).y_offset == 0);
        CHECK(g.get_glyph_properties(3).glyph == 'B');
    }

    TEST_CASE("bank entry ids are the glyph ids") {
        alpha_builder b(font);
        b.start_glyph(200);
        b.put(0, 0, 255);
        b.end_glyph();
        b.empty_glyph(32);
        b.start_glyph(66);
        b.put(0, 0, 255);
        b.end_glyph();
        const glyph_atlas g = std::move(b).build();

        const auto& a = atlas_of(g);
        CHECK(a.find_by_id(200).value().picture_idx == 0);
        CHECK(a.find_by_id(32).value().picture_idx == 1);
        CHECK(a.find_by_id(66).value().picture_idx == 2);
        CHECK_FALSE(a.find_by_id(0).has_value());
    }

    TEST_CASE("the font properties are kept as given") {
        alpha_builder b(font_properties{.line_height = 10, .baseline = 8, .space_advance = 4,
                                        .letter_spacing = 1, .default_glyph = 63});
        b.empty_glyph(32);
        const glyph_atlas g = std::move(b).build();

        const auto& f = details::glyph_atlas_access::font(g);
        CHECK(f.line_height == 10);
        CHECK(f.baseline == 8);
        CHECK(f.space_advance == 4);
        CHECK(f.letter_spacing == 1);
        CHECK(f.default_glyph == 63);
    }

    TEST_CASE("glyph calls out of order throw") {
        SUBCASE("end_glyph without start_glyph") {
            alpha_builder b(font);
            CHECK_THROWS_AS(b.end_glyph(), std::runtime_error);
        }
        SUBCASE("start_glyph twice") {
            alpha_builder b(font);
            b.start_glyph(1);
            CHECK_THROWS_AS(b.start_glyph(2), std::runtime_error);
        }
        SUBCASE("build with a glyph not ended") {
            alpha_builder b(font);
            b.start_glyph(1);
            CHECK_THROWS_AS((void)std::move(b).build(), std::runtime_error);
        }
        SUBCASE("properties out of range") {
            alpha_builder b(font);
            b.empty_glyph(1);
            const glyph_atlas g = std::move(b).build();
            CHECK_THROWS_AS((void)g.get_glyph_properties(1), std::runtime_error);
        }
    }

    TEST_CASE("a glyph further than an int16 from the pen throws") {
        alpha_builder b(font);
        b.start_glyph(1);
        b.put(40000, 0, 255);
        CHECK_THROWS_AS(b.end_glyph(), std::runtime_error);
    }

    TEST_CASE("advances are kept to the nearest 1/64 pixel; a non-finite advance throws") {
        alpha_builder b(font);
        b.empty_glyph('a', {.advance = 6.4f});
        CHECK_THROWS_AS(b.start_glyph('b', {.advance = std::numeric_limits <float>::quiet_NaN()}),
                        std::runtime_error);
        const glyph_atlas g = std::move(b).build();
        CHECK(g.get_glyph_properties(0).advance == 410.0f / 64.0f);
    }

    TEST_CASE("a glyph id used twice throws at build") {
        alpha_builder b(font);
        b.empty_glyph('a');
        b.empty_glyph('a');
        CHECK_THROWS_AS((void)std::move(b).build(), std::runtime_error);
    }

    TEST_CASE("a font of only empty glyphs builds, also as INDEX8") {
        const palette pal = four_colours();
        indexed_builder b(font, pal, std::uint8_t{3});
        b.empty_glyph(32);
        b.empty_glyph(160);
        const glyph_atlas g = std::move(b).build(pixel_format::INDEX8);

        CHECK(g.glyph_count() == 2);
        CHECK(atlas_test::page_count(atlas_of(g)) == 0);
        CHECK(atlas_of(g).picture_size(1) == dim{0, 0});
    }
}

TEST_SUITE("neutrino::video register_font") {
    TEST_CASE("a font registers its atlas with one picture per glyph") {
        neutrino::test::test_application app("glyph atlas register");
        alpha_builder b(font);
        b.start_glyph('A');
        b.put(0, 0, 255);
        b.end_glyph();
        b.empty_glyph(' ', {.advance = 3});
        const font_id f = register_font(std::move(b).build());
        REQUIRE(f.valid());

        const auto& loaded = require_fonts_manager().get(f);
        CHECK(require_texture_registry().picture_count(loaded.atlas) == 2);
        CHECK(require_texture_registry().picture_size(loaded.atlas, 1) == dim{0, 0});
        CHECK(loaded.font_props.line_height == 8);
        CHECK(loaded.font_props.baseline == 7);
        REQUIRE(loaded.glyph_props.size() == 2);
        CHECK(loaded.glyph_props[0].glyph == 'A');
        CHECK(loaded.glyph_props[1].glyph == ' ');
        CHECK(loaded.glyph_props[1].advance == 3);

        const atlas_id atlas = loaded.atlas;
        unregister_font(f);
        CHECK_THROWS_AS((void)require_fonts_manager().get(f), std::runtime_error);
        CHECK_THROWS_AS((void)require_texture_registry().picture_count(atlas), std::runtime_error);
    }

    TEST_CASE("a font of only empty glyphs registers") {
        neutrino::test::test_application app("glyph atlas register empty");
        alpha_builder b(font);
        b.empty_glyph(' ');
        const font_id f = register_font(std::move(b).build());
        CHECK(require_texture_registry().picture_count(require_fonts_manager().get(f).atlas) == 1);
        unregister_font(f);
    }

    TEST_CASE("unregister_font ignores invalid and already unregistered ids") {
        neutrino::test::test_application app("glyph atlas unregister twice");
        alpha_builder b(font);
        b.empty_glyph(' ');
        const font_id f = register_font(std::move(b).build());
        unregister_font(f);
        CHECK_NOTHROW(unregister_font(f));
        CHECK_NOTHROW(unregister_font(font_id{}));
    }
}

// Build cost of the glyph builder (font_plan.md FM1 item 2): timings go to the test log.
TEST_SUITE("neutrino::glyph_atlas_builder cost") {
    TEST_CASE("synthetic 20 000-glyph font, 16x16, one put per pixel") {
        constexpr int glyphs = 20000;
        const auto t0 = std::chrono::steady_clock::now();
        alpha_builder b(font);
        for (int g = 0; g < glyphs; g++) {
            b.start_glyph(static_cast <std::uint32_t>(g), {.advance = 16});
            for (int y = 0; y < 16; y++) {
                for (int x = 0; x < 16; x++) {
                    if (((x * 7 + y * 3 + g) & 3) != 0) {
                        b.put(x, y - 12, 0xFF);
                    }
                }
            }
            b.end_glyph();
        }
        const auto t1 = std::chrono::steady_clock::now();
        const auto atlas = std::move(b).build();
        const auto t2 = std::chrono::steady_clock::now();
        CHECK(atlas.glyph_count() == glyphs);
        MESSAGE("20000 glyphs per pixel: glyphs ", std::chrono::duration <double, std::milli>(t1 - t0).count(),
                " ms, pack ", std::chrono::duration <double, std::milli>(t2 - t1).count(), " ms");
    }

    TEST_CASE("synthetic 20 000-glyph font, 16x16, one put_mask per glyph") {
        constexpr int glyphs = 20000;
        // The same pixels as the per-pixel case; the pattern repeats every 4 glyphs.
        std::vector <bitmask> masks;
        for (int v = 0; v < 4; v++) {
            bitmask& m = masks.emplace_back(16, 16);
            for (int y = 0; y < 16; y++) {
                for (int x = 0; x < 16; x++) {
                    m.set(x, y, ((x * 7 + y * 3 + v) & 3) != 0);
                }
            }
        }
        const auto t0 = std::chrono::steady_clock::now();
        alpha_builder b(font);
        for (int g = 0; g < glyphs; g++) {
            b.start_glyph(static_cast <std::uint32_t>(g), {.advance = 16});
            b.put_mask(0, -12, masks[static_cast <std::size_t>(g & 3)], 0xFF);
            b.end_glyph();
        }
        const auto t1 = std::chrono::steady_clock::now();
        const auto atlas = std::move(b).build();
        const auto t2 = std::chrono::steady_clock::now();
        CHECK(atlas.glyph_count() == glyphs);
        MESSAGE("20000 glyphs put_mask: glyphs ", std::chrono::duration <double, std::milli>(t1 - t0).count(),
                " ms, pack ", std::chrono::duration <double, std::milli>(t2 - t1).count(), " ms");
    }

    TEST_CASE("synthetic 20 000-glyph font, 16x16, one put_span per row") {
        constexpr int glyphs = 20000;
        std::uint8_t rows[4][16][16]{};
        for (int v = 0; v < 4; v++) {
            for (int y = 0; y < 16; y++) {
                for (int x = 0; x < 16; x++) {
                    rows[v][y][x] = ((x * 7 + y * 3 + v) & 3) != 0 ? 0xFF : 0;
                }
            }
        }
        const auto t0 = std::chrono::steady_clock::now();
        alpha_builder b(font);
        for (int g = 0; g < glyphs; g++) {
            b.start_glyph(static_cast <std::uint32_t>(g), {.advance = 16});
            for (int y = 0; y < 16; y++) {
                b.put_span(0, y - 12, rows[g & 3][y]);
            }
            b.end_glyph();
        }
        const auto t1 = std::chrono::steady_clock::now();
        const auto atlas = std::move(b).build();
        const auto t2 = std::chrono::steady_clock::now();
        CHECK(atlas.glyph_count() == glyphs);
        MESSAGE("20000 glyphs put_span: glyphs ", std::chrono::duration <double, std::milli>(t1 - t0).count(),
                " ms, pack ", std::chrono::duration <double, std::milli>(t2 - t1).count(), " ms");
    }
}
