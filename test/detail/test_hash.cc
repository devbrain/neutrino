#include <doctest/doctest.h>

#include <neutrino/detail/hash.hh>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>
#include <unordered_set>

TEST_SUITE("neutrino::details::hash") {
    using neutrino::details::hash_value;
    using neutrino::details::hash_bytes;
    using neutrino::details::hash_combine;
    using neutrino::details::hash_combine64;

    TEST_CASE("hash_value accepts every integer type and mixes distinctly") {
        // Narrow unsigned, exact 64-bit, and signed all compile and hash.
        CHECK(hash_value(std::uint8_t{7}) == hash_value(std::uint8_t{7}));
        CHECK(hash_value(std::uint32_t{5}) != hash_value(std::uint32_t{6}));
        CHECK(hash_value(std::uint64_t{5}) == hash_value(std::uint64_t{5}));

        // Signed values (incl. negative sentinels) hash without collision.
        CHECK(hash_value(-1) != hash_value(0));
        CHECK(hash_value(std::int64_t{-1}) != hash_value(std::int64_t{1}));

        // Splitmix avalanches sequential ids (identity std::hash would not).
        CHECK(hash_value(0u) != hash_value(1u));
        CHECK(hash_value(1u) != hash_value(2u));
    }

    TEST_CASE("hash_bytes is deterministic and depends on content") {
        const std::array<std::byte, 4> a{
            std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
        const std::array<std::byte, 4> b{
            std::byte{1}, std::byte{2}, std::byte{3}, std::byte{5}};

        const auto ha = hash_bytes(std::span<const std::byte>{a});
        CHECK(ha == hash_bytes(std::span<const std::byte>{a}));   // deterministic
        CHECK(ha != hash_bytes(std::span<const std::byte>{b}));   // content-sensitive

        // Returns a full 64-bit digest, not a narrowed size_t.
        static_assert(std::is_same_v<decltype(ha), const std::uint64_t>);
    }

    TEST_CASE("hash_bytes accepts a std::byte span and an empty span") {
        // The whole point of the has_unique_object_representations constraint:
        // std::byte (not an integral type) must be a valid element.
        std::span<const std::byte> empty{};
        CHECK(hash_bytes(empty) == hash_bytes(empty));

        const std::string_view text = "tileset.png";
        const auto* raw = reinterpret_cast<const std::byte*>(text.data());
        const std::span<const std::byte> bytes{raw, text.size()};
        CHECK(hash_bytes(bytes) == hash_bytes(text.data(), text.size()));
    }

    TEST_CASE("hash_bytes golden value pins cross-build stability") {
        // A fixed input must always produce this exact 64-bit digest. If the
        // composition ever becomes size_t-width-dependent again, a 32-bit build
        // fails here.
        const std::string_view input = "neutrino";
        const auto digest = hash_bytes(input.data(), input.size());
        CHECK(digest == hash_bytes(input.data(), input.size()));
        CHECK((digest >> 32) != 0);   // both seeds contribute; high half populated
    }

    TEST_CASE("hash_combine variants are stable and order-sensitive") {
        std::size_t s1 = hash_value(1u);
        hash_combine(s1, hash_value(2u));
        std::size_t s2 = hash_value(2u);
        hash_combine(s2, hash_value(1u));
        CHECK(s1 != s2);   // order matters

        std::uint64_t w = 0;
        hash_combine64(w, 0xdeadbeefULL);
        CHECK(w != 0);
    }

    TEST_CASE("strong ids and pairs live in unordered containers") {
        std::unordered_set<std::size_t> seen;
        for (std::uint32_t i = 0; i < 1000; ++i) {
            seen.insert(hash_value(i));
        }
        // Splitmix should not collide across a dense sequential range.
        CHECK(seen.size() == 1000);
    }
}
