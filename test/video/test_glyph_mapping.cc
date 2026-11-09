//
// Created by igor on 09/11/2026.
//

#include <cstdint>
#include <limits>
#include <stdexcept>

#include <doctest/doctest.h>

#include <neutrino/video/text/glyph_mapping.hh>

using neutrino::glyph_mapping;

TEST_SUITE("neutrino::glyph_mapping") {
    TEST_CASE("an empty mapping finds nothing") {
        const glyph_mapping m;
        CHECK_FALSE(m.find(0));
        CHECK_FALSE(m.find(U'A'));
        CHECK_FALSE(m.find(glyph_mapping::max_code_point));
    }

    TEST_CASE("one range maps [begin, end)") {
        glyph_mapping m;
        m.bind(10, 20, 1);
        CHECK(m.find(10) == 1u);
        CHECK(m.find(11) == 2u);
        CHECK(m.find(19) == 10u);
        CHECK_FALSE(m.find(20));
        CHECK_FALSE(m.find(9));
        CHECK_FALSE(m.find(0));
        CHECK_FALSE(m.find(100));
    }

    TEST_CASE("separate ranges, bound in either order") {
        glyph_mapping m;
        SUBCASE("low first") {
            m.bind(10, 20, 1);
            m.bind(30, 40, 100);
        }
        SUBCASE("high first") {
            m.bind(30, 40, 100);
            m.bind(10, 20, 1);
        }
        CHECK(m.find(10) == 1u);
        CHECK(m.find(19) == 10u);
        CHECK_FALSE(m.find(20));
        CHECK_FALSE(m.find(25));
        CHECK_FALSE(m.find(29));
        CHECK(m.find(30) == 100u);
        CHECK(m.find(31) == 101u);
        CHECK(m.find(39) == 109u);
        CHECK_FALSE(m.find(40));
        CHECK_FALSE(m.find(9));
    }

    TEST_CASE("ranges inserted out of order, including in the middle") {
        glyph_mapping m;
        m.bind(50, 60, 500);
        m.bind(10, 20, 100);
        m.bind(30, 40, 300);
        m.bind(70, 80, 700);
        m.bind(0, 5, 0);
        for (char32_t base : {U'\0', char32_t{10}, char32_t{30}, char32_t{50}, char32_t{70}}) {
            CHECK(m.find(base) == static_cast <std::uint32_t>(base * 10));
        }
        CHECK(m.find(4) == 4u);
        CHECK(m.find(35) == 305u);
        CHECK_FALSE(m.find(5));
        CHECK_FALSE(m.find(45));
        CHECK_FALSE(m.find(80));
    }

    TEST_CASE("touching ranges are allowed") {
        glyph_mapping m;
        m.bind(20, 30, 200);
        m.bind(10, 20, 100);
        m.bind(30, 40, 300);
        CHECK(m.find(19) == 109u);
        CHECK(m.find(20) == 200u);
        CHECK(m.find(29) == 209u);
        CHECK(m.find(30) == 300u);
    }

    TEST_CASE("single code points") {
        glyph_mapping m;
        m.bind(U'A', 7u);
        m.bind(U'C', 9u);
        m.bind(U'B', 8u);
        CHECK(m.find(U'A') == 7u);
        CHECK(m.find(U'B') == 8u);
        CHECK(m.find(U'C') == 9u);
        CHECK_FALSE(m.find(U'D'));
        CHECK_THROWS_AS(m.bind(U'B', 1u), std::runtime_error);
    }

    TEST_CASE("overlapping ranges throw and leave the mapping unchanged") {
        glyph_mapping m;
        m.bind(10, 20, 100);
        m.bind(30, 40, 300);

        SUBCASE("overlaps the next range") {
            CHECK_THROWS_AS(m.bind(5, 11, 0), std::runtime_error);
            CHECK_THROWS_AS(m.bind(25, 31, 0), std::runtime_error);
        }
        SUBCASE("overlaps the previous range") {
            CHECK_THROWS_AS(m.bind(15, 25, 0), std::runtime_error);
            CHECK_THROWS_AS(m.bind(39, 50, 0), std::runtime_error);
        }
        SUBCASE("nested inside a range") {
            CHECK_THROWS_AS(m.bind(12, 15, 0), std::runtime_error);
        }
        SUBCASE("covers a range") {
            CHECK_THROWS_AS(m.bind(5, 25, 0), std::runtime_error);
            CHECK_THROWS_AS(m.bind(5, 50, 0), std::runtime_error);
        }
        SUBCASE("same begin") {
            CHECK_THROWS_AS(m.bind(10, 12, 0), std::runtime_error);
            CHECK_THROWS_AS(m.bind(10, 25, 0), std::runtime_error);
        }
        SUBCASE("same range") {
            CHECK_THROWS_AS(m.bind(30, 40, 0), std::runtime_error);
        }

        CHECK(m.find(10) == 100u);
        CHECK(m.find(19) == 109u);
        CHECK(m.find(30) == 300u);
        CHECK_FALSE(m.find(5));
        CHECK_FALSE(m.find(25));
        CHECK_FALSE(m.find(45));
    }

    TEST_CASE("empty and inverted ranges throw") {
        glyph_mapping m;
        CHECK_THROWS_AS(m.bind(5, 5, 0), std::runtime_error);
        CHECK_THROWS_AS(m.bind(6, 5, 0), std::runtime_error);
        CHECK_FALSE(m.find(5));
    }

    TEST_CASE("code points past U+10FFFF throw") {
        glyph_mapping m;
        constexpr char32_t last = glyph_mapping::max_code_point;
        m.bind(last, 1u);
        CHECK(m.find(last) == 1u);
        CHECK_THROWS_AS(m.bind(last + 1, 2u), std::runtime_error);
        CHECK_THROWS_AS(m.bind(char32_t{0xFFFFFFFF}, 2u), std::runtime_error);
        CHECK_THROWS_AS(m.bind(last - 1, last + 2, 3u), std::runtime_error);
    }

    TEST_CASE("glyph id overflow throws") {
        constexpr auto max = std::numeric_limits <std::uint32_t>::max();
        glyph_mapping m;
        m.bind(0, 10, max - 9);
        CHECK(m.find(9) == max);
        CHECK_THROWS_AS(m.bind(20, 31, max - 9), std::runtime_error);
        CHECK_FALSE(m.find(20));
        m.bind(U'A', max);
        CHECK(m.find(U'A') == max);
    }

    TEST_CASE("a range continuing its neighbour is merged into it") {
        using range = glyph_mapping::range;
        glyph_mapping m;
        for (char32_t cp = U'A'; cp <= U'Z'; cp++) {
            m.bind(cp, static_cast <std::uint32_t>(cp - U'A'));
        }
        CHECK(m.ranges().size() == 1);
        CHECK(m.ranges()[0] == range{U'A', U'Z' + 1, 0});

        SUBCASE("a gap filled from the middle joins both sides") {
            glyph_mapping g;
            g.bind(10, 20, 100);
            g.bind(30, 40, 120);
            g.bind(20, 30, 110);
            REQUIRE(g.ranges().size() == 1);
            CHECK(g.ranges()[0] == range{10, 40, 100});
            CHECK(g.find(25) == 115u);
        }
        SUBCASE("touching ranges with other glyphs stay apart") {
            glyph_mapping g;
            g.bind(10, 20, 100);
            g.bind(20, 30, 0);
            CHECK(g.ranges().size() == 2);
        }
        SUBCASE("no merge across the largest glyph id") {
            constexpr auto max = std::numeric_limits <std::uint32_t>::max();
            glyph_mapping g;
            g.bind(10, 11, max);
            g.bind(11, 12, 0);
            CHECK(g.ranges().size() == 2);
            CHECK(g.find(10) == max);
            CHECK(g.find(11) == 0u);
        }
    }

    TEST_CASE("the list constructor binds ranges in any order") {
        const glyph_mapping m{{U'a', U'z' + 1, 0}, {U'A', U'Z' + 1, 0}, {U' ', U' ' + 1, 26}};
        CHECK(m.find(U'a') == 0u);
        CHECK(m.find(U'A') == 0u);
        CHECK(m.find(U'z') == 25u);
        CHECK(m.find(U' ') == 26u);
        CHECK_FALSE(m.find(U'!'));
        CHECK(m.ranges().size() == 3);
        CHECK_FALSE(m.empty());
        CHECK(glyph_mapping{}.empty());
        CHECK_THROWS_AS((glyph_mapping{{10, 20, 0}, {15, 25, 0}}), std::runtime_error);
    }
}
