//
// Created by igor on 08/11/2026.
//

#include "video/text/onyx_bitmap_font.hh"

#include <cstdint>
#include <limits>
#include <utility>

#include <failsafe/enforce.hh>
#include <onyx_font/charset.hh>

namespace neutrino {
    namespace {
        std::int16_t to_int16(int v, const char* what) {
            ENFORCE(v >= std::numeric_limits <std::int16_t>::min() && v <= std::numeric_limits <std::int16_t>::max(),
                    "Bitmap font", what, "is out of range:", v);
            return static_cast <std::int16_t>(v);
        }

        int advance_of(const onyx_font::bitmap_font& font, std::uint8_t ch) {
            const auto& s = font.get_spacing(ch);
            return s.a_space.value_or(0) + s.b_space.value_or(font.get_glyph(ch).width()) + s.c_space.value_or(0);
        }

        // The charsets are 8-bit with ASCII below 0x80 and every other character in the BMP,
        // so the BMP holds every code point that encodes to a byte.
        constexpr char32_t last_code_point = 0xFFFF;
    }

    converted_bitmap_font convert_bitmap_font(const onyx_font::bitmap_font& font) {
        const unsigned first = font.get_first_char();
        const unsigned last = font.get_last_char();
        ENFORCE(first <= last, "Bitmap font", font.get_name(), ": first char", first, "is after last char", last);
        const auto in_font = [first, last](unsigned ch) { return ch >= first && ch <= last; };

        const auto& m = font.get_metrics();
        const auto break_char = font.get_break_char();
        font_properties props;
        props.line_height = to_int16(m.pixel_height + m.external_leading, "line height");
        props.baseline = to_int16(m.ascent, "ascent");
        props.space_advance = to_int16(in_font(break_char) ? advance_of(font, break_char) : m.avg_width,
                                       "space advance");
        props.default_glyph = font.get_default_char();

        glyph_atlas_builder <glyph_atlas_format::alpha> builder(props);
        const int ascent = m.ascent;
        for (unsigned ch = first; ch <= last; ch++) {
            const auto code = static_cast <std::uint8_t>(ch);
            const auto glyph = font.get_glyph(code);
            const int a = font.get_spacing(code).a_space.value_or(0);
            builder.start_glyph(ch, {.advance = static_cast <float>(advance_of(font, code))});
            bitmask mask(glyph.width(), glyph.height());
            for (std::uint16_t y = 0; y < glyph.height(); y++) {
                for (std::uint16_t x = 0; x < glyph.width(); x++) {
                    mask.set(x, y, glyph.pixel(x, y));
                }
            }
            // Bitmap rows start at the cell top; the baseline is `ascent` below it.
            builder.put_mask(a, -ascent, mask, 0xFF);
            builder.end_glyph();
        }

        // Ascending code points: each bind appends or extends the last range.
        glyph_mapping mapping;
        const auto set = font.get_charset();
        for (char32_t cp = 0; cp <= last_code_point; cp++) {
            if (const auto code = onyx_font::encode_char(set, cp); code && in_font(*code)) {
                mapping.bind(cp, static_cast <std::uint32_t>(*code));
            }
        }

        return {std::move(builder).build(), std::move(mapping)};
    }
}
