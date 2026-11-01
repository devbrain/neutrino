//
// Created by igor on 06/07/2026.
//

#pragma once

/**
 * @file image_loader.hh
 * @brief Image loading: built-in formats, custom decoders, and multi-picture files.
 *
 * Loading returns a value, not an @c expected: every function here turns a decode
 * failure into a thrown exception, so callers get a ready @c image or an error propagates.
 *
 * There is one pixel type, @ref image; what differs is how a file is read and whether it
 * holds one picture or many:
 *  - **One picture in a known format** (PNG, GIF, PCX, ...): @ref load_image. The format is
 *    sniffed by the registered decoders.
 *  - **One picture in a custom format**: implement an @ref image_decoder and either
 *    register it (@ref register_image_decoder, then @ref load_image finds it) or call it
 *    explicitly with @ref decode_image -- the way to go for headerless formats or ones that
 *    need context, such as an external palette. A game's own parser can also simply build
 *    the @ref image with @ref image_builder.
 *  - **Many numbered pictures in one file** (sprite banks, animation frames, tile sets): the
 *    file's parser returns an @ref image_bank, one @ref image per picture plus the file's
 *    own number for it. Packing those pictures into atlas pages is the atlas builder's job,
 *    not the parser's.
 */

#include <cstdint>
#include <utility>
#include <filesystem>
#include <functional>
#include <vector>
#include <memory>
#include <optional>
#include <span>

#include <neutrino/neutrino_export.h>
#include <sdlpp/image/image.hh>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/palette.hh>

#include <failsafe/exception.hh>
#include <neutrino/video/image/image.hh>
#include <neutrino/video/image/image_bank.hh>
#include <neutrino/video/atlas/atlas_packer.hh>

namespace neutrino {
    /**
     * @brief Decode an image file into a surface.
     *
     * @param pth Filesystem path to the image (format inferred by sdlpp).
     * @return The decoded surface (ownership transferred to the caller).
     * @throws std::runtime_error carrying the decoder error if the file cannot
     *         be opened or decoded.
     */
    [[nodiscard]] inline image load_image(const std::filesystem::path& pth) {
        auto v = sdlpp::image::load(pth); // sdlpp-ok: implementation
        if (v) {
            return std::move(*v);
        }
        THROW_RUNTIME(v.error());
    }

    /**
     * @brief Decode an image from an in-memory encoded byte buffer into a surface.
     *
     * The counterpart to the path overload for images already resident in memory
     * (embedded assets, network/archive bytes); the format is sniffed from the
     * buffer contents.
     *
     * @param data Span over the encoded image bytes.
     * @return The decoded surface (ownership transferred to the caller).
     * @throws std::runtime_error carrying the decoder error if the bytes cannot
     *         be decoded.
     */
    [[nodiscard]] inline image load_image(std::span <const uint8_t> data) {
        auto v = sdlpp::image::load(data); // sdlpp-ok: implementation
        if (v) {
            return std::move(*v);
        }
        THROW_RUNTIME(v.error());
    }

    // ------------------------------------------------------------------------------------
    // Single images: custom decoders
    // ------------------------------------------------------------------------------------

    /**
     * @brief A single-image format plugin (onyx_image's decoder interface).
     *
     * Implement @c name(), @c extensions(), @c sniff() and @c decode(bytes, sink, opts).
     * A decoder may carry configuration in its members (an external palette, fixed
     * dimensions for a headerless format), which is why @ref decode_image takes an instance.
     */
    using image_decoder = onyx_image::decoder;

    /// @brief Decode limits and requested colour output, passed to the decoder. Its packing
    ///        fields are ignored: placing pictures in an atlas is the atlas builder's job.
    using image_decode_options = onyx_image::decode_options;

    /// @brief Success or a decode error with a message; what decoders return.
    using image_decode_result = onyx_image::decode_result;

    /**
     * @brief Make @p dec available to @ref load_image (by content sniffing and by extension).
     *
     * Use it for self-describing formats, those @c sniff() can recognise. Registered decoders
     * live for the rest of the program. Formats that need context (a palette, dimensions)
     * cannot be sniffed: call them through @ref decode_image instead.
     *
     * @param dec The decoder; must not be null.
     */
    NEUTRINO_EXPORT void register_image_decoder(std::unique_ptr <image_decoder> dec);

    /**
     * @brief Decode @p bytes with the given decoder, bypassing the registry and sniffing.
     *
     * @param bytes The encoded image.
     * @param dec   The decoder to use, possibly configured (e.g. with an external palette).
     * @param opts  Size limits and colour output; defaults when omitted.
     * @return The decoded image.
     * @throws std::runtime_error carrying the decoder's message if decoding fails.
     */
    [[nodiscard]] NEUTRINO_EXPORT image decode_image(std::span <const std::uint8_t> bytes, const image_decoder& dec);
    /// @copydoc decode_image(std::span<const std::uint8_t>, const image_decoder&)
    [[nodiscard]] NEUTRINO_EXPORT image decode_image(std::span <const std::uint8_t> bytes, const image_decoder& dec,
                                                     const image_decode_options& opts);
}
