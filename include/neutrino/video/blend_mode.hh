//
// Created by igor on 16/11/2026.
//

#pragma once

#include <cstdint>

namespace neutrino {
    /**
     * @brief How drawn pixels combine with what the target already holds: the operation.
     *
     * One choice per draw, not a set of flags. The form SDL uses, straight or premultiplied,
     * follows from what is drawn: a texture's alpha_mode (painter.md §2.8, R49). The values are
     * neutrino's own: the engine maps them to SDL's internally, so nothing outside it depends
     * on SDL's numbers. The underlying type leaves room for custom blends (painter.md §2.4,
     * R26), which neutrino will encode itself as well.
     */
    enum class blend_mode : std::uint32_t {
        none,   ///< No blending: the source replaces the target.
        blend,  ///< Alpha blending: source × alpha + target × (1 − alpha).
        add,    ///< Additive: source × alpha added to the target; its alpha kept.
        mul,    ///< Multiply: source × target + target × (1 − alpha).
        mod     ///< Modulate: source × target; the source's alpha is ignored.
    };
}
