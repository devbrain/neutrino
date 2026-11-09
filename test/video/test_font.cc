//
// Tests for fonts: the code point lookup (own glyph, default glyph, none); onyx bitmap fonts
// drawn into glyph atlases (metrics, ABC spacing, charset mapping incl. CP437's control-byte
// pictures); bitmap font decoders (built-in detection of FON files, custom decoders, failures);
// BIOS fonts.
//
#include <doctest/doctest.h>

#include <neutrino/video/text/font.hh>

#include "image_atlas_access.hh"
#include "test_application.hh"
#include "services/service_access.hh"
#include "video/sprite/texture_registry.hh"
#include "video/text/fonts_manager.hh"
#include "video/text/glyph_atlas_access.hh"
#include "video/text/onyx_bitmap_font.hh"

#include <onyx_font/bios_font.hh>

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

using namespace neutrino;

namespace {
    std::vector <std::uint8_t> read_font_file(std::string_view name) {
        const auto path = std::filesystem::path(NEUTRINO_FONT_TEST_DATA_DIR) / name;
        std::ifstream in(path, std::ios::binary);
        REQUIRE(in.good());
        return {std::istreambuf_iterator <char>(in), std::istreambuf_iterator <char>()};
    }

    // 'A': 3x4 block, A = -1, B = 3, C = 2 (advance 4, ink starts left of the pen).
    // 'B': 2x4, only the bottom row lit, no ABC (advance = width 2).
    // 'C': 2x4, blank (the break char).
    // Ascent 3, pixel height 4, external leading 1; default char 'B'.
    onyx_font::bitmap_font synthetic_font() {
        onyx_font::bitmap_builder builder;
        (void)builder.add_glyph_from_fn(3, 4, [](auto, auto) { return true; });
        (void)builder.add_glyph_from_fn(2, 4, [](auto, auto y) { return y == 3; });
        (void)builder.add_glyph_from_fn(2, 4, [](auto, auto) { return false; });
        auto storage = std::move(builder).build();
        std::vector <onyx_font::glyph_spacing> spacing(3);
        spacing[0] = {std::int16_t{-1}, std::uint16_t{3}, std::int16_t{2}};
        onyx_font::font_metrics m;
        m.ascent = 3;
        m.pixel_height = 4;
        m.external_leading = 1;
        m.avg_width = 2;
        return {"synthetic", 'A', 'C', 'B', 'C', m, std::move(spacing), std::move(storage)};
    }

    constexpr std::array <std::uint8_t, 4> synthetic_magic{'N', 'T', 'F', '0'};

    class synthetic_decoder final : public bitmap_font_decoder {
        public:
            [[nodiscard]] std::string_view name() const noexcept override { return "synthetic"; }

            [[nodiscard]] std::span <const std::string_view> extensions() const noexcept override {
                static constexpr std::array <std::string_view, 1> ext{".ntf"};
                return ext;
            }

            [[nodiscard]] bool sniff(std::span <const std::uint8_t> data) const noexcept override {
                return data.size() >= synthetic_magic.size() &&
                       std::equal(synthetic_magic.begin(), synthetic_magic.end(), data.begin());
            }

            [[nodiscard]] std::vector <onyx_font::font_entry> enumerate(std::span <const std::uint8_t>) const override {
                return {};
            }

            [[nodiscard]] onyx_font::bitmap_font load(std::span <const std::uint8_t>, std::size_t index) const override {
                if (index != 0) {
                    throw std::invalid_argument("only face 0");
                }
                return synthetic_font();
            }
    };

    const glyph_properties& glyph_of(const loaded_font& f, char32_t cp) {
        const auto idx = f.find_glyph(cp);
        REQUIRE(idx.has_value());
        return f.glyph_props[*idx];
    }
}

TEST_SUITE("neutrino::font lookup") {
    std::vector <glyph_properties> three_glyphs() {
        return {{.glyph = 'a'}, {.glyph = 'b'}, {.glyph = '?'}};
    }

    TEST_CASE("glyph ids as code points") {
        const auto lookup = glyph_ids_as_code_points(three_glyphs());
        CHECK(lookup.size() == 3);
        CHECK(lookup.at(U'a') == 0);
        CHECK(lookup.at(U'?') == 2);
    }

    TEST_CASE("a code point finds its glyph, else the default glyph") {
        const loaded_font f({}, {.default_glyph = '?'}, three_glyphs(), glyph_ids_as_code_points(three_glyphs()));
        CHECK(f.find_glyph(U'b') == 1);
        CHECK(f.find_glyph(U'z') == 2);
        CHECK(f.default_index == 2);
    }

    TEST_CASE("no glyph and no default glyph finds nothing") {
        const loaded_font f({}, {.default_glyph = '#'}, three_glyphs(), glyph_ids_as_code_points(three_glyphs()));
        CHECK_FALSE(f.default_index.has_value());
        CHECK_FALSE(f.find_glyph(U'z').has_value());
    }

    TEST_CASE("a lookup naming a missing glyph index throws") {
        CHECK_THROWS_AS(loaded_font({}, {}, three_glyphs(), glyph_lookup{{U'x', 3}}), std::runtime_error);
    }
}

TEST_SUITE("neutrino::font onyx bitmap fonts") {
    TEST_CASE("font properties come from the onyx metrics") {
        const auto c = convert_bitmap_font(synthetic_font());
        const auto f = c.glyphs.get_font_properties();
        CHECK(f.baseline == 3);
        CHECK(f.line_height == 5);
        CHECK(f.space_advance == 2); // the break char 'C'
        CHECK(f.default_glyph == 'B');
        CHECK(c.glyphs.glyph_count() == 3);
    }

    TEST_CASE("ABC spacing gives the advance; ink position gives bearing and offset") {
        const auto c = convert_bitmap_font(synthetic_font());
        const auto& atlas = details::glyph_atlas_access::atlas(c.glyphs);

        const auto& a = c.glyphs.get_glyph_properties(0);
        CHECK(a.glyph == 'A');
        CHECK(a.advance == 4);   // -1 + 3 + 2
        CHECK(a.bearing_x == -1);
        CHECK(a.y_offset == 0);  // ink from the cell top
        CHECK(atlas.picture_size(0) == dim{3, 4});

        const auto& b = c.glyphs.get_glyph_properties(1);
        CHECK(b.advance == 2);   // no ABC: the bitmap width
        CHECK(b.bearing_x == 0);
        CHECK(b.y_offset == 3);  // only the bottom row, at the baseline
        CHECK(atlas.picture_size(1) == dim{2, 1});

        const auto& space = c.glyphs.get_glyph_properties(2);
        CHECK(space.advance == 2);
        CHECK(atlas.picture_size(2) == dim{0, 0});
    }

    TEST_CASE("the lookup maps code points to the font's bytes") {
        const auto c = convert_bitmap_font(synthetic_font());
        CHECK(c.lookup.size() == 3);
        CHECK(c.lookup.at(U'A') == 0);
        CHECK(c.lookup.at(U'C') == 2);
        CHECK_FALSE(c.lookup.contains(U'D'));
    }

    TEST_CASE("BIOS 8x16: 256 CP437 glyphs, control-byte pictures reachable") {
        const auto& bios = onyx_font::bios_font_8x16();
        const auto c = convert_bitmap_font(bios);
        CHECK(c.glyphs.glyph_count() == 256);
        const auto f = c.glyphs.get_font_properties();
        CHECK(f.line_height == 16);
        CHECK(f.baseline == bios.get_metrics().ascent);
        CHECK(f.space_advance == 8);

        CHECK(c.lookup.at(U'A') == 0x41);
        CHECK(c.lookup.at(U'░') == 0xB0); // light shade
        CHECK(c.lookup.at(U'é') == 0x82); // e acute
        CHECK(c.lookup.at(U'☺') == 0x01); // smiley, the picture of byte 1
        CHECK(c.lookup.at(U'\u0001') == 0x01); // the control code reaches the same glyph
        CHECK(c.lookup.at(U'⌂') == 0x7F); // house
        CHECK_FALSE(c.lookup.contains(U'€')); // no euro in CP437

        const auto& atlas = details::glyph_atlas_access::atlas(c.glyphs);
        CHECK(atlas.picture_size(0xDB) == dim{8, 16}); // full block fills the cell
        CHECK(c.glyphs.get_glyph_properties(0xDB).bearing_x == 0);
        CHECK(c.glyphs.get_glyph_properties(0xDB).y_offset == 0);
        CHECK(atlas.picture_size(0x20) == dim{0, 0}); // space
        for (std::size_t i = 0; i < 256; i++) {
            CHECK(c.glyphs.get_glyph_properties(i).advance == 8);
        }
    }
}

TEST_SUITE("neutrino::font onyx bitmap pixels") {
    TEST_CASE("every BIOS glyph's picture holds exactly the font's lit pixels") {
        for (const auto* bios : {&onyx_font::bios_font_8x8(), &onyx_font::bios_font_8x14(),
                                 &onyx_font::bios_font_8x16()}) {
            const auto c = convert_bitmap_font(*bios);
            const auto& atlas = details::glyph_atlas_access::atlas(c.glyphs);
            int mismatches = 0;
            for (unsigned ch = 0; ch < 256; ch++) {
                const auto glyph = bios->get_glyph(static_cast <std::uint8_t>(ch));
                const auto& p = c.glyphs.get_glyph_properties(ch);
                const auto place = atlas_test::place(atlas, ch);
                const dim size = atlas.picture_size(ch);
                for (int y = 0; y < glyph.height(); y++) {
                    for (int x = 0; x < glyph.width(); x++) {
                        // BIOS glyphs have no A space; bitmap row 0 is the cell top.
                        const int px = x - p.bearing_x;
                        const int py = y - p.y_offset;
                        bool lit = false;
                        if (px >= 0 && py >= 0 && px < size.width && py < size.height) {
                            const auto pixel = atlas_test::page(atlas, place.page)
                                .get_pixel(place.dest.x + px, place.dest.y + py);
                            REQUIRE(pixel.has_value());
                            lit = pixel->a == 255;
                        }
                        if (lit != glyph.pixel(static_cast <std::uint16_t>(x), static_cast <std::uint16_t>(y))) {
                            mismatches++;
                        }
                    }
                }
            }
            CHECK(mismatches == 0);
        }
    }

    TEST_CASE("BIOS 8x16 conversion cost") {
        const auto t0 = std::chrono::steady_clock::now();
        const auto c = convert_bitmap_font(onyx_font::bios_font_8x16());
        const auto t1 = std::chrono::steady_clock::now();
        CHECK(c.glyphs.glyph_count() == 256);
        MESSAGE("BIOS 8x16 conversion (glyphs, pack, code point lookup): ",
                std::chrono::duration <double, std::milli>(t1 - t0).count(), " ms");
    }
}

TEST_SUITE("neutrino::font queries") {
    TEST_CASE("properties and glyphs of a registered font") {
        neutrino::test::test_application app("font queries");
        const font_id id = register_bios_font(bios_font::vga_8x16);
        const auto props = get_font_properties(id);
        CHECK(props.line_height == 16);
        CHECK(props.space_advance == 8);

        const auto a = get_glyph_properties(id, U'A');
        REQUIRE(a.has_value());
        CHECK(a->glyph == 0x41);
        CHECK(a->advance == 8);

        CHECK(get_glyph_properties(id, U'☺')->glyph == 0x01);
        const auto euro = get_glyph_properties(id, U'€'); // drawn as the default glyph
        REQUIRE(euro.has_value());
        CHECK(euro->glyph == props.default_glyph);

        CHECK(has_glyph(id, U'A'));
        CHECK(has_glyph(id, U'☺'));
        CHECK_FALSE(has_glyph(id, U'€'));
        unregister_font(id);
    }

    TEST_CASE("without a default glyph an unmapped code point has no glyph") {
        neutrino::test::test_application app("font queries no default");
        glyph_atlas_builder <glyph_atlas_format::alpha> b({.default_glyph = '?'});
        b.start_glyph('x', {.advance = 3});
        b.put(0, 0, 255);
        b.end_glyph();
        const font_id id = register_font(std::move(b).build());
        CHECK(get_glyph_properties(id, U'x')->advance == 3);
        CHECK_FALSE(get_glyph_properties(id, U'y').has_value());
        CHECK_FALSE(has_glyph(id, U'y'));
        unregister_font(id);
    }

    TEST_CASE("queries on an unknown font throw") {
        neutrino::test::test_application app("font queries unknown");
        const font_id id = register_bios_font(bios_font::cga_8x8);
        unregister_font(id);
        CHECK_THROWS_AS((void)get_font_properties(id), std::runtime_error);
        CHECK_THROWS_AS((void)get_glyph_properties(id, U'A'), std::runtime_error);
        CHECK_THROWS_AS((void)has_glyph(id, U'A'), std::runtime_error);
    }
}

TEST_SUITE("neutrino::font registration") {
    TEST_CASE("register_bios_font registers 256 glyphs") {
        neutrino::test::test_application app("bios font");
        const font_id id = register_bios_font(bios_font::vga_8x16);
        const auto& f = require_fonts_manager().get(id);
        CHECK(f.glyph_props.size() == 256);
        CHECK(require_texture_registry().picture_count(f.atlas) == 256);
        CHECK(glyph_of(f, U'░').glyph == 0xB0);
        unregister_font(id);
    }

    TEST_CASE("every BIOS font registers with its height") {
        neutrino::test::test_application app("bios fonts");
        for (const auto [which, height] : {std::pair{bios_font::cga_8x8, 8}, std::pair{bios_font::ega_8x14, 14},
                                           std::pair{bios_font::vga_8x16, 16}}) {
            const font_id id = register_bios_font(which);
            CHECK(require_fonts_manager().get(id).font_props.line_height == height);
            unregister_font(id);
        }
    }

    TEST_CASE("register_font(glyph_atlas) takes glyph ids as code points") {
        neutrino::test::test_application app("font code points");
        glyph_atlas_builder <glyph_atlas_format::alpha> b({.default_glyph = '?'});
        b.start_glyph('x');
        b.put(0, 0, 255);
        b.end_glyph();
        b.start_glyph('?');
        b.put(0, 0, 255);
        b.end_glyph();
        const font_id id = register_font(std::move(b).build());
        const auto& f = require_fonts_manager().get(id);
        CHECK(f.find_glyph(U'x') == 0);
        CHECK(f.find_glyph(U'y') == 1);
        unregister_font(id);
    }

    TEST_CASE("a CP437 FON file is detected and loaded") {
        neutrino::test::test_application app("fon cp437");
        const auto data = read_font_file("VGAOEM.FON");
        const font_id id = register_font(data);
        const auto& f = require_fonts_manager().get(id);
        CHECK(f.font_props.line_height > 0);
        CHECK(glyph_of(f, U'A').glyph == 'A');
        CHECK(glyph_of(f, U'░').glyph == 0xB0);
        unregister_font(id);
    }

    TEST_CASE("a proportional FON file keeps per-glyph advances") {
        neutrino::test::test_application app("fon proportional");
        const auto data = read_font_file("HELVA.FON");
        const font_id id = register_font(data);
        const auto& f = require_fonts_manager().get(id);
        CHECK(glyph_of(f, U'i').advance < glyph_of(f, U'W').advance);
        unregister_font(id);
    }

    TEST_CASE("a registered custom decoder is found by its data") {
        neutrino::test::test_application app("custom font decoder");
        register_font_decoder(std::make_unique <synthetic_decoder>());
        const std::vector <std::uint8_t> data{'N', 'T', 'F', '0', 0, 0};
        const font_id id = register_font(data);
        const auto& f = require_fonts_manager().get(id);
        CHECK(f.glyph_props.size() == 3);
        CHECK(glyph_of(f, U'A').advance == 4);
        CHECK(glyph_of(f, U'Z').glyph == 'B'); // unmapped -> default char
        unregister_font(id);
    }

    TEST_CASE("a decoder named explicitly is used without detection") {
        neutrino::test::test_application app("explicit font decoder");
        const synthetic_decoder dec;
        const std::vector <std::uint8_t> data{0, 1, 2, 3};
        const font_id id = register_font(data, dec);
        CHECK(require_fonts_manager().get(id).glyph_props.size() == 3);
        unregister_font(id);
    }

    TEST_CASE("failures throw") {
        neutrino::test::test_application app("font failures");
        const std::vector <std::uint8_t> junk{1, 2, 3, 4, 5, 6, 7, 8};
        CHECK_THROWS_AS((void)register_font(junk), std::runtime_error);         // nobody recognises it
        const synthetic_decoder dec;
        CHECK_THROWS_AS((void)register_font(junk, dec, 1), std::runtime_error); // decoder failure, wrapped
        CHECK_THROWS_AS(register_font_decoder(nullptr), std::runtime_error);
    }
}
