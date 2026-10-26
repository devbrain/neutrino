//
// Created by igor on 26/10/2026.
//

#pragma once

/**
 * @file image.hh
 * @brief The engine's CPU-side image vocabulary: @ref image and @ref pixel_format.
 *
 * Engine-owned NAMES for sdlpp's surface types (roadmap P1.7): aliases, so an image passes to
 * sdlpp unchanged. An image is decoded pixels in system memory -- what image_loader.hh returns and
 * what sprite atlases are built from; it becomes a GPU texture when a sheet is registered.
 */

#include <sdlpp/video/surface.hh>
#include <sdlpp/video/pixels.hh>

namespace neutrino {
    /// @brief Decoded pixels in system memory (an SDL surface).
    using image = sdlpp::surface;                    // sdlpp-ok
    /// @brief A pixel layout: @c pixel_format::RGBA8888, ...
    using pixel_format = sdlpp::pixel_format_enum;   // sdlpp-ok
}
