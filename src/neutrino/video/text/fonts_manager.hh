//
// Created by igor on 08/11/2026.
//

#pragma once

#include <cstddef>
#include <optional>
#include <unordered_map>
#include <vector>

#include <neutrino/video/atlas/atlas_id.hh>
#include <neutrino/video/text/glyph_atlas.hh>
#include <neutrino/video/text/font.hh>

#include "video/text/fixed26_6.hh"

namespace neutrino {
    /// Unicode code point -> glyph index (glyph i = picture i). Many code points may share a glyph.
    using glyph_lookup = std::unordered_map <char32_t, std::size_t>;

    /// The identity lookup: each glyph's id is taken as its code point.
    [[nodiscard]] glyph_lookup glyph_ids_as_code_points(const std::vector <glyph_properties>& glyphs);

    struct loaded_font {
        /// @throws std::runtime_error when @p lookup names a glyph index the font does not have.
        loaded_font(atlas_id atlas_, font_properties font_props_, std::vector <glyph_properties>&& glyph_props_,
                    glyph_lookup&& lookup_);

        /// The glyph drawn for @p code_point: its own, else the default glyph; nullopt when
        /// the font has neither.
        [[nodiscard]] std::optional <std::size_t> find_glyph(char32_t code_point) const noexcept;

        atlas_id atlas;
        font_properties font_props;
        std::vector <glyph_properties> glyph_props;
        glyph_lookup lookup;
        /// glyph_props[i].advance in 26.6, what layout adds up (P9).
        std::vector <details::fixed26_6> advances;
        /// Index of the glyph whose id is font_props.default_glyph, when the font has it.
        std::optional <std::size_t> default_index;
    };

    class fonts_manager {
        public:
            font_id add(loaded_font&& f);
            [[nodiscard]] const loaded_font& get(font_id fid) const;
            /// Erase @p fid and unregister its atlas; an unknown id is ignored.
            void erase(font_id fid);
        private:
            std::unordered_map<font_id, loaded_font> m_fonts;
    };

    namespace details {
        /// register_font with an explicit code point lookup (sources whose glyph ids are not
        /// code points, e.g. 8-bit fonts in a code page).
        [[nodiscard]] font_id register_font(glyph_atlas&& glyphs, glyph_lookup&& lookup,
                                            atlas_texture_format format, texture_filter filter);
    }
}
