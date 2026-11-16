//
// Created by igor on 16/11/2026.
//

#pragma once

#include <cstdint>
#include <neutrino/detail/enum.hh>

namespace neutrino {
    /**
     * @brief How a picture, a texture or a layer is mirrored when drawn: bit flags.
     *
     * Each flag undoes itself, so two flips compose with @c ^ — a draw's flip with an
     * appearance's own, for example. The position given to a draw is the visual origin after
     * the flips. @ref diagonal follows Tiled's diagonal flag: applied before the horizontal and
     * vertical flags, it swaps the bottom-left and top-right corners, an x/y axis swap.
     */
    enum class flip_mode : std::uint8_t {
        none = 0,              ///< Drawn as it is.
        horizontal = 1u << 0u, ///< Mirrored left to right.
        vertical = 1u << 1u,   ///< Mirrored top to bottom.
        diagonal = 1u << 2u    ///< Mirrored across the top-left to bottom-right diagonal (tiles).
    };

    d_NEUTRINO_PROMOTE_ENUM(flip_mode);
}
