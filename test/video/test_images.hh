//
// Shared image fixtures for video/sprite/world tests. Consolidates helpers that were
// copy-pasted (and drifting) across many test files. Construction-validity guards
// (REQUIRE) stay here; the values tests actually assert on (sizes, ids) are passed in.
//
#pragma once

#include <doctest/doctest.h>

#include <neutrino/world/world_common.hh>

#include <sdlpp/video/surface.hh>

#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace neutrino::test {
    /// @brief A world_image whose bytes are a BMP of a blank w*h RGBA surface (decodable).
    inline world_image bmp_image(unsigned w, unsigned h) {
        auto surface = sdlpp::surface::create_rgb(
            static_cast <int>(w), static_cast <int>(h), sdlpp::pixel_format_enum::RGBA8888);
        REQUIRE(surface.has_value());
        auto bytes = sdlpp::save_bmp(*surface);
        REQUIRE(bytes.has_value());

        world_image img;
        img.width = w;
        img.height = h;
        img.source = image_from_memory{std::move(*bytes)};
        return img;
    }

    /// @brief Square convenience overload.
    inline world_image bmp_image(unsigned size) { return bmp_image(size, size); }

    /// @brief A world_image carrying caller-supplied encoded bytes (no decode guarantee).
    inline world_image mem_image(unsigned w, unsigned h, std::vector <std::uint8_t> bytes) {
        world_image img;
        img.width = w;
        img.height = h;
        img.source = image_from_memory{std::move(bytes)};
        return img;
    }

    /// @brief A world_image backed by an already-decoded surface, with an optional producer id.
    inline world_image surface_image(unsigned w, unsigned h,
                                     std::optional <std::uint64_t> identity = std::nullopt) {
        auto surface = sdlpp::surface::create_rgb(
            static_cast <int>(w), static_cast <int>(h), sdlpp::pixel_format_enum::RGBA8888);
        REQUIRE(surface.has_value());

        world_image img;
        img.width = w;
        img.height = h;
        img.source = image_from_surface{
            std::make_shared <const sdlpp::surface>(std::move(*surface)), identity};
        return img;
    }

    /// @brief A size*size surface of @p format whose raw bytes are a fixed 1,2,3,4 pattern, so
    ///        two different formats hold byte-identical pixels but decode to different colours.
    inline world_image patterned_surface(sdlpp::pixel_format_enum format, unsigned size = 16) {
        auto surface = sdlpp::surface::create_rgb(
            static_cast <int>(size), static_cast <int>(size), format);
        REQUIRE(surface.has_value());
        SDL_Surface* raw = surface->get();
        REQUIRE(raw != nullptr);
        const bool must_lock = SDL_MUSTLOCK(raw);
        if (must_lock) {
            REQUIRE(SDL_LockSurface(raw));
        }
        auto* bytes = static_cast <std::uint8_t*>(raw->pixels);
        const std::size_t total = static_cast <std::size_t>(raw->pitch) * static_cast <std::size_t>(raw->h);
        for (std::size_t i = 0; i < total; ++i) {
            bytes[i] = static_cast <std::uint8_t>((i % 4) + 1);
        }
        if (must_lock) {
            SDL_UnlockSurface(raw);
        }

        world_image img;
        img.width = size;
        img.height = size;
        img.source = image_from_surface{
            std::make_shared <const sdlpp::surface>(std::move(*surface)), std::nullopt};
        return img;
    }
}
