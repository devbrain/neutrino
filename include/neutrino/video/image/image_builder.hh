//
// Created by igor on 29/10/2026.
//

#pragma once

/**
 * @file image_builder.hh
 * @brief Writing pixels into a new @ref image without locking, pitch or format code.
 *
 * A game's own image parser (planar EGA, RLE sprites, packed glyphs) creates a builder,
 * writes pixels by coordinate or a row at a time, and calls @ref image_builder::build.
 * The result is an ordinary @ref image, the same type the standard codecs return.
 */

#include <optional>
#include <span>
#include <memory>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/image/image.hh>
#include <neutrino/video/palette.hh>
#include <neutrino/video/geometry_types.hh>

namespace neutrino {
    /**
     * @brief Builds one @ref image in one of two pixel formats: indexed (INDEX8) or RGBA (RGBA32).
     *
     * Every pixel starts at 0: index 0 for an indexed image, transparent black for RGBA.
     * Write pixels with @ref set or @ref row, then call @ref build once.
     *
     * @code{.cpp}
     * auto b = image_builder::indexed({16, 16}, pal, 0);    // index 0 is transparent
     * for (int y = 0; y < 16; y++) {
     *     auto px = b.row(y);
     *     for (int x = 0; x < 16; x++) {
     *         px[x] = read_index(stream);
     *     }
     * }
     * image tile = std::move(b).build();
     * @endcode
     *
     * Move-only. A moved-from builder, or one that has already been built, must not be used again.
     */
    class NEUTRINO_EXPORT image_builder {
        public:
            /**
             * @brief Start an indexed (INDEX8) image with no palette.
             *
             * For formats whose palette comes from somewhere else (a sibling `.pal` file,
             * the level): the image holds indices only. Attach a palette before it is drawn
             * or converted to RGBA.
             *
             * @param dimensions Size in pixels.
             * @param transparent_index Palette index to treat as transparent. It is stored as
             *        the image's colour key, so it is skipped when blitting and stays
             *        transparent after a palette swap.
             */
            static image_builder indexed(dim dimensions, std::optional <uint8_t> transparent_index = {});

            /**
             * @brief Start an indexed (INDEX8) image that uses @p pal.
             *
             * The image shares @p pal rather than copying it. SDL palettes are reference
             * counted, so every image of a file can share one palette, and later changes to
             * @p pal are visible in the image.
             *
             * @param dimensions Size in pixels.
             * @param pal The image's palette.
             * @param transparent_index Palette index to treat as transparent, stored as the
             *        image's colour key (see the overload without a palette).
             */
            static image_builder indexed(dim dimensions, const palette& pal,
                                         std::optional <uint8_t> transparent_index = {});

            /**
             * @brief Start an RGBA image (RGBA32: bytes R, G, B, A in memory on every platform).
             *
             * Transparency is alpha 0.
             *
             * @param dimensions Size in pixels.
             */
            static image_builder rgba(dim dimensions);

            image_builder(const image_builder&) = delete;
            image_builder& operator = (const image_builder&) = delete;

            image_builder(image_builder&&) noexcept;
            image_builder& operator = (image_builder&&) noexcept;
            ~image_builder();

            /**
             * @brief Write palette index @p index at (@p x, @p y) of an indexed image.
             *
             * @throws std::runtime_error when the builder is RGBA or the pixel is outside the image.
             */
            void set(int x, int y, uint8_t index);

            /**
             * @brief Write colour @p c at (@p x, @p y) of an RGBA image.
             *
             * @throws std::runtime_error when the builder is indexed or the pixel is outside the image.
             */
            void set(int x, int y, color c);

            /**
             * @brief The pixels of row @p y, for decoders that produce a row at a time (planar, RLE).
             *
             * An indexed image gives one byte (palette index) per pixel. An RGBA image gives
             * four bytes per pixel, in the order R, G, B, A on every platform. The span covers
             * exactly the image width, without the padding SDL may add to each row, and stays
             * valid until @ref build.
             *
             * @throws std::runtime_error when @p y is outside the image.
             */
            std::span <uint8_t> row(int y);

            /**
             * @brief Finish and return the image.
             *
             * Called on an rvalue (`std::move(builder).build()`), because it hands over the
             * image instead of copying it.
             */
            [[nodiscard]] image build() const &&;

        private:
            struct impl;
            std::unique_ptr <impl> m_pimpl;
        private:
            explicit image_builder(std::unique_ptr <impl> pimpl);
    };
}
