//
// Created by igor on 06/07/2026.
//

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

#include <neutrino/neutrino_export.h>

namespace neutrino::details {
    /**
     * @brief Terminal hash for a single integer value.
     *
     * A splitmix64 finalizer: gives good avalanche even for dense sequential
     * keys (as minted by the resource registries), where a plain identity
     * std::hash would leave the high bits unmixed. Computed in fixed 64-bit and
     * narrowed to std::size_t.
     *
     * Accepts any integer type. Signed values convert to std::uint64_t by the
     * usual modular rule, which is injective over the signed range, so hashing
     * stays collision-free; the explicit cast also documents intent under
     * sign-conversion warnings. Enums are excluded by @c std::integral and
     * should be cast to their underlying type at the call site.
     */
    template <std::integral T>
    [[nodiscard]] constexpr std::size_t hash_value(T value) noexcept {
        auto x = static_cast <std::uint64_t>(value);
        x ^= x >> 30u;
        x *= 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 27u;
        x *= 0x94d049bb133111ebULL;
        x ^= x >> 31u;
        return static_cast <std::size_t>(x);
    }

    /**
     * @brief Content hash of a raw byte buffer.
     *
     * Composes two seeded SDL_murmur3_32 into a full 64-bit digest. The result
     * is a fixed 64-bit value computed identically on every platform (never
     * narrowed to std::size_t), so it is safe as a content-addressed cache key
     * that may be persisted or shared across builds. This is a byte digest, not
     * the scalar @ref hash_value finalizer, and the two do not agree.
     */
    NEUTRINO_EXPORT [[nodiscard]] std::uint64_t hash_bytes(const void* data, std::size_t length) noexcept;

    /**
     * @brief Content hash of a typed span, hashed over its object bytes.
     *
     * Constrained to element types with a unique object representation (integers,
     * std::byte, unsigned char) so the byte image is well-defined and
     * deterministic; float and padded aggregate types are rejected because equal
     * values can have differing byte images. Note the byte image of multi-byte
     * elements is endianness-dependent — for cross-platform-stable keys hash a
     * std::byte / unsigned char stream, not a span of wider integers.
     */
    template <typename T>
        requires std::has_unique_object_representations_v<std::remove_cv_t<T>>
    [[nodiscard]] std::uint64_t hash_bytes(std::span<T> material) noexcept {
        return hash_bytes(material.data(), material.size() * sizeof(T));
    }

    /**
     * @brief Fixed-width 64-bit hash combine.
     *
     * Always mixes in 64-bit arithmetic, so the result is identical on every
     * platform regardless of std::size_t width. Use this for hash values that
     * must be stable across builds or persisted to disk (content-addressed
     * cache keys); never route those through the std::size_t overload, which is
     * 32-bit on 32-bit targets.
     */
    constexpr void hash_combine64(std::uint64_t& seed, std::uint64_t value) noexcept {
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6u) + (seed >> 2u);
    }

    /**
     * @brief Boost-style hash combine for in-process std::hash bucketing.
     *
     * The golden-ratio constant is chosen to match std::size_t so a 32-bit
     * size_t does not silently truncate the 64-bit constant down to a
     * meaningless value. Suitable for hash-table bucketing only; for stable or
     * persisted keys use @ref hash_combine64.
     */
    constexpr void hash_combine(std::size_t& seed, std::size_t value) noexcept {
        if constexpr (sizeof(std::size_t) >= 8) {
            seed ^= value + static_cast <std::size_t>(0x9e3779b97f4a7c15ULL) + (seed << 6u) + (seed >> 2u);
        } else {
            seed ^= value + static_cast <std::size_t>(0x9e3779b9UL) + (seed << 6u) + (seed >> 2u);
        }
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
