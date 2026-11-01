//
// Created by igor on 26/10/2026.
//

#pragma once

/**
 * @file image.hh
 * @brief The engine's CPU-side image vocabulary: @ref image and @ref pixel_format.
 *
 * Engine-owned NAMES for sdlpp's surface types: aliases, so an image passes to
 * sdlpp unchanged. An image is decoded pixels in system memory -- what image_loader.hh returns and
 * what sprite atlases are built from; it becomes a GPU texture when a sheet is registered.
 */
#include <filesystem>
#include <failsafe/enforce.hh>

#include <sdlpp/video/surface.hh>
#include <sdlpp/video/pixels.hh>

namespace neutrino {
    /// @brief Decoded pixels in system memory (an SDL surface).
    using image = sdlpp::surface;                    // sdlpp-ok
    /// @brief A pixel layout: @c pixel_format::RGBA8888, ...
    using pixel_format = sdlpp::pixel_format_enum;   // sdlpp-ok

    /**
     * @brief Write @p img to @p pth as a BMP file -- e.g. a capture_frame() screenshot.
     * @throws std::runtime_error carrying the error if the file cannot be written.
     */
    inline void save_bmp(const image& img, const std::filesystem::path& pth) {
        auto v = sdlpp::save_bmp(img, pth.string()); // sdlpp-ok: implementation
        if (!v) {
            THROW_RUNTIME(v.error());
        }
    }
}
