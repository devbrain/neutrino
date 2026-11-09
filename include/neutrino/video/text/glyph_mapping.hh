//
// Created by igor on 09/11/2026.
//

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include <neutrino/neutrino_export.h>

namespace neutrino {
    namespace details {
        struct glyph_mapping_access;
    }

    /**
     * @brief Maps Unicode code points to glyph ids (font_plan.md F3, FM2).
     *
     * Holds sorted, non-overlapping half-open ranges [begin, end) of code points; the code
     * point @c begin + k maps to glyph @c first_glyph + k. Many code points may map to one
     * glyph (case folding, look-alike letters of other scripts); offsets and code pages are
     * ranges or single code points. Code points that are not bound draw the font's
     * @c default_glyph. Lookup is a binary search.
     *
     * A range that continues its neighbour (it starts where the neighbour ends, at the glyph
     * after the neighbour's last) is merged into it, so binding a table one code point at a
     * time keeps few ranges.
     */
    class NEUTRINO_EXPORT glyph_mapping {
        friend struct details::glyph_mapping_access;

        public:
            /// The largest Unicode code point.
            static constexpr char32_t max_code_point = 0x10FFFF;

            /**
             * @brief Map the code points [@p begin, @p end) to glyphs @p first_glyph, @p first_glyph + 1, ...
             *
             * @throws std::runtime_error when the range is empty, goes past @ref max_code_point,
             *         its glyph ids overflow, or it overlaps a range already bound.
             */
            void bind(char32_t begin, char32_t end, std::uint32_t first_glyph);

            /// @brief Map the code point @p cp to @p glyph. Throws as the range overload does.
            void bind(char32_t cp, std::uint32_t glyph);

            /// @brief The glyph mapped to @p cp; nullopt when @p cp is not bound.
            [[nodiscard]] std::optional <std::uint32_t> find(char32_t cp) const;

        private:
            /// Code points [begin, end) map to glyphs first_glyph, first_glyph + 1, ...
            struct range {
                char32_t begin;
                char32_t end;
                std::uint32_t first_glyph;

                [[nodiscard]] bool in_range(char32_t v) const noexcept {
                    return begin <= v && v < end;
                }

                [[nodiscard]] std::uint32_t map(char32_t v) const noexcept {
                    return first_glyph + (v - begin);
                }

                [[nodiscard]] bool intersects(char32_t other_begin, char32_t other_end) const noexcept {
                    return begin < other_end && other_begin < end;
                }

                /// Whether @p next starts where this range ends, at the glyph after its last
                /// (64-bit: the last glyph may be the largest id).
                [[nodiscard]] bool continued_by(const range& next) const noexcept {
                    return end == next.begin &&
                           std::uint64_t{first_glyph} + (end - begin) == std::uint64_t{next.first_glyph};
                }
            };

            /// Sorted by code point, neighbours merged.
            std::vector <range> m_ranges;
    };
}
