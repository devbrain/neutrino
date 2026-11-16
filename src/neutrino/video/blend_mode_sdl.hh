//
// Created by igor on 16/11/2026.
//

#pragma once

#include <optional>

#include <neutrino/video/blend_mode.hh>
#include <sdlpp/video/blend_mode.hh>

// The one place neutrino's blend modes meet SDL's. Nothing else relies on the two sets of
// values agreeing.

namespace neutrino::detail {
    /// @p mode as sdlpp's blend mode. A value outside the enum maps to sdlpp's @c invalid, which
    /// SDL rejects, so a draw with it fails and is counted instead of drawing something else.
    [[nodiscard]] constexpr sdlpp::blend_mode to_sdl_blend(blend_mode mode) noexcept {
        switch (mode) {
            case blend_mode::none: return sdlpp::blend_mode::none;
            case blend_mode::blend: return sdlpp::blend_mode::blend;
            case blend_mode::add: return sdlpp::blend_mode::add;
            case blend_mode::mul: return sdlpp::blend_mode::mul;
            case blend_mode::mod: return sdlpp::blend_mode::mod;
            case blend_mode::blend_premultiplied: return sdlpp::blend_mode::blend_premultiplied;
            case blend_mode::add_premultiplied: return sdlpp::blend_mode::add_premultiplied;
        }
        return sdlpp::blend_mode::invalid;
    }

    /// sdlpp's @p mode as neutrino's, or nullopt for a mode neutrino has no name for: sdlpp's
    /// @c invalid, or a custom blend set behind neutrino's back (texture::native()).
    [[nodiscard]] constexpr std::optional <blend_mode> from_sdl_blend(sdlpp::blend_mode mode) noexcept {
        switch (mode) {
            case sdlpp::blend_mode::none: return blend_mode::none;
            case sdlpp::blend_mode::blend: return blend_mode::blend;
            case sdlpp::blend_mode::add: return blend_mode::add;
            case sdlpp::blend_mode::mul: return blend_mode::mul;
            case sdlpp::blend_mode::mod: return blend_mode::mod;
            case sdlpp::blend_mode::blend_premultiplied: return blend_mode::blend_premultiplied;
            case sdlpp::blend_mode::add_premultiplied: return blend_mode::add_premultiplied;
            case sdlpp::blend_mode::invalid: break;
        }
        return std::nullopt;
    }
}
