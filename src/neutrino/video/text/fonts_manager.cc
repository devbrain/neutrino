//
// Created by igor on 08/11/2026.
//

#include <atomic>
#include <failsafe/enforce.hh>
#include <neutrino/video/sprite/texture_atlas.hh>
#include "fonts_manager.hh"

namespace neutrino {
    glyph_lookup glyph_ids_as_code_points(const std::vector <glyph_properties>& glyphs) {
        glyph_lookup lookup;
        lookup.reserve(glyphs.size());
        for (std::size_t i = 0; i < glyphs.size(); i++) {
            lookup.emplace(static_cast <char32_t>(glyphs[i].glyph), i);
        }
        return lookup;
    }

    loaded_font::loaded_font(atlas_id atlas_, font_properties font_props_,
                             std::vector <glyph_properties>&& glyph_props_, glyph_lookup&& lookup_)
        : atlas(atlas_),
          font_props(font_props_),
          glyph_props(std::move(glyph_props_)),
          lookup(std::move(lookup_)) {
        for (const auto& [code_point, index] : lookup) {
            ENFORCE(index < glyph_props.size(), "Code point", static_cast <std::uint32_t>(code_point),
                    "maps to glyph index", index, "; the font has", glyph_props.size(), "glyphs");
        }
        advances.reserve(glyph_props.size());
        for (std::size_t i = 0; i < glyph_props.size(); i++) {
            advances.push_back(details::fixed26_6::from_float(glyph_props[i].advance));
            if (!default_index && glyph_props[i].glyph == font_props.default_glyph) {
                default_index = i;
            }
        }
    }

    std::optional <std::size_t> loaded_font::find_glyph(char32_t code_point) const noexcept {
        if (const auto itr = lookup.find(code_point); itr != lookup.end()) {
            return itr->second;
        }
        return default_index;
    }

    font_id fonts_manager::add(loaded_font&& f) {
        static std::atomic <uint32_t> font_id_counter = 0;
        auto fid = details::id_access::make <font_id>(font_id_counter.fetch_add(1, std::memory_order_relaxed));
        m_fonts.emplace(fid, std::move(f));
        return fid;
    }

    const loaded_font& fonts_manager::get(font_id fid) const {
        auto itr = m_fonts.find(fid);
        ENFORCE(itr != m_fonts.end(), "Font", fid, "not found");
        return itr->second;
    }

    void fonts_manager::erase(font_id fid) {
        auto itr = m_fonts.find(fid);
        if (itr != m_fonts.end()) {
            unregister_atlas(itr->second.atlas);
            m_fonts.erase(itr);
        }
    }
}
