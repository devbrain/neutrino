#include <doctest/doctest.h>

#include <neutrino/detail/enum.hh>
#include <neutrino/scene/base_scene.hh>

#include <bitset>
#include <cstdint>
#include <type_traits>

// Test enumerations live in namespace neutrino: the operators are found by
// argument-dependent lookup there, and the promotion macro must be used there.
namespace neutrino {
    enum class enum_test_u8 : std::uint8_t {
        none = 0,
        a = 1u << 0u,
        b = 1u << 1u,
        c = 1u << 2u,
        high = 1u << 7u
    };
    d_NEUTRINO_PROMOTE_ENUM(enum_test_u8);

    enum class enum_test_u16 : std::uint16_t { none = 0, a = 1u << 0u, high = 1u << 15u };
    d_NEUTRINO_PROMOTE_ENUM(enum_test_u16);

    enum class enum_test_u32 : std::uint32_t { none = 0, a = 1u << 0u, high = 1u << 31u };
    d_NEUTRINO_PROMOTE_ENUM(enum_test_u32);

    enum class enum_test_u64 : std::uint64_t { none = 0, a = 1u << 0u, high = std::uint64_t{1} << 63u };
    d_NEUTRINO_PROMOTE_ENUM(enum_test_u64);

    // Unscoped, unsigned, promoted.
    enum enum_test_unscoped : std::uint8_t { enum_test_unscoped_a = 1, enum_test_unscoped_b = 2 };
    d_NEUTRINO_PROMOTE_ENUM(enum_test_unscoped);

    // Promoted, but signed: must not satisfy flags_enum.
    enum class enum_test_signed : std::int8_t { a = 1, b = 2 };
    d_NEUTRINO_PROMOTE_ENUM(enum_test_signed);

    // Unsigned, not promoted.
    enum class enum_test_plain : std::uint8_t { a = 1, b = 2 };

    // Not promoted, with its own operator (like sprite_flip): must keep using it.
    enum class enum_test_own : std::uint8_t { a = 1, b = 2, marker = 0x80 };
    constexpr enum_test_own operator|(enum_test_own, enum_test_own) {
        return enum_test_own::marker;
    }

    // Unscoped, unsigned, not promoted: must keep the built-in int operators.
    enum enum_test_unscoped_plain : std::uint8_t { enum_test_plain_a = 1, enum_test_plain_b = 2 };
}

// An enumeration outside namespace neutrino, promoted from inside it.
namespace enum_test_outside {
    enum class bits : std::uint8_t { a = 1, b = 2 };
}
namespace neutrino {
    d_NEUTRINO_PROMOTE_ENUM(enum_test_outside::bits);
}

namespace {
    template<typename T>
    concept has_or = requires(T x, T y) { x | y; };

    template<typename T>
    concept has_or_assign = requires(T& x, T y) { x |= y; };

    template<typename T>
    concept has_not = requires(T x) { ~x; };

    template<typename T>
    constexpr auto raw(T v) {
        return static_cast <std::underlying_type_t <T>>(v);
    }
}

TEST_SUITE("neutrino::detail::enum") {
    using neutrino::enum_test_u8;
    using neutrino::enum_test_u16;
    using neutrino::enum_test_u32;
    using neutrino::enum_test_u64;
    using neutrino::flags_enum;

    TEST_CASE("flags_enum accepts promoted unsigned enumerations only") {
        static_assert(flags_enum <enum_test_u8>);
        static_assert(flags_enum <enum_test_u16>);
        static_assert(flags_enum <enum_test_u32>);
        static_assert(flags_enum <enum_test_u64>);
        static_assert(flags_enum <neutrino::enum_test_unscoped>);
        static_assert(flags_enum <enum_test_outside::bits>);

        static_assert(!flags_enum <neutrino::enum_test_signed>);   // promoted, but signed
        static_assert(!flags_enum <neutrino::enum_test_plain>);    // not promoted
        static_assert(!flags_enum <neutrino::enum_test_own>);
        static_assert(!flags_enum <neutrino::enum_test_unscoped_plain>);
        static_assert(!flags_enum <int>);
        static_assert(!flags_enum <unsigned>);
        static_assert(!flags_enum <std::bitset <8>>);

        static_assert(neutrino::is_flag <enum_test_u8>::value);
        static_assert(neutrino::is_flag <neutrino::enum_test_signed>::value);
        static_assert(!neutrino::is_flag <neutrino::enum_test_plain>::value);
    }

    TEST_CASE("operators exist only for flags_enum types") {
        static_assert(has_or <enum_test_u8>);
        static_assert(has_or_assign <enum_test_u8>);
        static_assert(has_not <enum_test_u8>);

        static_assert(!has_or <neutrino::enum_test_plain>);
        static_assert(!has_or_assign <neutrino::enum_test_plain>);
        static_assert(!has_not <neutrino::enum_test_plain>);

        static_assert(!has_or <neutrino::enum_test_signed>);
        static_assert(!has_or_assign <neutrino::enum_test_signed>);

        // An enumeration outside namespace neutrino is promoted, but ADL does not reach
        // the operators in namespace neutrino.
        static_assert(!has_or <enum_test_outside::bits>);
    }

    TEST_CASE("result types are the enumeration itself") {
        constexpr auto a = enum_test_u8::a;
        constexpr auto b = enum_test_u8::b;
        static_assert(std::is_same_v <decltype(a | b), enum_test_u8>);
        static_assert(std::is_same_v <decltype(a & b), enum_test_u8>);
        static_assert(std::is_same_v <decltype(a ^ b), enum_test_u8>);
        static_assert(std::is_same_v <decltype(~a), enum_test_u8>);

        auto x = a;
        static_assert(std::is_same_v <decltype(x |= b), enum_test_u8&>);
        static_assert(std::is_same_v <decltype(x &= b), enum_test_u8&>);
        static_assert(std::is_same_v <decltype(x ^= b), enum_test_u8&>);
    }

    TEST_CASE("binary operators combine the bits") {
        using e = enum_test_u8;
        static_assert(raw(e::a | e::b) == 0b011);
        static_assert(raw(e::a | e::a) == 0b001);
        static_assert(raw((e::a | e::b) & e::b) == 0b010);
        static_assert(raw(e::a & e::b) == 0);
        static_assert(raw((e::a | e::b) ^ (e::b | e::c)) == 0b101);
        static_assert(raw(e::a ^ e::a) == 0);
        static_assert((e::none | e::a) == e::a);
        static_assert((e::a & e::none) == e::none);

        CHECK(raw(e::a | e::b | e::c) == 0b111);
        CHECK(((e::a | e::b) & (e::b | e::c)) == e::b);
    }

    TEST_CASE("complement covers the full width of the underlying type") {
        static_assert(raw(~enum_test_u8::none) == 0xFFu);
        static_assert(raw(~enum_test_u8::a) == 0xFEu);
        static_assert(raw(~enum_test_u16::none) == 0xFFFFu);
        static_assert(raw(~enum_test_u32::none) == 0xFFFFFFFFu);
        static_assert(raw(~enum_test_u64::none) == ~std::uint64_t{0});
        static_assert(~~enum_test_u8::c == enum_test_u8::c);

        // a & ~b clears exactly the flags of b.
        using e = enum_test_u8;
        static_assert(((e::a | e::b | e::c) & ~e::b) == (e::a | e::c));
        static_assert(((e::a | e::c) & ~e::b) == (e::a | e::c));
    }

    TEST_CASE("the highest bit of every width survives") {
        static_assert(raw(enum_test_u8::high | enum_test_u8::a) == 0x81u);
        static_assert(raw(enum_test_u16::high | enum_test_u16::a) == 0x8001u);
        static_assert(raw(enum_test_u32::high | enum_test_u32::a) == 0x80000001u);
        static_assert(raw(enum_test_u64::high | enum_test_u64::a) == ((std::uint64_t{1} << 63u) | 1u));
        static_assert(raw(~enum_test_u64::high) == ~(std::uint64_t{1} << 63u));
    }

    TEST_CASE("compound assignment modifies the left operand and returns it") {
        using e = enum_test_u8;
        auto x = e::a;

        auto& r1 = (x |= e::b);
        CHECK(&r1 == &x);
        CHECK(x == (e::a | e::b));

        auto& r2 = (x &= e::b);
        CHECK(&r2 == &x);
        CHECK(x == e::b);

        auto& r3 = (x ^= (e::b | e::c));
        CHECK(&r3 == &x);
        CHECK(x == e::c);

        x ^= e::c;
        CHECK(x == e::none);
    }

    TEST_CASE("compound assignment is constexpr and chains") {
        constexpr auto build = [] {
            auto x = enum_test_u8::none;
            x |= enum_test_u8::a;
            x |= enum_test_u8::c;
            x &= ~enum_test_u8::a;
            (x |= enum_test_u8::b) ^= enum_test_u8::c;
            return x;
        };
        static_assert(build() == enum_test_u8::b);
        CHECK(build() == enum_test_u8::b);
    }

    TEST_CASE("has requires every flag of the mask") {
        using e = enum_test_u8;
        constexpr auto set = e::a | e::c;

        static_assert(neutrino::has(set, e::a));
        static_assert(neutrino::has(set, e::c));
        static_assert(neutrino::has(set, e::a | e::c));
        static_assert(!neutrino::has(set, e::b));
        static_assert(!neutrino::has(set, e::a | e::b));     // partial match is not enough
        static_assert(neutrino::has(e::high | e::a, e::high));

        // The empty mask is contained in every set.
        static_assert(neutrino::has(set, e::none));
        static_assert(neutrino::has(e::none, e::none));
        static_assert(!neutrino::has(e::none, e::a));

        CHECK(neutrino::has(set, e::c));
        CHECK_FALSE(neutrino::has(set, e::b));
    }

    TEST_CASE("has works for every underlying width") {
        static_assert(neutrino::has(enum_test_u16::high | enum_test_u16::a, enum_test_u16::high));
        static_assert(neutrino::has(enum_test_u32::high | enum_test_u32::a, enum_test_u32::high));
        static_assert(neutrino::has(enum_test_u64::high | enum_test_u64::a, enum_test_u64::high));
        static_assert(!neutrino::has(enum_test_u64::a, enum_test_u64::high));
    }

    TEST_CASE("unscoped promoted enumerations use the flag operators") {
        using neutrino::enum_test_unscoped;
        constexpr auto v = neutrino::enum_test_unscoped_a | neutrino::enum_test_unscoped_b;
        static_assert(std::is_same_v <decltype(v), const enum_test_unscoped>);
        static_assert(raw(v) == 3);
        static_assert(neutrino::has(v, neutrino::enum_test_unscoped_b));
    }

    TEST_CASE("types that are not flags_enum keep their own operators") {
        // A non-promoted enumeration with its own operator still uses it.
        static_assert((neutrino::enum_test_own::a | neutrino::enum_test_own::b) == neutrino::enum_test_own::marker);

        // A non-promoted unscoped enumeration keeps the built-in int operators.
        constexpr auto v = neutrino::enum_test_plain_a | neutrino::enum_test_plain_b;
        static_assert(std::is_same_v <decltype(v), const int>);
        static_assert(v == 3);

        // Integers and class types are untouched.
        static_assert((1 | 2) == 3);
        CHECK((std::bitset <4>(1) | std::bitset <4>(2)) == std::bitset <4>(3));
    }

    TEST_CASE("an enumeration outside namespace neutrino needs using-declarations") {
        using neutrino::operator|;
        using neutrino::operator|=;
        using neutrino::operator~;
        using enum_test_outside::bits;

        auto x = bits::a | bits::b;
        CHECK(raw(x) == 3);
        x |= bits::a;
        CHECK(raw(x) == 3);
        CHECK(raw(~bits::a) == 0xFEu);
        CHECK(neutrino::has(x, bits::b));
    }

    TEST_CASE("scene_flags is a flags_enum") {
        using neutrino::scene_flags;
        static_assert(flags_enum <scene_flags>);
        static_assert(std::is_same_v <std::underlying_type_t <scene_flags>, std::uint8_t>);
        static_assert(raw(scene_flags::none) == 0);
        static_assert((scene_flags::hides_below & scene_flags::pauses_below) == scene_flags::none);

        constexpr auto both = scene_flags::hides_below | scene_flags::pauses_below;
        static_assert(neutrino::has(both, scene_flags::hides_below));
        static_assert(neutrino::has(both, scene_flags::pauses_below));
        static_assert(!neutrino::has(scene_flags::hides_below, scene_flags::pauses_below));
        static_assert((both & ~scene_flags::hides_below) == scene_flags::pauses_below);
    }
}
