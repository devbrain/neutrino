//
// Created by igor on 23/11/2026.
//

#pragma once

#include <cstdint>

namespace neutrino {
    /**
     * @brief What a texture's pixels hold (painter.md §2.8, R49): straight colour, or colour
     *        already multiplied by alpha.
     *
     * A draw combines its source with the painter's blend in the form this says the pixels
     * need: from a premultiplied texture, @c blend and @c add use their premultiplied forms.
     * A target texture is premultiplied, since blended drawing into a transparent target
     * leaves premultiplied colour in it; fixed and streaming textures are straight.
     */
    enum class alpha_mode : std::uint8_t {
        straight,      ///< Colour as given; alpha says how much of it covers.
        premultiplied  ///< Colour already multiplied by alpha.
    };
}
