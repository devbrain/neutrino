//
// Created by igor on 06/07/2026.
//

#pragma once

#include <cstdint>
#include <utility>
#include <filesystem>
#include <span>
#include <sdlpp/image/image.hh>
#include <failsafe/exception.hh>


namespace neutrino {
    [[nodiscard]] inline sdlpp::surface load_image(const std::filesystem::path& pth) {
        auto v = sdlpp::image::load(pth);
        if (v) {
            return std::move(*v);
        }
        THROW_RUNTIME(v.error());
    }

    [[nodiscard]] inline sdlpp::surface load_image(std::span<const uint8_t> data) {
        auto v = sdlpp::image::load(data);
        if (v) {
            return std::move(*v);
        }
        THROW_RUNTIME(v.error());
    }
}

