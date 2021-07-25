//
// Created by igor on 24/07/2021.
//

#include <doctest/doctest.h>
#include "test1/example.h"
#include "test_utils.hh"

using namespace neutrino::tiled::tmx;

TEST_CASE("test1/example/test map attribs")
{
    auto the_map = test::load_map(example, example_length);
    REQUIRE(the_map.tile_width() == 32);
    REQUIRE(the_map.tile_height() == 32);
    REQUIRE(the_map.width() == 10);
    REQUIRE(the_map.height() == 10);
    REQUIRE(the_map.orientation() == orientation_t::ORTHOGONAL);
    REQUIRE(the_map.get_render_order() == render_order_t::RIGHT_DOWN);
    REQUIRE(the_map.version() == "1.0");
    REQUIRE(the_map.background_color() == "#595959");
    REQUIRE(the_map.next_object_id() == 5);
}
