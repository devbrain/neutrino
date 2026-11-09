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
#include <neutrino/video/text/glyph_mapping.hh>

#include "video/text/fixed26_6.hh"

namespace neutrino {
    /// The identity mapping: each glyph's id is taken as its code point.
    /// @throws std::runtime_error when a glyph id is past U+10FFFF.
    [[nodiscard]] glyph_mapping glyph_ids_as_code_points(const std::vector <glyph_properties>& glyphs);

    struct loaded_font {
        /**
         * @param mapping Code point -> glyph id; kept as code point -> glyph index.
         * @throws std::runtime_error when @p mapping names a glyph id the font does not have.
         */
        loaded_font(atlas_id atlas_, font_properties font_props_, std::vector <glyph_properties>&& glyph_props_,
                    const glyph_mapping& mapping);

        /// The font's own glyph for @p code_point; the default glyph does not count.
        [[nodiscard]] std::optional <std::size_t> own_glyph(char32_t code_point) const;

        /// The glyph drawn for @p code_point: its own, else the default glyph; nullopt when
        /// the font has neither.
        [[nodiscard]] std::optional <std::size_t> find_glyph(char32_t code_point) const;

        atlas_id atlas;
        font_properties font_props;
        std::vector <glyph_properties> glyph_props;
        /// Code point -> glyph index (glyph i = picture i).
        glyph_mapping lookup;
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
}
