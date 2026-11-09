//
// Tests for text layout: measure (lines, letter / line spacing, scale, spaces, tabs, 26.6
// advances), wrap (word breaks, long words, space runs, newlines), alignment, fit, caret
// positions and hit tests (incl. the round trip and multi-byte characters), glyph picture
// positions, missing glyphs and malformed UTF-8; the public font_id API.
//
#include <doctest/doctest.h>

#include <neutrino/video/text/text_layout.hh>

#include "test_application.hh"
#include "services/service_access.hh"
#include "video/text/fonts_manager.hh"
#include "video/text/text_layout_engine.hh"

#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

using namespace neutrino;
using neutrino::details::layout_text;

namespace {
    // Monospace 8 px: a-z, A-Z, é, ö, '?'; no space glyph (space_advance 4); line height 10.
    // 'b' has bearing 1 and y offset 2, to check picture positions.
    loaded_font mono(std::uint32_t default_glyph = '?', bool with_default = true) {
        std::vector <glyph_properties> g;
        auto add = [&g](char32_t cp) {
            g.push_back({.glyph = static_cast <std::uint32_t>(cp), .advance = 8});
        };
        for (char32_t c = U'a'; c <= U'z'; c++) add(c);
        for (char32_t c = U'A'; c <= U'Z'; c++) add(c);
        add(U'é');
        add(U'ö');
        if (with_default) add(U'?');
        g[1].bearing_x = 1;
        g[1].y_offset = 2;
        auto lookup = glyph_ids_as_code_points(g);
        return {{}, {.line_height = 10, .baseline = 8, .space_advance = 4, .default_glyph = default_glyph},
                std::move(g), std::move(lookup)};
    }

    dim measure_of(std::string_view text, const loaded_font& f, const text_params& p = {},
                   std::optional <int> w = {}) {
        return layout_text(text, f, p, w).size;
    }

    std::vector <line_range> wrap_of(std::string_view text, const loaded_font& f, int w, const text_params& p = {}) {
        std::vector <line_range> out;
        for (const auto& l : layout_text(text, f, p, w).lines) {
            out.push_back(l.range);
        }
        return out;
    }

    point caret(std::string_view text, const loaded_font& f, std::size_t i, const text_params& p = {},
                std::optional <int> w = {}) {
        return details::caret_position(layout_text(text, f, p, w), text, i);
    }

    std::size_t hit(std::string_view text, const loaded_font& f, point pos, const text_params& p = {},
                    std::optional <int> w = {}) {
        return details::hit_test(layout_text(text, f, p, w), text, pos);
    }
}

TEST_SUITE("neutrino::text_layout measure") {
    TEST_CASE("one line, empty text, several lines, trailing newline") {
        const auto f = mono();
        CHECK(measure_of("abc", f) == dim{24, 10});
        CHECK(measure_of("", f) == dim{0, 10});
        CHECK(measure_of("ab\ncde", f) == dim{24, 20});
        CHECK(measure_of("ab\n", f) == dim{16, 20});
        CHECK(measure_of("ab\r\ncd", f) == dim{16, 20}); // '\r' takes no space
    }

    TEST_CASE("letter spacing goes between characters; line spacing between lines") {
        const auto f = mono();
        CHECK(measure_of("abc", f, {.letter_spacing = 2}) == dim{28, 10});
        CHECK(measure_of("a\nb", f, {.line_spacing = 3}) == dim{8, 23});
    }

    TEST_CASE("the font's letter spacing adds to the parameter") {
        auto f = mono();
        f.font_props.letter_spacing = 1;
        CHECK(measure_of("abc", f, {.letter_spacing = 2}) == dim{30, 10});
    }

    TEST_CASE("scale multiplies every metric") {
        const auto f = mono();
        CHECK(measure_of("abc\nd", f, {.scale = 2.0f}) == dim{48, 40});
    }

    TEST_CASE("a space without a space glyph advances space_advance; tabs go to tab stops") {
        const auto f = mono();
        CHECK(measure_of("a b", f) == dim{20, 10});
        CHECK(measure_of("a\tb", f) == dim{24, 10});          // stops every 4 x 4 = 16 px
        CHECK(measure_of("a\tb", f, {.tab_width = 8}) == dim{40, 10});
    }

    TEST_CASE("a space glyph's own advance wins over space_advance") {
        std::vector <glyph_properties> g{{.glyph = 'a', .advance = 8},
                                         {.glyph = ' ', .advance = 6}};
        auto lookup = glyph_ids_as_code_points(g);
        const loaded_font f({}, {.line_height = 10, .space_advance = 4}, std::move(g), std::move(lookup));
        CHECK(measure_of("a a", f) == dim{22, 10});
    }

    TEST_CASE("fractional advances add up without drift; each caret snaps to a pixel") {
        std::vector <glyph_properties> g{{.glyph = 'i', .advance = 6.4f}}; // stored as 410/64 = 6.40625 px
        auto lookup = glyph_ids_as_code_points(g);
        const loaded_font f({}, {.line_height = 10}, std::move(g), std::move(lookup));
        CHECK(measure_of("iiiiiiiiii", f).width == 64); // not 10 x 6 = 60
        CHECK(caret("iii", f, 1).x == 6);
        CHECK(caret("iii", f, 2).x == 13);
    }

    TEST_CASE("missing glyphs draw the default glyph, or nothing without one") {
        CHECK(measure_of("a#b", mono()) == dim{24, 10});
        CHECK(measure_of("a#b", mono('!', false)) == dim{16, 10});
        CHECK(measure_of("\xff", mono()) == dim{8, 10}); // malformed UTF-8 -> default glyph
    }

    TEST_CASE("a scale that is not positive throws") {
        CHECK_THROWS_AS((void)measure_of("a", mono(), {.scale = 0.0f}), std::runtime_error);
    }
}

TEST_SUITE("neutrino::text_layout wrap") {
    TEST_CASE("lines break at spaces; the break spaces belong to no line") {
        const auto f = mono();
        // A space is 4 px here (space_advance): "hello world" is 84 px.
        CHECK(wrap_of("hello world foo", f, 84) == std::vector <line_range>{{0, 11, 84}, {12, 15, 24}});
        CHECK(wrap_of("hello world foo", f, 83) == std::vector <line_range>{{0, 5, 40}, {6, 15, 68}});
        CHECK(wrap_of("hello world foo", f, 50) ==
              std::vector <line_range>{{0, 5, 40}, {6, 11, 40}, {12, 15, 24}});
        CHECK(wrap_of("ab   cd", f, 24) == std::vector <line_range>{{0, 2, 16}, {5, 7, 16}});
    }

    TEST_CASE("a word wider than the width breaks between characters") {
        const auto f = mono();
        CHECK(wrap_of("abcdefghij", f, 30) ==
              std::vector <line_range>{{0, 3, 24}, {3, 6, 24}, {6, 9, 24}, {9, 10, 8}});
        CHECK(wrap_of("ab", f, 0) == std::vector <line_range>{{0, 1, 8}, {1, 2, 8}}); // one character per line
    }

    TEST_CASE("newlines end lines; empty paragraphs are empty lines") {
        const auto f = mono();
        CHECK(wrap_of("ab\ncd", f, 100) == std::vector <line_range>{{0, 2, 16}, {3, 5, 16}});
        CHECK(wrap_of("ab\n\ncd", f, 100) == std::vector <line_range>{{0, 2, 16}, {3, 3, 0}, {4, 6, 16}});
        CHECK(wrap_of("", f, 100) == std::vector <line_range>{{0, 0, 0}});
    }

    TEST_CASE("leading spaces stay with the paragraph's first line") {
        CHECK(wrap_of("  abcd", mono(), 24) == std::vector <line_range>{{0, 4, 24}, {4, 6, 16}});
    }

    TEST_CASE("trailing spaces do not count in a line's width") {
        CHECK(wrap_of("ab  ", mono(), 100) == std::vector <line_range>{{0, 2, 16}});
        CHECK(measure_of("ab  ", mono()) == dim{16, 10});
    }

    TEST_CASE("wrapped measure: the widest line and all lines' height") {
        CHECK(measure_of("hello world foo", mono(), {}, 50) == dim{40, 30});
    }
}

TEST_SUITE("neutrino::text_layout align, carets, hit tests") {
    TEST_CASE("alignment within the wrap width, or within the widest line") {
        const auto f = mono();
        CHECK(caret("ab", f, 0, {.align = text_align::center}, 30) == point{7, 0});
        CHECK(caret("ab", f, 0, {.align = text_align::right}, 30) == point{14, 0});
        CHECK(caret("abcd\nab", f, 5, {.align = text_align::center}) == point{8, 10});
        CHECK(caret("abcd\nab", f, 5, {.align = text_align::right}) == point{16, 10});
    }

    TEST_CASE("caret positions") {
        const auto f = mono();
        CHECK(caret("ab\ncd", f, 0) == point{0, 0});
        CHECK(caret("ab\ncd", f, 1) == point{8, 0});
        CHECK(caret("ab\ncd", f, 2) == point{16, 0}); // before the '\n'
        CHECK(caret("ab\ncd", f, 3) == point{0, 10});
        CHECK(caret("ab\ncd", f, 5) == point{16, 10}); // after the last character
        CHECK(caret("ab\ncd", f, 99) == point{16, 10}); // clamped
        CHECK(caret("ab\n", f, 3) == point{0, 10});    // the empty last line
    }

    TEST_CASE("an index inside a multi-byte character means that character") {
        const std::string_view text = "aéb"; // é is bytes 1-2
        const auto f = mono();
        CHECK(caret(text, f, 2) == caret(text, f, 1));
        CHECK(caret(text, f, 3) == point{16, 0});
    }

    TEST_CASE("hit tests pick the nearest caret on the line at y") {
        const auto f = mono();
        CHECK(hit("ab\ncd", f, {3, 0}) == 0);
        CHECK(hit("ab\ncd", f, {4, 0}) == 1);   // right half of 'a'
        CHECK(hit("ab\ncd", f, {100, 5}) == 2); // past the line: before its '\n'
        CHECK(hit("ab\ncd", f, {-5, 25}) == 3); // below the last line: clamped to it
        CHECK(hit("ab\ncd", f, {100, 15}) == 5);
        CHECK(hit("ab\ncd", f, {0, -7}) == 0);
    }

    TEST_CASE("caret round trip over wrapped multi-byte text") {
        const std::string_view text = "héllo wörld  foo\nbar\n\nbaz qux";
        const auto f = mono();
        for (const auto align : {text_align::left, text_align::center, text_align::right}) {
            const text_params p{.align = align, .letter_spacing = 1};
            std::size_t i = 0;
            while (true) {
                INFO("index ", i);
                CHECK(hit(text, f, caret(text, f, i, p, 40), p, 40) == i);
                if (i == text.size()) {
                    break;
                }
                i++;
                while (i < text.size() && (static_cast <unsigned char>(text[i]) & 0xC0) == 0x80) {
                    i++; // continuation bytes are not character starts
                }
            }
        }
    }

    TEST_CASE("glyph pictures sit at caret + bearing, line top + y offset, scaled") {
        const auto f = mono();
        const auto l1 = layout_text("ab\nb", f, {}, {});
        REQUIRE(l1.lines.size() == 2);
        CHECK(l1.lines[0].chars[1].picture == point{9, 2});
        CHECK(l1.lines[1].chars[0].picture == point{1, 12});
        const auto l2 = layout_text("ab", f, {.scale = 2.0f}, {});
        CHECK(l2.lines[0].chars[1].picture == point{18, 4});
        CHECK(l2.lines[0].chars[1].glyph == 1u);
        CHECK_FALSE(layout_text("a b", f, {}, {}).lines[0].chars[1].glyph.has_value()); // no space glyph
    }
}

TEST_SUITE("neutrino::text_layout fit") {
    TEST_CASE("the longest start of the first line that fits") {
        const auto f = mono();
        CHECK(details::fit_text("abcdef", f, 20, {}) == fit_result{2, 16});
        CHECK(details::fit_text("abcdef", f, 0, {}) == fit_result{0, 0});
        CHECK(details::fit_text("ab\ncd", f, 100, {}) == fit_result{2, 16});
        CHECK(details::fit_text("aéb", f, 16, {}) == fit_result{3, 16}); // é is two bytes
    }
}

TEST_SUITE("neutrino::text_layout public API") {
    TEST_CASE("queries take a registered font") {
        neutrino::test::test_application app("text layout");
        const font_id id = register_font(bios_font::vga_8x16);
        CHECK(measure("Hello\nWorld!", id) == dim{48, 32});
        CHECK(wrap("Hello World", id, 50) == std::vector <line_range>{{0, 5, 40}, {6, 11, 40}});
        CHECK(fit("Hello", id, 20) == fit_result{2, 16});
        CHECK(offset_of("Hello", id, 2) == point{16, 0});
        CHECK(index_at("Hello", id, {17, 3}) == 2);
        CHECK(measure("░☺", id) == dim{16, 16}); // CP437 pictures resolve
        unregister_font(id);
        CHECK_THROWS_AS((void)measure("x", id), std::runtime_error);
    }
}
