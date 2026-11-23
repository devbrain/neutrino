//
// Created by igor on 16/11/2026.
//

#pragma once

#include <neutrino/video/alpha_mode.hh>
#include <neutrino/video/blend_mode.hh>
#include <sdlpp/video/blend_mode.hh>

// The one place neutrino's blend modes meet SDL's. Nothing else relies on the two sets of
// values agreeing.

namespace neutrino::detail {
    /// @p mode as sdlpp's blend mode, in the form a source whose pixels are @p source needs
    /// (painter.md §2.8, R49): from a premultiplied source, @c blend and @c add are their
    /// premultiplied forms; @c none, @c mul and @c mod are the same in both. A value outside
    /// the enum maps to sdlpp's @c invalid, which SDL rejects, so a draw with it fails and is
    /// counted instead of drawing something else.
    [[nodiscard]] constexpr sdlpp::blend_mode to_sdl_blend(blend_mode mode,
                                                           alpha_mode source = alpha_mode::straight) noexcept {
        const bool premultiplied = source == alpha_mode::premultiplied;
        switch (mode) {
            case blend_mode::none: return sdlpp::blend_mode::none;
            case blend_mode::blend:
                return premultiplied ? sdlpp::blend_mode::blend_premultiplied : sdlpp::blend_mode::blend;
            case blend_mode::add:
                return premultiplied ? sdlpp::blend_mode::add_premultiplied : sdlpp::blend_mode::add;
            case blend_mode::mul: return sdlpp::blend_mode::mul;
            case blend_mode::mod: return sdlpp::blend_mode::mod;
        }
        return sdlpp::blend_mode::invalid;
    }
}
