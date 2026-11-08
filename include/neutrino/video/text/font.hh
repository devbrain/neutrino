//
// Created by igor on 08/11/2026.
//

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

#include <onyx_font/decoder.hh>
#include <onyx_font/font_factory.hh> // font_entry, for decoders' enumerate()

#include <neutrino/video/text/glyph_atlas.hh>
#include <neutrino/neutrino_export.h>
#include <neutrino/detail/id_strong_type.hh>

#include <neutrino/video/sprite/texture_atlas.hh>

namespace neutrino {
    namespace details {
        struct font_id_tag;
    }

    class NEUTRINO_EXPORT font_id
        : public details::id_strong_type <details::font_id_tag> {
        friend struct details::id_access;

        public:
            font_id() = default;

        private:
            explicit font_id(std::uint32_t value)
                : id_strong_type(value) {
            }
    };

    /**
     * @brief Register a font built with a glyph_atlas_builder.
     *
     * Uploads the glyph pictures as a texture atlas and keeps the font and glyph properties.
     * Each glyph's id is taken as its Unicode code point; code points without a glyph draw
     * the glyph whose id is @c font_properties::default_glyph, when the font has it.
     *
     * @throws std::runtime_error when the application is not ready.
     */
    [[nodiscard]] NEUTRINO_EXPORT font_id register_font(glyph_atlas&& glyphs,
        atlas_texture_format format = atlas_texture_format::automatic,
        texture_filter filter = texture_filter::linear);

    // ------------------------------------------------------------------------------------
    // Bitmap font files: decoders (the same arrangement as image decoders)
    // ------------------------------------------------------------------------------------

    /// A bitmap font format: recognises its data (@c sniff) and loads a face from it.
    using bitmap_font_decoder = onyx_font::bitmap_font_decoder;

    /**
     * @brief Add a bitmap font decoder, tried after the built-in ones (GEM, Windows FNT / FON).
     * @throws std::runtime_error when @p dec is null.
     */
    NEUTRINO_EXPORT void register_font_decoder(std::unique_ptr <bitmap_font_decoder> dec);

    /**
     * @brief Register face @p face of a bitmap font file, decoded by the first registered
     *        decoder that recognises @p data.
     *
     * Glyphs are white with coverage as alpha. Code points reach glyphs through the font's
     * character set (CP437, Windows-1252, Latin-1); unmapped ones draw the font's default char.
     *
     * @throws std::runtime_error when no decoder recognises the data, the decoder fails, or
     *         the application is not ready.
     */
    [[nodiscard]] NEUTRINO_EXPORT font_id register_font(std::span <const std::uint8_t> data,
        std::size_t face = 0,
        atlas_texture_format format = atlas_texture_format::automatic,
        texture_filter filter = texture_filter::linear);

    /// @brief Register face @p face of @p data, decoded by @p dec.
    /// @throws std::runtime_error when the decoder fails or the application is not ready.
    [[nodiscard]] NEUTRINO_EXPORT font_id register_font(std::span <const std::uint8_t> data,
        const bitmap_font_decoder& dec,
        std::size_t face = 0,
        atlas_texture_format format = atlas_texture_format::automatic,
        texture_filter filter = texture_filter::linear);

    /// The IBM PC BIOS fonts (Code Page 437, 256 glyphs, 8 pixels wide).
    enum class bios_font {
        cga_8x8,
        ega_8x14,
        vga_8x16
    };

    /// @brief Register a built-in BIOS font.
    /// @throws std::runtime_error when the application is not ready.
    [[nodiscard]] NEUTRINO_EXPORT font_id register_bios_font(bios_font which,
        atlas_texture_format format = atlas_texture_format::automatic,
        texture_filter filter = texture_filter::linear);

    /**
     * @brief Unregister a font and its texture atlas.
     *
     * Does nothing for an invalid id, an id already unregistered, or after the application
     * is torn down.
     */
    NEUTRINO_EXPORT void unregister_font(font_id font);
}

template<>
struct std::hash <neutrino::font_id>
    : std::hash <neutrino::details::id_strong_type <neutrino::details::font_id_tag>> {
};
