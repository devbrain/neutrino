//
// Created by igor on 06/07/2026.
//

#pragma once

/**
 * @file image_loader.hh
 * @brief Throwing wrappers over sdlpp's image decode that yield a
 *        @c sdlpp::surface directly.
 *
 * The rest of the engine loads images as a value, not an @c expected: these
 * thin adapters call sdlpp's image loader and turn a decode failure into a
 * thrown exception, so callers get a ready surface or an error propagates.
 */

#include <cstdint>
#include <utility>
#include <filesystem>
#include <span>
#include <sdlpp/image/image.hh>
#include <failsafe/exception.hh>


namespace neutrino {
    /**
     * @brief Decode an image file into a surface.
     *
     * @param pth Filesystem path to the image (format inferred by sdlpp).
     * @return The decoded surface (ownership transferred to the caller).
     * @throws std::runtime_error carrying the decoder error if the file cannot
     *         be opened or decoded.
     */
    [[nodiscard]] inline sdlpp::surface load_image(const std::filesystem::path& pth) {
        auto v = sdlpp::image::load(pth);
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
    [[nodiscard]] inline sdlpp::surface load_image(std::span<const uint8_t> data) {
        auto v = sdlpp::image::load(data);
        if (v) {
            return std::move(*v);
        }
        THROW_RUNTIME(v.error());
    }
}

