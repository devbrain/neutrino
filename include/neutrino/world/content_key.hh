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
    /**
     * @brief Content-addressed identity of a byte buffer: a hash paired with its length.
     *
     * Used as a map/set key to dedup identical byte content (e.g. decoded images in
     * the resource cache). Construct one with @ref content_hash. Equality and ordering
     * compare both fields, so distinct-length inputs never collide.
     */
    struct content_key {
        std::uint64_t hash;   ///< Platform-stable 64-bit digest of the bytes (@ref details::hash_bytes).
        std::uint64_t length; ///< Byte length; collision discriminator so unequal-length inputs never compare equal.
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
