//
// Created by igor on 27/10/2026.
//

#pragma once

/**
 * @file palette.hh
 * @brief Colour palettes for indexed images, and loading them from raw bytes.
 */

#include <cstdint>
#include <span>

#include <neutrino/video/color.hh>
#include <neutrino/neutrino_export.h>
#include <sdlpp/video/palette.hh>

namespace neutrino {
    /**
     * @brief An indexed image's colour table (up to 256 entries).
     *
     * The sdlpp type itself, so it attaches to an indexed @c image with no conversion.
     */
    using palette = sdlpp::palette; // sdlpp-ok: alias

    /**
     * @brief How a raw palette stores each entry. Both use three bytes per entry, in R, G, B order.
     */
    enum class palette_encoding {
        vga6, ///< 6 bits per channel (0..63), as VGA DAC dumps store them; scaled to 0..255.
        rgb8  ///< 8 bits per channel (0..255), stored as-is.
    };

    /**
     * @brief Build a palette from a headerless table of RGB triples.
     *
     * Every entry is opaque. The entry count is @c bytes.size() / 3.
     *
     * @param bytes The raw table: a non-empty multiple of 3 bytes, at most 256 entries.
     * @param enc   How each channel is stored.
     * @return The palette.
     * @throws std::runtime_error if @p bytes is empty, not a multiple of 3, or longer than
     *         256 entries, or if a @c vga6 channel exceeds 63.
     */
    [[nodiscard]] NEUTRINO_EXPORT palette load_palette(std::span <const std::uint8_t> bytes, palette_encoding enc);
}
