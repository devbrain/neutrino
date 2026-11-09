//
// Created by igor on 09/11/2026.
//

#pragma once

// Private access to a glyph_mapping's ranges: the font registry rewrites them as glyph
// indices, tests inspect merging. Header-only, so the library and the tests see the same
// definition.

#include <cstddef>
#include <cstdint>

#include <neutrino/video/text/glyph_mapping.hh>

namespace neutrino::details {
    struct glyph_mapping_access {
        /// One bound range: code points [begin, end) -> glyphs first_glyph, first_glyph + 1, ...
        struct range_view {
            char32_t begin;
            char32_t end;
            std::uint32_t first_glyph;

            bool operator==(const range_view&) const = default;
        };

        static std::size_t range_count(const glyph_mapping& m) noexcept {
            return m.m_ranges.size();
        }

        static range_view range(const glyph_mapping& m, std::size_t i) {
            const auto& r = m.m_ranges.at(i);
            return {r.begin, r.end, r.first_glyph};
        }
    };
}
