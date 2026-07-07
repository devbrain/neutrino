#include <doctest/doctest.h>

#include <neutrino/world/content_key.hh>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

namespace {
    // Reinterpret a string_view as the byte span content_hash consumes.
    std::span<const std::byte> as_bytes(std::string_view text) noexcept {
        return {reinterpret_cast<const std::byte*>(text.data()), text.size()};
    }
}

TEST_SUITE("neutrino::content_key") {
    using neutrino::content_key;
    using neutrino::content_hash;

    TEST_CASE("content_hash is deterministic") {
        const auto a = content_hash(as_bytes("tileset.png"));
        const auto b = content_hash(as_bytes("tileset.png"));
        CHECK(a == b);
        CHECK(a.hash == b.hash);
        CHECK(a.length == b.length);
    }

    TEST_CASE("content_hash carries the byte length") {
        const std::string_view text = "collection/hero.qoi";
        const auto key = content_hash(as_bytes(text));
        CHECK(key.length == text.size());
    }

    TEST_CASE("distinct contents produce distinct keys") {
        static constexpr std::string_view corpus[] = {
            "", "a", "b", "ab", "ba", "abc", "abd",
            "tileset.png", "tileset.qoi", "tileset.bmp",
            "hero", "hero ", " hero", "HERO", "0", "1", "00",
        };
        std::unordered_set<content_key> seen;
        for (const auto s : corpus) {
            seen.insert(content_hash(as_bytes(s)));
        }
        // Every distinct input maps to a distinct key (no collisions in corpus).
        CHECK(seen.size() == std::size(corpus));
    }

    TEST_CASE("length discriminates inputs that share a hash") {
        // Even if two keys somehow shared a hash, differing lengths keep them
        // unequal — the discriminator the cache relies on.
        const content_key a{0x1122334455667788ULL, 4};
        const content_key b{0x1122334455667788ULL, 5};
        CHECK(a != b);
        CHECK(a.hash == b.hash);
        CHECK(std::hash<content_key>{}(a) != std::hash<content_key>{}(b));
    }

    TEST_CASE("empty input is handled without UB") {
        const std::span<const std::byte> empty{};
        const auto key = content_hash(empty);
        CHECK(key.length == 0);
        CHECK(key == content_hash(empty));   // stable
    }

    TEST_CASE("golden value pins cross-build stability") {
        // A fixed input must always produce this exact 64-bit digest. If the
        // composition ever becomes size_t-width-dependent again, a 32-bit build
        // fails here.
        const auto key = content_hash(as_bytes("neutrino"));
        CHECK(key.hash == 0xE12B6F8F24F63248ULL);
        CHECK(key.length == 8);
    }

    TEST_CASE("content_key is a usable unordered_map key") {
        std::unordered_map<content_key, int> bundles;
        bundles[content_hash(as_bytes("a.png"))] = 1;
        bundles[content_hash(as_bytes("b.png"))] = 2;
        bundles[content_hash(as_bytes("a.png"))] = 3;   // overwrites the first
        CHECK(bundles.size() == 2);
        CHECK(bundles.at(content_hash(as_bytes("a.png"))) == 3);
    }
}
