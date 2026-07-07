//
// Created by igor on 06/07/2026.
//

#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

#include <neutrino/detail/hash.hh>

namespace neutrino {
    struct content_key {
        std::uint64_t hash;
        std::uint64_t length;
        std::strong_ordering operator<=>(const content_key&) const = default;
    };

    /**
     * @brief Canonical content-addressed identity of a byte buffer.
     *
     * Pairs the platform-stable 64-bit @ref details::hash_bytes digest with the
     * byte length as a cheap collision discriminator: two inputs of differing
     * length can never compare equal even under a hash collision. The value is
     * identical on 32- and 64-bit builds, so it is safe to persist or share
     * across platforms as a cache key.
     */
    [[nodiscard]] inline content_key content_hash(std::span<const std::byte> bytes) noexcept {
        return content_key{details::hash_bytes(bytes), bytes.size()};
    }
}

template<>
struct std::hash <neutrino::content_key> {
    [[nodiscard]] std::size_t operator()(const neutrino::content_key& key) const noexcept {
        std::uint64_t seed = key.hash;
        neutrino::details::hash_combine64(seed, key.length);
        return static_cast <std::size_t>(seed);
    }
};
