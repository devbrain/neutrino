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
#include <cstdint>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/image/image.hh>
#include <neutrino/video/palette.hh>
#include <neutrino/video/color.hh>
#include <neutrino/video/geometry_types.hh>

namespace neutrino {
    /**
     * @brief The pixel format of an @ref image_builder.
     */
    enum class image_kind {
        indexed, ///< INDEX8: one palette index per pixel.
        rgba     ///< RGBA32: bytes R, G, B, A in memory on every platform.
    };

    namespace details {
        template<image_kind K>
        struct image_pixel;

        template<>
        struct image_pixel <image_kind::indexed> {
            using type = std::uint8_t;
        };

        template<>
        struct image_pixel <image_kind::rgba> {
            using type = color;
        };
    }

    /**
     * @brief Builds one @ref image of kind @p K: indexed (INDEX8) or RGBA (RGBA32).
     *
     * The kind is part of the type, so a pixel of the wrong kind, or a palette or colour key
     * on an RGBA image, does not compile. Every pixel starts at 0: index 0 for an indexed image,
     * transparent black for RGBA. Write pixels with @ref set, @ref row or @ref fill, then call
     * @ref build once.
     *
     * @code{.cpp}
     * indexed_image_builder b({16, 16}, pal, 0);           // index 0 is transparent
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
    template<image_kind K>
    class image_builder {
        public:
            /// @brief One pixel: a palette index (indexed) or a @ref color (RGBA).
            using pixel_type = typename details::image_pixel <K>::type;

            static constexpr image_kind kind = K;

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
            explicit image_builder(dim dimensions, std::optional <std::uint8_t> transparent_index = {})
                requires (K == image_kind::indexed);

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
             *        image's colour key (see the constructor without a palette).
             */
            image_builder(dim dimensions, const palette& pal, std::optional <std::uint8_t> transparent_index = {})
                requires (K == image_kind::indexed);

            /**
             * @brief Start an RGBA image (RGBA32). Transparency is alpha 0.
             *
             * @param dimensions Size in pixels.
             */
            explicit image_builder(dim dimensions)
                requires (K == image_kind::rgba);

            image_builder(const image_builder&) = delete;
            image_builder& operator = (const image_builder&) = delete;

            image_builder(image_builder&&) noexcept = default;
            image_builder& operator = (image_builder&&) noexcept = default;
            ~image_builder() = default;

            /**
             * @brief Write @p p at (@p x, @p y).
             *
             * @throws std::runtime_error when the pixel is outside the image.
             */
            void set(int x, int y, pixel_type p);

            /**
             * @brief Set every pixel to @p p.
             *
             * For a background other than 0: the colour key of an indexed image whose transparent
             * index is not 0, or transparent white (`{255, 255, 255, 0}`) under alpha-only
             * pictures, so that filtered edges do not blend toward black.
             */
            void fill(pixel_type p);

            /**
             * @brief The pixels of row @p y, for decoders that produce a row at a time (planar, RLE).
             *
             * One element per pixel: a palette index, or a @ref color whose bytes are R, G, B, A
             * in memory. The span covers exactly the image width, without the padding SDL may add
             * to each row, and stays valid until @ref build.
             *
             * @throws std::runtime_error when @p y is outside the image.
             */
            std::span <pixel_type> row(int y);

            /// @brief Width and height in pixels.
            [[nodiscard]] dim size() const noexcept;

            /**
             * @brief Finish and return the image.
             *
             * Called on an rvalue (`std::move(builder).build()`), because it hands over the
             * image instead of copying it.
             */
            [[nodiscard]] image build() &&;

        private:
            image m_image;
    };

    /// @brief Builder of an indexed (INDEX8) image.
    using indexed_image_builder = image_builder <image_kind::indexed>;
    /// @brief Builder of an RGBA (RGBA32) image.
    using rgba_image_builder = image_builder <image_kind::rgba>;

    // Both kinds are instantiated once, in image_builder.cc.
    extern template class NEUTRINO_EXPORT image_builder <image_kind::indexed>;
    extern template class NEUTRINO_EXPORT image_builder <image_kind::rgba>;
}
