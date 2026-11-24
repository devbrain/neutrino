//
// Created by igor on 24/11/2026.
//

#include <doctest/doctest.h>

#include <neutrino/detail/generic_handle.hh>
#include "detail/generic_handle_factory.hh"

#include <neutrino/video/atlas/atlas_id.hh>
#include <neutrino/video/sprite/sprite_sheet.hh>
#include <neutrino/video/sprite/sprite_state.hh>
#include <neutrino/video/text/font.hh>

#include <concepts>
#include <cstdint>
#include <functional>
#include <sstream>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

namespace {
    struct first_tag;
    struct second_tag;
    using first_id = neutrino::detail::generic_handle <first_tag>;
    using second_id = neutrino::detail::generic_handle <second_tag>;
}

TEST_SUITE("neutrino::detail::generic_handle") {
    using neutrino::detail::id_access;
    using neutrino::detail::make_handle;

    TEST_CASE("a default handle is invalid; a minted one is valid") {
        const first_id none;
        CHECK_FALSE(none.valid());
        CHECK(make_handle <first_id>().valid());
        CHECK(id_access::make <first_id>(7).valid());

        std::ostringstream os;
        os << none << ' ' << id_access::make <first_id>(7);
        CHECK(os.str() == "<invalid> 7");
    }

    TEST_CASE("make_handle never repeats, and each kind counts on its own") {
        const auto a = make_handle <first_id>();
        const auto b = make_handle <first_id>();
        CHECK(a != b);
        CHECK(a < b);

        // A kind minted for the first time starts at the bottom, whatever the others did.
        struct third_tag;
        using third_id = neutrino::detail::generic_handle <third_tag>;
        CHECK(make_handle <third_id>() == id_access::make <third_id>(0));
    }

    TEST_CASE("handles key unordered containers, hashing as their base does") {
        const auto a = id_access::make <first_id>(1);
        const auto b = id_access::make <first_id>(2);

        std::unordered_set <first_id> set{a, b, a};
        CHECK(set.size() == 2);
        CHECK(set.contains(b));

        std::unordered_map <first_id, int> map{{a, 10}};
        CHECK(map.at(a) == 10);
        CHECK_FALSE(map.contains(b));

        using base = neutrino::detail::id_strong_type <first_tag>;
        CHECK(std::hash <first_id>{}(a) == std::hash <base>{}(a));
    }

    TEST_CASE("handles of different kinds do not mix, and only id_access mints them") {
        static_assert(!std::equality_comparable_with <first_id, second_id>);
        static_assert(!std::is_convertible_v <first_id, second_id>);
        static_assert(!std::is_constructible_v <first_id, std::uint32_t>);
        static_assert(!std::is_convertible_v <std::uint32_t, first_id>);
        static_assert(std::is_trivially_copyable_v <first_id>);
        CHECK(true);
    }

    TEST_CASE("the engine's plain ids are generic handles") {
        static_assert(std::is_same_v <neutrino::atlas_id,
                                      neutrino::detail::generic_handle <neutrino::detail::atlas_id_tag>>);
        static_assert(std::is_same_v <neutrino::font_id,
                                      neutrino::detail::generic_handle <neutrino::detail::font_id_tag>>);
        static_assert(std::is_same_v <neutrino::sprite_sheet_id,
                                      neutrino::detail::generic_handle <neutrino::detail::sprite_sheet_id_tag>>);
        static_assert(std::is_same_v <neutrino::sprite_animation_id,
                                      neutrino::detail::generic_handle <neutrino::detail::sprite_animation_id_tag>>);
        static_assert(std::is_same_v <neutrino::sprite_state_id,
                                      neutrino::detail::generic_handle <neutrino::detail::sprite_state_id_tag>>);
        static_assert(!std::equality_comparable_with <neutrino::atlas_id, neutrino::font_id>);

        std::unordered_set <neutrino::sprite_state_id> states{id_access::make <neutrino::sprite_state_id>(3)};
        CHECK(states.size() == 1);
    }
}
