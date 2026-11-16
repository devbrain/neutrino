//
// Created by igor on 15/11/2026.
//

#pragma once

#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/color.hh>

namespace neutrino::detail {
    /**
     * @brief What a layer target or a canvas asks the texture allocator for (painter.md §3.3).
     *
     * Format (ARGB8888) and access (target) are fixed by the allocator, and texture::create gives
     * every texture the engine's blend and filtering, so the size alone keys the pool. The clear
     * colour belongs to the handle, not to the texture: a reused texture is cleared to the new
     * handle's colour.
     */
    struct texture_spec {
        /// @brief In pixels. A zero or negative side gives a failed handle.
        dim size;
        /// @brief What acquire(), clear() and the resets fill the texture with: opaque white for
        ///        a @c mod layer, transparent for every other target.
        color clear_color;
    };
}
