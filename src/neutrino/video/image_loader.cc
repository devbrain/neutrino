//
// Created by igor on 27/10/2026.
//

#include <neutrino/video/image_loader.hh>
#include <onyx_image/codec.hpp>

namespace neutrino {

    void register_image_decoder(std::unique_ptr<image_decoder> dec) {
        onyx_image::codec_registry::instance().register_decoder(std::move(dec));
    }

    atlas_sink::~atlas_sink() = default;

    // ------------------------------------------------------------------------------------------------------


    // ------------------------------------------------------------------------------------------------------
    void decode_image_atlas(std::span <const std::uint8_t> bytes, const atlas_decoder& dec, atlas_sink& sink,
                            const image_decode_options& opts) {
        auto status = dec(bytes, sink, opts);
        if (!status.ok) {
            THROW_RUNTIME("Failed to decode atlas: ", status.message);
        }
    }
}
