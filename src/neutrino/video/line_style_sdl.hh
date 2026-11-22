//
// Created by igor on 17/11/2026.
//

#pragma once

#include <neutrino/video/line_style.hh>
#include <sdlpp/video/line_style.hh>

// The one place neutrino's line style meets sdlpp's. SDL has no line styles: sdlpp keeps the
// style in its renderer wrapper and dashes lines itself. Nothing else relies on the two types
// agreeing.

namespace neutrino::detail {
    /// @p style as sdlpp's line style, the pattern mapped by name and the lengths copied.
    [[nodiscard]] constexpr sdlpp::line_style to_sdl_line(const line_style& style) noexcept {
        sdlpp::line_style out;
        switch (style.pattern) {
            case line_pattern::dashed: out.type = sdlpp::line_style_type::dashed; break;
            case line_pattern::dotted: out.type = sdlpp::line_style_type::dotted; break;
            case line_pattern::solid: out.type = sdlpp::line_style_type::solid; break;
        }
        out.dash = style.dash;
        out.gap = style.gap;
        out.spacing = style.spacing;
        return out;
    }

    /// sdlpp's @p style as neutrino's, the pattern mapped by name and the lengths copied.
    [[nodiscard]] constexpr line_style from_sdl_line(const sdlpp::line_style& style) noexcept {
        line_style out;
        switch (style.type) {
            case sdlpp::line_style_type::dashed: out.pattern = line_pattern::dashed; break;
            case sdlpp::line_style_type::dotted: out.pattern = line_pattern::dotted; break;
            case sdlpp::line_style_type::solid: out.pattern = line_pattern::solid; break;
        }
        out.dash = style.dash;
        out.gap = style.gap;
        out.spacing = style.spacing;
        return out;
    }
}
