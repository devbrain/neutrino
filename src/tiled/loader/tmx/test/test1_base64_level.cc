//
// Created by igor on 25/07/2021.
//

#include <doctest/doctest.h>
#include "test1/test_base64_level.h"
#include "test_utils.hh"

using namespace neutrino::tiled::tmx;

TEST_CASE("test1/test_base64_level/test map attribs")
{
    auto the_map = test::load_map(test_base64_level, test_base64_level_length);
    REQUIRE(the_map.tile_width() == 16);
    REQUIRE(the_map.tile_height() == 16);
    REQUIRE(the_map.width() == 10);
    REQUIRE(the_map.height() == 10);
    REQUIRE(the_map.orientation() == orientation_t::ORTHOGONAL);
    REQUIRE(the_map.render_order() == render_order_t::RIGHT_DOWN);
    REQUIRE(the_map.version() == "1.0");
    REQUIRE(the_map.background_color() == "#000000");
    REQUIRE(the_map.next_object_id() == 11);
}
