#include <doctest/doctest.h>
#include "tiled/loader/tmx/color.hh"

using namespace neutrino::tiled;

#define EXPECT_EQ(A, B) REQUIRE(A==B)

TEST_CASE("ColorTEST_CASE, argb")
{
    tmx::colori color {"#ffaa07ff"};
    tmx::colori expected {170, 7, 255, 255 };
    EXPECT_EQ( color, expected );
}

TEST_CASE("ColorTEST_CASE, argbf")
{
    tmx::colorf color {"#ffaa07ff"};
    tmx::colori expected {170, 7, 255, 255 };
    EXPECT_EQ( color, expected.as_float() );
}

TEST_CASE("ColorTEST_CASE, rgb")
{
    tmx::colori color {"#aa07ff"};
    tmx::colori expected { 170, 7, 255, 255};
    EXPECT_EQ( color, expected );
}

TEST_CASE("ColorTEST_CASE, color_float")
{
    tmx::colori color {29, 170, 205, 255};
    tmx::colorf expected {(float)29 / 255, (float)170 / 255, (float)205 / 255, (float)255 / 255};
    tmx::colorf result = color.as_float();

    EXPECT_EQ( expected, result );
}

TEST_CASE("ColorTEST_CASE, color_compare")
{
    tmx::colori color {170, 7, 255, 255 };
    REQUIRE( color == "#ffaa07ff" );
}