//
// Created by igor on 08/11/2026.
//

#include <neutrino/video/text/font.hh>

#include <exception>
#include <utility>

#include <failsafe/enforce.hh>
#include <failsafe/exception.hh>
#include <onyx_font/bios_font.hh>
#include <onyx_font/font_registry.hh>

#include "services/service_access.hh"
#include "video/sprite/texture_registry.hh"
#include "video/text/fonts_manager.hh"
#include "video/text/glyph_atlas_access.hh"
#include "video/text/onyx_bitmap_font.hh"

namespace neutrino {
    namespace details {
        font_id register_font(glyph_atlas&& glyphs, glyph_lookup&& lookup,
                              atlas_texture_format format, texture_filter filter) {
            // Both services first: a missing one must not leave an uploaded atlas behind.
            auto& fonts = require_fonts_manager();
            auto& textures = require_texture_registry();

            const font_properties props = glyphs.get_font_properties();
            // The two extracts move out different members of glyphs.
            const atlas_id atlas = textures.add(service_locator::instance().get_renderer(),
                                                glyph_atlas_access::extract_atlas(std::move(glyphs)),
                                                format, filter);
            try {
                return fonts.add({atlas, props, glyph_atlas_access::extract_glyphs(std::move(glyphs)),
                                  std::move(lookup)});
            } catch (...) {
                unregister_atlas(atlas);
                throw;
            }
        }
    }

    namespace {
        font_id register_bitmap_font(const onyx_font::bitmap_font& font, atlas_texture_format format,
                                     texture_filter filter) {
            auto converted = convert_bitmap_font(font);
            return details::register_font(std::move(converted.glyphs), std::move(converted.lookup), format, filter);
        }
    }

    font_id register_font(glyph_atlas&& glyphs, atlas_texture_format format, texture_filter filter) {
        auto lookup = glyph_ids_as_code_points(details::glyph_atlas_access::glyphs(glyphs));
        return details::register_font(std::move(glyphs), std::move(lookup), format, filter);
    }

    void unregister_font(font_id font) {
        erase_if_live(font, maybe_fonts_manager());
    }

    void register_font_decoder(std::unique_ptr <bitmap_font_decoder> dec) {
        ENFORCE(dec != nullptr, "register_font_decoder: the decoder is null");
        onyx_font::font_registry::instance().register_decoder(std::move(dec));
    }

    font_id register_font(std::span <const std::uint8_t> data, std::size_t face,
                          atlas_texture_format format, texture_filter filter) {
        const auto* dec = onyx_font::font_registry::instance().find_bitmap_decoder(data);
        ENFORCE(dec != nullptr, "No bitmap font decoder recognises the data (", data.size(), "bytes)");
        return register_font(data, *dec, face, format, filter);
    }

    font_id register_font(std::span <const std::uint8_t> data, const bitmap_font_decoder& dec, std::size_t face,
                          atlas_texture_format format, texture_filter filter) {
        onyx_font::bitmap_font font;
        try {
            font = dec.load(data, face);
        } catch (const std::exception& e) {
            THROW_RUNTIME("font decoder", dec.name(), "failed on face", face, ":", e.what());
        }
        return register_bitmap_font(font, format, filter);
    }

    font_id register_bios_font(bios_font which, atlas_texture_format format, texture_filter filter) {
        switch (which) {
            case bios_font::cga_8x8:
                return register_bitmap_font(onyx_font::bios_font_8x8(), format, filter);
            case bios_font::ega_8x14:
                return register_bitmap_font(onyx_font::bios_font_8x14(), format, filter);
            case bios_font::vga_8x16:
                return register_bitmap_font(onyx_font::bios_font_8x16(), format, filter);
        }
        THROW_RUNTIME("Unknown BIOS font", static_cast <int>(which));
    }
}
