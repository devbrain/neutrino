//
// Created by igor on 08/11/2026.
//

#pragma once

// onyx bitmap fonts (FNT, FON, GEM, BIOS, custom decoders) -> glyph_atlas + code point lookup.

#include <onyx_font/bitmap_font.hh>

#include <neutrino/video/text/glyph_atlas.hh>
#include "video/text/fonts_manager.hh"

namespace neutrino {
    struct converted_bitmap_font {
        glyph_atlas glyphs;
        glyph_lookup lookup;
    };

    /**
     * @brief Draw an onyx bitmap font into an alpha glyph atlas.
     *
     * Glyph ids are the font's byte codes, from first to last char; glyph i is byte
     * first + i. The lookup maps every code point the font's charset encodes to a byte in
     * the font (cp437's control-byte pictures included). Font properties: baseline = ascent,
     * line height = pixel height + external leading, space advance = the break char's advance,
     * default glyph = the default char. Advance = A + B + C (B defaults to the bitmap width).
     */
    [[nodiscard]] converted_bitmap_font convert_bitmap_font(const onyx_font::bitmap_font& font);
}
