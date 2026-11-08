//
// Created by igor on 05/11/2026.
//

#include <neutrino/video/text/glyph_atlas.hh>
#include <failsafe/enforce.hh>

namespace neutrino {
    std::size_t glyph_atlas::glyph_count() const noexcept {
        return m_props.size();
    }

    const glyph_properties& glyph_atlas::get_glyph_properties(std::size_t idx) const {
        ENFORCE(idx < m_props.size(), "Glyph index out of range. There are", m_props.size(), "glyphs");
        return m_props[idx];
    }

    font_properties glyph_atlas::get_font_properties() const noexcept {
        return m_font_props;
    }

    glyph_atlas::glyph_atlas(const font_properties& font_props, image_atlas&& atlas, std::vector <glyph_properties>&& props)
        : m_font_props(font_props),
          m_atlas(std::move(atlas)),
          m_props(std::move(props)) {
    }
}
