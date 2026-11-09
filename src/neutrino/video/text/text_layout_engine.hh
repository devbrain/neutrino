//
// Created by igor on 08/11/2026.
//

#pragma once

// The layout behind text_layout.hh, over a loaded_font so it runs without an application.
// draw_text / render_text (FM3) consume the same text_layout.

#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/text/text_layout.hh>

#include "video/text/fonts_manager.hh"

namespace neutrino::details {
    struct laid_out_char {
        std::size_t index = 0;        ///< byte offset in the text
        char32_t code_point = 0;
        int caret_x = 0;              ///< caret before the character (line alignment included)
        int advance = 0;              ///< pixels to the next caret
        std::optional <std::size_t> glyph; ///< glyph index drawn; none for tabs, '\n', '\r', missing glyphs
        point picture{};              ///< top-left of the glyph's picture, when there is a glyph
    };

    struct laid_out_line {
        line_range range;             ///< visible bytes and width
        int x = 0;                    ///< alignment offset
        int top = 0;                  ///< top of the line's cell
        /// Every character of the line, the spaces a wrap broke at and the '\n' included.
        std::vector <laid_out_char> chars;
        /// Byte after the line's last character: where the next line starts.
        std::size_t after = 0;
        int end_caret_x = 0;          ///< caret after the line's last character
    };

    struct text_layout {
        std::vector <laid_out_line> lines; ///< never empty
        dim size{};
        int line_step = 0;            ///< top of one line to the top of the next
    };

    /// @throws std::runtime_error when params.scale is not positive.
    [[nodiscard]] text_layout layout_text(std::string_view text, const loaded_font& font, const text_params& params,
                                          std::optional <int> wrap_width);

    [[nodiscard]] fit_result fit_text(std::string_view text, const loaded_font& font, int width,
                                      const text_params& params);

    [[nodiscard]] point caret_position(const text_layout& layout, std::string_view text, std::size_t index);

    [[nodiscard]] std::size_t hit_test(const text_layout& layout, std::string_view text, point position);
}
