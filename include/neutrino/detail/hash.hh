//
// Created by igor on 06/07/2026.
//

#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>

namespace neutrino::details {
    /**
     * @brief Boost-style hash combine shared by handle/value hashers.
     */
    constexpr void hash_combine(std::size_t& seed, std::size_t value) noexcept {
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6u) + (seed >> 2u);
    }

    /**
     * @brief Transparent string hasher enabling string_view lookups in
     *        unordered containers keyed by std::string.
     */
    struct transparent_string_hash {
        using is_transparent = void;

        [[nodiscard]] std::size_t operator()(std::string_view value) const noexcept {
            return std::hash <std::string_view>{}(value);
        }
    };
}
