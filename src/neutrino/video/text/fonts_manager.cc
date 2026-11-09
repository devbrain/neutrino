//
// Created by igor on 08/11/2026.
//

#include <algorithm>
#include <atomic>
#include <utility>
#include <failsafe/enforce.hh>
#include <neutrino/video/sprite/texture_atlas.hh>
#include "fonts_manager.hh"
#include "video/text/glyph_mapping_access.hh"

namespace neutrino {
    glyph_mapping glyph_ids_as_code_points(const std::vector <glyph_properties>& glyphs) {
        std::vector <std::uint32_t> ids;
        ids.reserve(glyphs.size());
        for (const auto& g : glyphs) {
            ids.push_back(g.glyph);
        }
        std::sort(ids.begin(), ids.end());
        glyph_mapping mapping;
        for (const auto id : ids) {
            mapping.bind(static_cast <char32_t>(id), id); // ascending: each bind appends or merges
        }
        return mapping;
    }

    namespace {
        // Rewrite code point -> glyph id as code point -> glyph index. A range splits where
        // consecutive ids stop being consecutive pictures.
        glyph_mapping to_glyph_indices(const glyph_mapping& mapping, const std::vector <glyph_properties>& glyphs) {
            std::vector <std::pair <std::uint32_t, std::size_t>> by_id; // (id, index), sorted by id
            by_id.reserve(glyphs.size());
            for (std::size_t i = 0; i < glyphs.size(); i++) {
                by_id.emplace_back(glyphs[i].glyph, i);
            }
            std::sort(by_id.begin(), by_id.end());

            using access = details::glyph_mapping_access;
            glyph_mapping out;
            for (std::size_t i = 0, n = access::range_count(mapping); i < n; i++) {
                const auto r = access::range(mapping, i);
                const auto id_of = [&r](char32_t cp) { return r.first_glyph + (cp - r.begin); };
                char32_t cp = r.begin;
                while (cp < r.end) {
                    const std::uint32_t id = id_of(cp);
                    auto it = std::lower_bound(by_id.begin(), by_id.end(), id,
                                               [](const auto& e, std::uint32_t v) { return e.first < v; });
                    ENFORCE(it != by_id.end() && it->first == id, "Code point", static_cast <std::uint32_t>(cp),
                            "maps to glyph", id, "; the font has no such glyph");
                    const char32_t run_begin = cp;
                    const std::size_t first_index = it->second;
                    for (++cp, ++it; cp < r.end && it != by_id.end() && it->first == id_of(cp) &&
                                     it->second == first_index + (cp - run_begin); ++cp, ++it) {
                    }
                    out.bind(run_begin, cp, static_cast <std::uint32_t>(first_index));
                }
            }
            return out;
        }
    }

    loaded_font::loaded_font(atlas_id atlas_, font_properties font_props_,
                             std::vector <glyph_properties>&& glyph_props_, const glyph_mapping& mapping)
        : atlas(atlas_),
          font_props(font_props_),
          glyph_props(std::move(glyph_props_)),
          lookup(to_glyph_indices(mapping, glyph_props)) {
        advances.reserve(glyph_props.size());
        for (std::size_t i = 0; i < glyph_props.size(); i++) {
            advances.push_back(details::fixed26_6::from_float(glyph_props[i].advance));
            if (!default_index && glyph_props[i].glyph == font_props.default_glyph) {
                default_index = i;
            }
        }
    }

    std::optional <std::size_t> loaded_font::own_glyph(char32_t code_point) const {
        return lookup.find(code_point);
    }

    std::optional <std::size_t> loaded_font::find_glyph(char32_t code_point) const {
        if (const auto g = own_glyph(code_point)) {
            return g;
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
