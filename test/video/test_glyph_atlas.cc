//
// Tests for glyph_atlas_builder: pixels relative to the pen are cropped to their bounds and give
// bearing_x / y_offset; the picture per kind (alpha: white + coverage, rgba, indexed with palette
// and key); empty glyphs keep their position with an empty picture; entry ids are glyph ids;
// out-of-order calls and out-of-range glyphs throw; a font of only empty glyphs builds and
// registers.
//
#include <doctest/doctest.h>

#include <neutrino/video/text/glyph_atlas.hh>
#include <neutrino/video/sprite/texture_atlas.hh>

#include "image_atlas_access.hh"
#include "test_application.hh"
#include "services/service_access.hh"
#include "video/sprite/texture_registry.hh"
#include "video/text/glyph_atlas_access.hh"

#include <cstdint>
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
        return details::glyph_atlass_acces::atlas(g);
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
        const auto& p = g.properties(0);
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

        const auto& p = g.properties(0);
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
        CHECK(g.properties(1).glyph == ' ');
        CHECK(g.properties(1).advance == 3);
        CHECK(g.properties(1).bearing_x == 0);
        CHECK(g.properties(1).y_offset == 0);
        CHECK(g.properties(3).glyph == 'B');
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

        const auto& f = details::glyph_atlass_acces::font(g);
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
            CHECK_THROWS_AS((void)g.properties(1), std::runtime_error);
        }
    }

    TEST_CASE("a glyph further than an int16 from the pen throws") {
        alpha_builder b(font);
        b.start_glyph(1);
        b.put(40000, 0, 255);
        CHECK_THROWS_AS(b.end_glyph(), std::runtime_error);
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

TEST_SUITE("neutrino::video register_atlas glyph atlases") {
    TEST_CASE("a glyph atlas registers with one picture per glyph") {
        neutrino::test::test_application app("glyph atlas register");
        alpha_builder b(font);
        b.start_glyph('A');
        b.put(0, 0, 255);
        b.end_glyph();
        b.empty_glyph(' ');
        const atlas_id atlas = register_atlas(std::move(b).build());
        CHECK(require_texture_registry().picture_count(atlas) == 2);
        CHECK(require_texture_registry().picture_size(atlas, 1) == dim{0, 0});
        unregister_atlas(atlas);
    }

    TEST_CASE("a font of only empty glyphs registers") {
        neutrino::test::test_application app("glyph atlas register empty");
        alpha_builder b(font);
        b.empty_glyph(' ');
        const atlas_id atlas = register_atlas(std::move(b).build());
        CHECK(require_texture_registry().picture_count(atlas) == 1);
        unregister_atlas(atlas);
    }
}
