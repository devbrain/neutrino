//
// Created by igor on 22/11/2026.
//

#pragma once

#include <cstdint>

#include <neutrino/video/flip_mode.hh>
#include <sdlpp/video/blend_mode.hh>

// Where neutrino's flip_mode meets SDL's: SDL has no diagonal flip.

namespace neutrino::detail {
    /// True if @p flags has @p flag.
    [[nodiscard]] constexpr bool has_flip(flip_mode flags, flip_mode flag) noexcept {
        return static_cast <std::uint8_t>(flags & flag) != 0;
    }

    /// True if @p flags has the diagonal flip, which SDL's texture copies cannot draw.
    [[nodiscard]] constexpr bool has_diagonal_flip(flip_mode flags) noexcept {
        return has_flip(flags, flip_mode::diagonal);
    }

    /// The horizontal and vertical flips of @p flags as sdlpp's; the diagonal one is dropped.
    [[nodiscard]] constexpr sdlpp::flip_mode to_sdl_flip(flip_mode flags) noexcept {
        auto result = sdlpp::flip_mode::none;
        if (has_flip(flags, flip_mode::horizontal)) {
            result |= sdlpp::flip_mode::horizontal;
        }
        if (has_flip(flags, flip_mode::vertical)) {
            result |= sdlpp::flip_mode::vertical;
        }
        return result;
    }
}
