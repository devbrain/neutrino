//
// Created by igor on 12/07/2026.
//

#pragma once

//
// Internal: decode the *encoded* arms (disk / memory) of a world_image source to an
// owned surface. The already-decoded image_from_surface arm is handled per call site,
// because the policies differ -- the tile bundle borrows the surface (no copy) while a
// sprite set clones it to RGBA for upload -- so it stays out of here.
//

#include <cstdint>
#include <span>
#include <variant>

#include <sdlpp/video/surface.hh>

#include <neutrino/video/image_loader.hh>
#include <neutrino/world/world_common.hh>

namespace neutrino::details {
    [[nodiscard]] inline sdlpp::surface load_encoded_image(const world_image_source& src) {
        if (const auto* mem = std::get_if <image_from_memory>(&src)) {
            return load_image(std::span <const std::uint8_t>(mem->bytes.data(), mem->bytes.size()));
        }
        return load_image(std::get <image_from_disk>(src).source);
    }
}
