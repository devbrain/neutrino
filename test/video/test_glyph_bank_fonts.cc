//
// FM2 (font_plan.md): game fonts through the general path. Each synthetic format below is a
// small decoder that writes its glyphs into a glyph_atlas_builder in its native pixel kind
// and states a glyph_mapping; the font is registered with register_font(atlas, mapping),
// then laid out and its pictures checked.
//
//   - Westwood-v3-style: 4-bit indexed, per-glyph Y offset and height, glyph data shared
//     between codes;
//   - Jazz-style: glyphs 0-25 = A-Z, unsupported code points draw glyph 126, no space glyph;
//   - RA2-style: many code points -> one glyph, implicit +1 px spacing;
//   - brix-style: RGBA, glyph id = code point - ' ', lower case folds to upper case.
//
#include <doctest/doctest.h>

#include <neutrino/video/text/font.hh>
#include <neutrino/video/text/glyph_mapping.hh>
#include <neutrino/video/text/text_layout.hh>

#include "image_atlas_access.hh"
#include "test_application.hh"
#include "services/service_access.hh"
#include "video/sprite/texture_registry.hh"
#include "video/text/fonts_manager.hh"
#include "video/text/glyph_atlas_access.hh"
#include "video/text/text_layout_engine.hh"

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace neutrino;

namespace {
    const image_atlas& atlas_of(const glyph_atlas& g) {
        return details::glyph_atlas_access::atlas(g);
    }

    const glyph_properties& props_of(const glyph_atlas& g, std::uint32_t id) {
        const auto& all = details::glyph_atlas_access::glyphs(g);
        for (const auto& p : all) {
            if (p.glyph == id) {
                return p;
            }
        }
        FAIL("no glyph " << id);
        return all.front();
    }

    std::size_t index_of(const glyph_atlas& g, std::uint32_t id) {
        const auto& all = details::glyph_atlas_access::glyphs(g);
        for (std::size_t i = 0; i < all.size(); i++) {
            if (all[i].glyph == id) {
                return i;
            }
        }
        FAIL("no glyph " << id);
        return 0;
    }

    dim picture_size(const glyph_atlas& g, std::uint32_t id) {
        const auto p = atlas_test::place(atlas_of(g), index_of(g, id));
        return {p.dest.w, p.dest.h};
    }

    std::uint8_t picture_index(const glyph_atlas& g, std::uint32_t id, int x, int y) {
        const auto& a = atlas_of(g);
        const auto p = atlas_test::place(a, index_of(g, id));
        const image& page = atlas_test::page(a, p.page);
        REQUIRE(page.format() == pixel_format::INDEX8);
        const auto* bytes = static_cast <const std::uint8_t*>(page.get_pixels());
        return bytes[static_cast <std::size_t>(p.dest.y + y) * page.pitch() + static_cast <std::size_t>(p.dest.x + x)];
    }

    color picture_pixel(const glyph_atlas& g, std::uint32_t id, int x, int y) {
        const auto& a = atlas_of(g);
        const auto p = atlas_test::place(a, index_of(g, id));
        const auto px = atlas_test::page(a, p.page).get_pixel(p.dest.x + x, p.dest.y + y);
        REQUIRE(px.has_value());
        return *px;
    }

    const loaded_font& loaded(font_id id) {
        return require_fonts_manager().get(id);
    }

    // ---------------------------------------------------------------------------------------
    // Westwood-v3-style: 4-bit indexed pixels, two per byte (high nibble first), rows padded
    // to whole bytes; per code: data offset, width, height, Y offset from the cell top.
    // Codes 'A' and 'a' share their data. Colour 0 is transparent.
    // ---------------------------------------------------------------------------------------
    struct ww_entry {
        std::uint8_t code;
        std::uint16_t offset;
        std::uint8_t width;
        std::uint8_t height;
        std::uint8_t y_offset;
    };

    struct ww_font {
        std::uint8_t cell_height = 8;
        std::uint8_t baseline = 7;
        std::vector <ww_entry> entries;
        std::vector <std::uint8_t> data;
    };

    ww_font westwood_font() {
        ww_font f;
        // Shape 0 (3x3 at offset 0): a ring of colour 1 around colour 2.
        //   1 1 1 / 1 2 1 / 1 1 1  -> rows: 0x11 0x10 | 0x12 0x10 | 0x11 0x10
        // Shape 1 (2x2 at offset 6): 3 0 / 0 3 (a diagonal; 0 is transparent)
        //   rows: 0x30 | 0x03
        f.data = {0x11, 0x10, 0x12, 0x10, 0x11, 0x10, 0x30, 0x03};
        f.entries = {
            {'A', 0, 3, 3, 2},  // capital: starts 2 below the cell top
            {'a', 0, 3, 3, 4},  // same picture, lower
            {'.', 6, 2, 2, 5},  // the diagonal, near the baseline
        };
        return f;
    }

    palette sixteen_colours() {
        std::vector <std::uint8_t> rgb;
        for (int i = 0; i < 16; i++) {
            rgb.insert(rgb.end(), {static_cast <std::uint8_t>(i * 16), 0, static_cast <std::uint8_t>(255 - i * 16)});
        }
        return load_palette(rgb, palette_encoding::rgb8);
    }

    struct decoded {
        glyph_atlas glyphs;
        glyph_mapping mapping;
    };

    decoded decode_westwood(const ww_font& f, const palette& pal) {
        glyph_atlas_builder <glyph_atlas_format::indexed> b(
            {.line_height = f.cell_height, .baseline = f.baseline, .space_advance = 3, .default_glyph = '.'},
            pal, std::uint8_t{0});
        glyph_mapping mapping;
        for (const auto& e : f.entries) {
            b.start_glyph(e.code, {.advance = static_cast <float>(e.width + 1)});
            const int row_bytes = (e.width + 1) / 2;
            for (int y = 0; y < e.height; y++) {
                for (int x = 0; x < e.width; x++) {
                    const std::uint8_t byte = f.data[e.offset + y * row_bytes + x / 2];
                    const auto index = static_cast <std::uint8_t>(x % 2 == 0 ? byte >> 4 : byte & 0x0F);
                    // Cell rows run from the top; the pen is `baseline` below it.
                    b.put(x, e.y_offset + y - f.baseline, index);
                }
            }
            b.end_glyph();
            mapping.bind(e.code, e.code);
        }
        return {std::move(b).build(), std::move(mapping)};
    }

    // ---------------------------------------------------------------------------------------
    // Jazz-style: glyphs 0-25 are A-Z (8 px wide boxes, each a different height), glyph 126
    // is the "unsupported" block; no space glyph.
    // ---------------------------------------------------------------------------------------
    decoded decode_jazz() {
        glyph_atlas_builder <glyph_atlas_format::alpha> b(
            {.line_height = 12, .baseline = 10, .space_advance = 6, .default_glyph = 126});
        for (std::uint32_t g = 0; g < 26; g++) {
            b.start_glyph(g, {.advance = 8});
            b.put(0, -1 - static_cast <int>(g % 8), 255);
            b.put(6, -1, 255);
            b.end_glyph();
        }
        b.start_glyph(126, {.advance = 7});
        for (int y = -8; y < 0; y++) {
            b.put(0, y, 255);
            b.put(5, y, 255);
        }
        b.end_glyph();
        return {std::move(b).build(), glyph_mapping{{U'A', U'Z' + 1, 0}}};
    }

    // ---------------------------------------------------------------------------------------
    // RA2-style: one glyph serves the Latin, Greek and Cyrillic capital A (and B); every glyph
    // is followed by an implicit 1 px gap.
    // ---------------------------------------------------------------------------------------
    decoded decode_ra2() {
        glyph_atlas_builder <glyph_atlas_format::alpha> b(
            {.line_height = 10, .baseline = 8, .space_advance = 4, .letter_spacing = 1, .default_glyph = 0});
        for (std::uint32_t g = 0; g < 3; g++) { // 0: '?', 1: A, 2: B
            b.start_glyph(g, {.advance = 6});
            for (int x = 0; x < 5; x++) {
                b.put(x, -1 - static_cast <int>(g), 255);
            }
            b.end_glyph();
        }
        glyph_mapping m;
        m.bind(U'?', 0);
        m.bind(U'A', U'C', 1);          // Latin A, B
        m.bind(U'Α', U'Γ', 1); // Greek Alpha, Beta
        m.bind(U'А', 1);            // Cyrillic A
        m.bind(U'В', 2);            // Cyrillic Ve looks like B
        return {std::move(b).build(), std::move(m)};
    }

    // ---------------------------------------------------------------------------------------
    // brix-style: RGBA, glyph id = code point - ' ' for ' '..'_', lower case drawn as upper
    // case. Glyph 0 (space) is blank with its own advance.
    // ---------------------------------------------------------------------------------------
    constexpr color brix_red{200, 30, 30, 255};
    constexpr color brix_shade{100, 15, 15, 128};

    decoded decode_brix() {
        glyph_atlas_builder <glyph_atlas_format::rgba> b({.line_height = 9, .baseline = 8, .space_advance = 99});
        for (char32_t cp = U' '; cp <= U'_'; cp++) {
            const auto id = static_cast <std::uint32_t>(cp - U' ');
            b.start_glyph(id, {.advance = cp == U' ' ? 5.0f : 7.0f});
            if (cp != U' ') {
                b.put(0, -2, brix_red);
                b.put(1, -1, brix_shade);
            }
            b.end_glyph();
        }
        return {std::move(b).build(), glyph_mapping{{U' ', U'_' + 1, 0}, {U'a', U'z' + 1, U'A' - U' '}}};
    }
}

TEST_SUITE("neutrino::font glyph-bank fonts (FM2)") {
    TEST_CASE("Westwood-v3-style: 4-bit indexed, per-glyph Y offset and height, shared glyph data") {
        const auto pal = sixteen_colours();
        auto [glyphs, mapping] = decode_westwood(westwood_font(), pal);

        // Rasterised: nibbles land as palette indices, 0 stays transparent (the key).
        CHECK(picture_size(glyphs, 'A') == dim{3, 3});
        CHECK(picture_index(glyphs, 'A', 0, 0) == 1);
        CHECK(picture_index(glyphs, 'A', 1, 1) == 2);
        CHECK(picture_size(glyphs, '.') == dim{2, 2});
        CHECK(picture_index(glyphs, '.', 0, 0) == 3);
        CHECK(picture_index(glyphs, '.', 1, 0) == 0);
        CHECK(picture_index(glyphs, '.', 1, 1) == 3);
        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                CHECK(picture_index(glyphs, 'a', x, y) == picture_index(glyphs, 'A', x, y));
            }
        }
        // The per-glyph Y offset is the picture's offset from the cell top.
        CHECK(props_of(glyphs, 'A').y_offset == 2);
        CHECK(props_of(glyphs, 'a').y_offset == 4);
        CHECK(props_of(glyphs, '.').y_offset == 5);

        neutrino::test::test_application app("westwood font");
        const font_id id = register_font(std::move(glyphs), mapping, atlas_texture_format::automatic,
                                         texture_filter::nearest);
        CHECK(require_texture_registry().picture_count(loaded(id).atlas) == 3);
        CHECK(measure("Aa.", id) == dim{11, 8});
        CHECK(measure("A Z", id) == dim{4 + 3 + 3, 8}); // no space glyph; 'Z' draws '.'
        CHECK(get_glyph_properties(id, U'Z')->glyph == '.');
        CHECK_FALSE(has_glyph(id, U'Z'));

        // Laid out, pictures sit at caret + bearing, line top + Y offset.
        const auto l = details::layout_text("Aa.\nA", loaded(id), {}, {});
        REQUIRE(l.lines.size() == 2);
        CHECK(l.lines[0].chars[0].picture == point{0, 2});
        CHECK(l.lines[0].chars[1].picture == point{4, 4});
        CHECK(l.lines[0].chars[2].picture == point{8, 5});
        CHECK(l.lines[1].chars[0].picture == point{0, 10});
        unregister_font(id);
    }

    TEST_CASE("Jazz-style: 0-25 = A-Z, unsupported -> 126, no space glyph") {
        auto [glyphs, mapping] = decode_jazz();
        CHECK(picture_size(glyphs, 0) == dim{7, 1});
        CHECK(picture_size(glyphs, 7) == dim{7, 8});
        CHECK(picture_size(glyphs, 126) == dim{6, 8});
        CHECK(picture_pixel(glyphs, 126, 0, 0) == color{255, 255, 255, 255});
        CHECK(picture_pixel(glyphs, 126, 1, 0) == color{255, 255, 255, 0});

        neutrino::test::test_application app("jazz font");
        const font_id id = register_font(std::move(glyphs), mapping);
        CHECK(get_glyph_properties(id, U'A')->glyph == 0);
        CHECK(get_glyph_properties(id, U'Z')->glyph == 25);
        CHECK(get_glyph_properties(id, U'a')->glyph == 126);
        CHECK(get_glyph_properties(id, U'!')->glyph == 126);
        CHECK(has_glyph(id, U'Q'));
        CHECK_FALSE(has_glyph(id, U'!'));
        CHECK_FALSE(has_glyph(id, U' '));
        // A B space(6) C ! -> 8 + 8 + 6 + 8 + 7
        CHECK(measure("AB C!", id) == dim{37, 12});
        CHECK(wrap("AB CD", id, 20) == std::vector <line_range>{{0, 2, 16}, {3, 5, 16}});
        unregister_font(id);
    }

    TEST_CASE("RA2-style: many code points -> one glyph, implicit +1 px spacing") {
        auto [glyphs, mapping] = decode_ra2();
        neutrino::test::test_application app("ra2 font");
        const font_id id = register_font(std::move(glyphs), mapping);
        for (const char32_t a : {U'A', U'Α', U'А'}) {
            CHECK(get_glyph_properties(id, a)->glyph == 1);
        }
        for (const char32_t bee : {U'B', U'Β', U'В'}) {
            CHECK(get_glyph_properties(id, bee)->glyph == 2);
        }
        CHECK(has_glyph(id, U'А'));
        CHECK_FALSE(has_glyph(id, U'Б')); // Cyrillic Be: not in the font
        CHECK(get_glyph_properties(id, U'Б')->glyph == 0);
        // 3 glyphs of 6 px with a 1 px gap between: "AΑА" (UTF-8)
        CHECK(measure("A\xCE\x91\xD0\x90", id) == dim{20, 10});
        CHECK(offset_of("AB", id, 1) == point{7, 0});
        // Lookup ranges: A-B and Greek Alpha-Beta are single ranges; '?' and the Cyrillic two stand alone.
        CHECK(loaded(id).lookup.ranges().size() == 5);
        unregister_font(id);
    }

    TEST_CASE("brix-style: RGBA, ' ' offset, case folding, own space glyph") {
        auto [glyphs, mapping] = decode_brix();
        CHECK(glyphs.glyph_count() == 64);
        CHECK(picture_size(glyphs, 0) == dim{0, 0}); // blank space
        CHECK(picture_pixel(glyphs, 'A' - ' ', 0, 0) == brix_red);
        CHECK(picture_pixel(glyphs, 'A' - ' ', 1, 1) == brix_shade);
        CHECK(picture_pixel(glyphs, 'A' - ' ', 1, 0) == color{0, 0, 0, 0});
        CHECK(props_of(glyphs, 'A' - ' ').y_offset == 6);

        neutrino::test::test_application app("brix font");
        const font_id id = register_font(std::move(glyphs), mapping, atlas_texture_format::rgba,
                                         texture_filter::nearest);
        CHECK(get_glyph_properties(id, U'a')->glyph == 'A' - ' ');
        CHECK(get_glyph_properties(id, U'z')->glyph == 'Z' - ' ');
        CHECK(get_glyph_properties(id, U'_')->glyph == '_' - ' ');
        CHECK(has_glyph(id, U'q'));
        CHECK_FALSE(has_glyph(id, U'{'));
        CHECK_FALSE(has_glyph(id, U'`'));
        // The space glyph's own advance (5), not space_advance (99).
        CHECK(measure("ab A", id) == dim{7 + 7 + 5 + 7, 9});
        unregister_font(id);
    }

    TEST_CASE("a mapping that names a missing glyph throws") {
        auto [glyphs, mapping] = decode_jazz();
        neutrino::test::test_application app("bad mapping");
        CHECK_THROWS_AS((void)register_font(std::move(glyphs), glyph_mapping{{U'A', U'Z' + 2, 0}}),
                        std::runtime_error); // '[' -> glyph 26
    }
}
