//
// Created by igor on 27/10/2026.
//

#include <failsafe/enforce.hh>
#include <neutrino/video/image/image_loader.hh>
#include <sdlpp/image/sdl_surface_adapter.hh>
#include <onyx_image/codec.hpp>

namespace neutrino {
    void register_image_decoder(std::unique_ptr <image_decoder> dec) {
        onyx_image::codec_registry::instance().register_decoder(std::move(dec));
    }

    image decode_image(std::span <const std::uint8_t> bytes, const image_decoder& dec) {
        return decode_image(bytes, dec, {});
    }

    image decode_image(std::span <const std::uint8_t> bytes, const image_decoder& dec,
                       const image_decode_options& opts) {
        image result;
        sdlpp::image::sdl_surface_adapter adapter(result);

        if (const auto r = dec.decode(bytes, adapter, opts); !r) {
            THROW_RUNTIME("decoder", dec.name(), "failed:",
                          r.message.empty() ? onyx_image::to_string(r.error) : r.message);
        }
        ENFORCE(static_cast <bool>(result), "decoder", dec.name(), "reported success but produced no image");
        return result;
    }
}
