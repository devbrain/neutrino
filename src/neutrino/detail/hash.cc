//
// Created by igor on 06/07/2026.
//

#include <SDL3/SDL_stdinc.h>
#include <neutrino/detail/hash.hh>

namespace neutrino::details {
    std::uint64_t hash_bytes(const void* data, std::size_t length) noexcept {
        static constexpr Uint32 SEED_A = 1;
        static constexpr Uint32 SEED_B = 2;
        // Two independent seeded murmur3_32 concatenated into a fixed 64-bit
        // digest. Always both seeds, always 64 bits, regardless of size_t width,
        // so the value is identical on 32- and 64-bit builds.
        const auto a = SDL_murmur3_32(data, length, SEED_A);
        const auto b = SDL_murmur3_32(data, length, SEED_B);
        return (static_cast<std::uint64_t>(a) << 32) | static_cast<std::uint64_t>(b);
    }
}
