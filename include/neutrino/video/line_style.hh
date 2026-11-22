//
// Created by igor on 17/11/2026.
//

#pragma once

#include <cstdint>

namespace neutrino {
    /// @brief The pattern a line is drawn with (see @ref line_style).
    enum class line_pattern : std::uint8_t {
        solid,  ///< One unbroken line.
        dashed, ///< Dashes of line_style::dash pixels separated by line_style::gap pixels.
        dotted  ///< Single pixels every line_style::spacing pixels.
    };

    /**
     * @brief How draw_line() draws: solid, dashed or dotted, with the lengths of the pattern
     *        (set_line_style()).
     *
     * The lengths are in render-space pixels along the line, starting at its first point. Only
     * the lengths of the current @ref pattern are used. Build one with solid(), dashed() or
     * dotted(); the defaults are sdlpp's, 8-pixel dashes and 4-pixel dot spacing.
     */
    struct line_style {
        line_pattern pattern{line_pattern::solid}; ///< Which pattern; the default draws solid lines.
        float        dash{8.0f};                   ///< dashed: the length of each dash.
        float        gap{8.0f};                    ///< dashed: the length of each gap.
        float        spacing{4.0f};                ///< dotted: the distance between dots.

        /// @brief A solid line.
        [[nodiscard]] static constexpr line_style solid() noexcept {
            return {};
        }

        /// @brief Dashes of @p dash pixels separated by gaps of @p gap pixels.
        [[nodiscard]] static constexpr line_style dashed(float dash = 8.0f, float gap = 8.0f) noexcept {
            line_style s;
            s.pattern = line_pattern::dashed;
            s.dash = dash;
            s.gap = gap;
            return s;
        }

        /// @brief Dots @p spacing pixels apart.
        [[nodiscard]] static constexpr line_style dotted(float spacing = 4.0f) noexcept {
            line_style s;
            s.pattern = line_pattern::dotted;
            s.spacing = spacing;
            return s;
        }

        /// @brief True when the pattern and every length are equal.
        bool operator==(const line_style&) const = default;
    };
}
