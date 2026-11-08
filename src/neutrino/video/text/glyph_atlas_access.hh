//
// Created by igor on 08/11/2026.
//

#pragma once

// Private access to a glyph_atlas's packed pictures: register_atlas takes them over, tests
// inspect them. Header-only, so the library and the tests see the same definition.

#include <utility>

#include <neutrino/video/text/glyph_atlas.hh>

namespace neutrino::details {
    struct glyph_atlass_acces {
        static image_atlas extract_atlas(glyph_atlas&& atlas) {
            return std::move(atlas.m_atlas);
        }

        static const image_atlas& atlas(const glyph_atlas& a) {
            return a.m_atlas;
        }

        static const font_properties& font(const glyph_atlas& a) {
            return a.m_font_props;
        }
    };
}
