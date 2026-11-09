//
// Created by igor on 09/11/2026.
//

#include <algorithm>
#include <iterator>
#include <limits>

#include <neutrino/video/text/glyph_mapping.hh>
#include <failsafe/failsafe.hh>

namespace neutrino {
    namespace {
        // Whether @p next starts where @p r ends, at the glyph after r's last (64-bit: r's
        // last glyph may be the largest id).
        bool continues(const glyph_mapping::range& r, const glyph_mapping::range& next) noexcept {
            return r.end == next.begin &&
                   std::uint64_t{r.first_glyph} + (r.end - r.begin) == std::uint64_t{next.first_glyph};
        }
    }

    glyph_mapping::glyph_mapping(std::initializer_list <range> ranges) {
        for (const auto& r : ranges) {
            bind(r.begin, r.end, r.first_glyph);
        }
    }

    // ---------------------------------------------------------------------------------------------------------------
    void glyph_mapping::bind(char32_t begin, char32_t end, std::uint32_t first_glyph) {
        const auto b = static_cast <std::uint32_t>(begin);
        const auto e = static_cast <std::uint32_t>(end);
        ENFORCE(b < e, "empty range [", b, ", ", e, ")");
        ENFORCE(e - 1 <= max_code_point, "[", b, ", ", e, ") goes past U+10FFFF");
        ENFORCE(e - b - 1 <= std::numeric_limits <std::uint32_t>::max() - first_glyph,
                "[", b, ", ", e, ") from glyph ", first_glyph, " overflows the glyph ids");

        const auto itr = std::lower_bound(m_ranges.begin(), m_ranges.end(), begin,
                                          [](const range& r, char32_t v) {
                                              return r.begin < v;
                                          });
        const bool hits_next = itr != m_ranges.end() && itr->intersects(begin, end);
        const bool hits_prev = itr != m_ranges.begin() && std::prev(itr)->intersects(begin, end);
        ENFORCE(!hits_next && !hits_prev, "[", b, ", ", e, ") overlaps a bound range");

        auto pos = m_ranges.insert(itr, range{begin, end, first_glyph});
        if (const auto next = std::next(pos); next != m_ranges.end() && continues(*pos, *next)) {
            pos->end = next->end;
            m_ranges.erase(next);
        }
        if (pos != m_ranges.begin()) {
            if (const auto prev = std::prev(pos); continues(*prev, *pos)) {
                prev->end = pos->end;
                m_ranges.erase(pos);
            }
        }
    }

    // ---------------------------------------------------------------------------------------------------------------
    void glyph_mapping::bind(char32_t cp, std::uint32_t glyph) {
        ENFORCE(cp <= max_code_point, static_cast <std::uint32_t>(cp), " is past U+10FFFF");
        bind(cp, cp + 1, glyph);
    }

    // ---------------------------------------------------------------------------------------------------------------
    std::optional <std::uint32_t> glyph_mapping::find(char32_t cp) const {
        auto itr = std::upper_bound(m_ranges.begin(), m_ranges.end(), cp,
                                    [](char32_t v, const range& r) {
                                        return v < r.begin;
                                    });
        if (itr == m_ranges.begin()) {
            return std::nullopt;
        }
        --itr;
        if (itr->in_range(cp)) {
            return itr->map(cp);
        }
        return std::nullopt;
    }
}
