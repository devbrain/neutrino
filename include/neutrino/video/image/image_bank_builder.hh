//
// Created by igor on 29/10/2026.
//

#pragma once

/**
 * @file image_bank_builder.hh
 * @brief Building an @ref image_bank whose pictures share one format, palette and colour key.
 *
 * For parsers of multi-picture files (sprite banks, tile sets, fonts). The bank builder holds
 * the palette and transparent index once and hands out @ref image_builder "image builders"
 * already set up with them, so the finished bank always qualifies for an indexed atlas.
 */

#include <cstddef>
#include <cstdint>
#include <optional>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/image/image_bank.hh>
#include <neutrino/video/image/image_builder.hh>
#include <neutrino/video/palette.hh>

namespace neutrino {
    /**
     * @brief Builds an @ref image_bank of kind @p K, one picture at a time.
     *
     * @code{.cpp}
     * indexed_image_bank_builder bank(pal, 16);            // index 16 is transparent
     * for (const auto& blk : blocks) {
     *     auto img = bank.next({blk.w, blk.h});
     *     decode_planes(blk, img);                         // img.row(y) / img.set(...)
     *     bank.add(std::move(img), blk.number);            // the game's key for it
     *     hotspots[blk.number] = blk.hotspot;              // side table by id
     * }
     * image_bank result = std::move(bank).build();
     * @endcode
     *
     * As with @ref image_builder, the kind is part of the type: an RGBA bank has no palette or
     * colour key, and only takes RGBA pictures.
     *
     * Move-only.
     */
    template<image_kind K>
    class image_bank_builder {
        public:
            /**
             * @brief A bank of indexed (INDEX8) pictures sharing @p pal.
             *
             * The builder refers to @p pal, which must outlive it; the finished images keep
             * the palette alive on their own (SDL palettes are reference counted).
             *
             * @param pal The palette of every picture.
             * @param transparent_index Palette index treated as transparent, stored as every
             *        picture's colour key.
             */
            explicit image_bank_builder(const palette& pal, std::optional <uint8_t> transparent_index = {})
                requires (K == image_kind::indexed);

            /**
             * @brief A bank of indexed (INDEX8) pictures sharing @p pal, taking ownership of it.
             *
             * For a palette made just for this bank, e.g. `indexed_image_bank_builder(load_palette(bytes), 0)`.
             */
            explicit image_bank_builder(palette&& pal, std::optional <uint8_t> transparent_index = {})
                requires (K == image_kind::indexed);

            /// @brief A bank of RGBA (RGBA32) pictures; transparency is alpha 0.
            image_bank_builder()
                requires (K == image_kind::rgba);

            image_bank_builder(const image_bank_builder&) = delete;
            image_bank_builder& operator = (const image_bank_builder&) = delete;
            image_bank_builder(image_bank_builder&&) noexcept = default;
            image_bank_builder& operator = (image_bank_builder&&) noexcept = default;
            ~image_bank_builder() = default;

            /**
             * @brief A builder for the next picture, set up with the bank's palette and colour
             *        key. Pass it back to @ref add when its pixels are written.
             */
            [[nodiscard]] image_builder <K> next(dim size) const;

            /**
             * @brief Finish @p picture and append it to the bank.
             *
             * @param picture A builder from @ref next (or one made with the same palette and
             *        colour key).
             * @param id The game's key for the picture -- typically the file's own number
             *        (block number, character code, cell index). Opaque to the engine and
             *        not required to be unique; @ref image_atlas::find_by_id looks it up.
             * @return The picture's position in the bank, i.e. the atlas region it becomes.
             * @throws std::runtime_error when an indexed picture's palette or colour key
             *         differs from the bank's.
             */
            std::size_t add(image_builder <K>&& picture, uint32_t id);

            /// @brief Number of entries added so far.
            [[nodiscard]] std::size_t size() const noexcept;

            /// @brief Finish and return the bank (`std::move(builder).build()`).
            [[nodiscard]] image_bank build() &&;

        private:
            [[nodiscard]] const palette* bank_palette() const noexcept;

            const palette* m_external = nullptr;  // caller's palette (indexed, const palette&)
            std::optional <palette> m_owned;      // palette taken over (indexed, palette&&)
            std::optional <uint8_t> m_transparent_index;
            image_bank m_bank;
    };

    /// @brief Builder of a bank of indexed (INDEX8) pictures.
    using indexed_image_bank_builder = image_bank_builder <image_kind::indexed>;
    /// @brief Builder of a bank of RGBA (RGBA32) pictures.
    using rgba_image_bank_builder = image_bank_builder <image_kind::rgba>;

    // Both kinds are instantiated once, in image_bank_builder.cc.
    extern template class NEUTRINO_EXPORT image_bank_builder <image_kind::indexed>;
    extern template class NEUTRINO_EXPORT image_bank_builder <image_kind::rgba>;
}
