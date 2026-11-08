//
// Created by igor on 08/11/2026.
//

#pragma once

// Private access to a glyph_atlas's parts: register_font takes them over, tests inspect them.
// Header-only, so the library and the tests see the same definition.

#include <utility>
#include <vector>

#include <neutrino/video/text/glyph_atlas.hh>

namespace neutrino::details {
    struct glyph_atlas_access {
        static image_atlas extract_atlas(glyph_atlas&& atlas) {
            return std::move(atlas.m_atlas);
        }

        static std::vector <glyph_properties> extract_glyphs(glyph_atlas&& atlas) {
            return std::move(atlas.m_props);
        }

        static const image_atlas& atlas(const glyph_atlas& a) {
            return a.m_atlas;
        }

        static const std::vector <glyph_properties>& glyphs(const glyph_atlas& a) {
            return a.m_props;
        }

        static const font_properties& font(const glyph_atlas& a) {
            return a.m_font_props;
        }
    };
}
