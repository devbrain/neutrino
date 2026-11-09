//
// Created by igor on 08/11/2026.
//

#include <neutrino/video/text/text_layout.hh>

#include "services/service_access.hh"
#include "video/text/fonts_manager.hh"
#include "video/text/text_layout_engine.hh"

namespace neutrino {
    namespace {
        const loaded_font& font_of(font_id font) {
            return require_fonts_manager().get(font);
        }
    }

    dim measure(std::string_view text, font_id font, const text_params& params, std::optional <int> wrap_width) {
        return details::layout_text(text, font_of(font), params, wrap_width).size;
    }

    std::vector <line_range> wrap(std::string_view text, font_id font, int width, const text_params& params) {
        const auto layout = details::layout_text(text, font_of(font), params, width);
        std::vector <line_range> out;
        out.reserve(layout.lines.size());
        for (const auto& line : layout.lines) {
            out.push_back(line.range);
        }
        return out;
    }

    fit_result fit(std::string_view text, font_id font, int width, const text_params& params) {
        return details::fit_text(text, font_of(font), width, params);
    }

    point offset_of(std::string_view text, font_id font, std::size_t index, const text_params& params,
                    std::optional <int> wrap_width) {
        return details::caret_position(details::layout_text(text, font_of(font), params, wrap_width), text, index);
    }

    std::size_t index_at(std::string_view text, font_id font, point position, const text_params& params,
                         std::optional <int> wrap_width) {
        return details::hit_test(details::layout_text(text, font_of(font), params, wrap_width), text, position);
    }
}
