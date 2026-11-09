//
// Created by igor on 08/11/2026.
//

#pragma once

// Text layout queries (font_plan.md P1, FM1 item 5): measure, wrap, fit, caret position and
// hit test, one code path for every font kind. Nothing here touches pixels; draw_text (FM3)
// draws the same layout.
//
// Text is UTF-8; malformed bytes show the default glyph. Indices are byte offsets into the
// text. Coordinates are pixels from the top-left of the text block (the top of the first
// line's cell), x right, y down. '\n' ends a line ("\r\n" too: '\r' takes no space); '\t'
// moves to the next tab stop; a space without a space glyph advances the font's
// space_advance; other code points without a glyph draw the default glyph, or nothing when
// the font has none.

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/text/font.hh>

namespace neutrino {
    enum class text_align {
        left,
        center,
        right
    };

    /// How text is laid out. Drawing options (colour, ...) join in FM3.
    struct text_params {
        /// Each line within the wrap width, or within the widest line when not wrapping.
        text_align align = text_align::left;
        /// Multiplies every metric; keep it whole for pixel fonts (T3). Must be > 0.
        float scale = 1.0f;
        /// Extra pixels between neighbouring characters, on top of the font's letter spacing.
        int letter_spacing = 0;
        /// Extra pixels between lines, on top of the font's line height.
        int line_spacing = 0;
        /// Tab stops every tab_width space advances from the line start.
        int tab_width = 4;
    };

    /// One laid out line: bytes [begin, end) of the text, without the spaces a wrap broke at
    /// and without the '\n'.
    struct line_range {
        std::size_t begin = 0;
        std::size_t end = 0;
        int width = 0;

        bool operator ==(const line_range&) const = default;
    };

    /// The longest start of the text's first line that fits a width.
    struct fit_result {
        std::size_t count = 0; ///< bytes of the text that fit
        int extent = 0;        ///< their width in pixels

        bool operator ==(const fit_result&) const = default;
    };

    /**
     * @brief Size of the laid out text: the widest line, and the height of all lines.
     *
     * Empty text is one empty line. With @p wrap_width, lines are wrapped as by @ref wrap.
     * @throws std::runtime_error for an unknown font or a scale that is not positive.
     */
    [[nodiscard]] NEUTRINO_EXPORT dim measure(std::string_view text, font_id font, const text_params& params = {},
                                              std::optional <int> wrap_width = {});

    /**
     * @brief Break the text into lines no wider than @p width.
     *
     * Lines break at spaces and tabs; the spaces at a break belong to neither line's range.
     * A word wider than @p width is broken between characters; every line holds at least
     * one character, so a single glyph wider than @p width still gets a line.
     */
    [[nodiscard]] NEUTRINO_EXPORT std::vector <line_range> wrap(std::string_view text, font_id font, int width,
                                                                const text_params& params = {});

    /// @brief How much of the text's first line fits in @p width (for truncation, text input).
    [[nodiscard]] NEUTRINO_EXPORT fit_result fit(std::string_view text, font_id font, int width,
                                                 const text_params& params = {});

    /**
     * @brief The caret position before the character at byte @p index: its x, and the top of
     *        its line.
     *
     * @p index is clamped to the text; an index inside a multi-byte character means that
     * character; the text's size means after the last character.
     */
    [[nodiscard]] NEUTRINO_EXPORT point offset_of(std::string_view text, font_id font, std::size_t index,
                                                  const text_params& params = {},
                                                  std::optional <int> wrap_width = {});

    /**
     * @brief The byte index of the caret position nearest @p position (hit test).
     *
     * The line is the one at @p position's y (clamped to the first and last line); within it,
     * the caret before the character whose left half holds x, or the end of the line.
     * For text whose characters all advance, `index_at(offset_of(i)) == i` at every character
     * start.
     */
    [[nodiscard]] NEUTRINO_EXPORT std::size_t index_at(std::string_view text, font_id font, point position,
                                                       const text_params& params = {},
                                                       std::optional <int> wrap_width = {});
}
