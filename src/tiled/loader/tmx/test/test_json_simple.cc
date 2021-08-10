//
// Created by igor on 07/08/2021.
//

#include <doctest/doctest.h>
#include "json/simple_map.h"
#include "test_utils.hh"

using namespace neutrino::tiled::tmx;
static std::vector<int> tiles = {
        1, 2, 3, 4, 5, 6, 7, 1, 2, 3, 4, 5, 6, 7, 0, 0, 9, 10, 11, 12, 13, 14, 15, 9, 10, 11, 12, 13, 14, 15, 0, 0, 17,
        18, 19, 20, 21, 22, 23, 17, 18, 19, 20, 21, 22, 23, 0, 0, 25, 26, 27, 28, 29, 30, 31, 25, 26, 27, 28, 29, 30,
        31, 0, 0, 33, 34, 35, 36, 37, 38, 39, 0, 0, 35, 36, 37, 38, 39, 0, 0, 41, 42, 43, 44, 45, 46, 47, 0, 0, 43, 44,
        45, 46, 47, 0, 0, 1, 2, 3, 4, 0, 0, 0, 0, 0, 3, 4, 5, 6, 7, 0, 0, 9, 10, 11, 12, 0, 0, 0, 0, 0, 11, 12, 13, 14,
        15, 0, 0, 17, 18, 19, 20, 21, 22, 23, 17, 18, 19, 20, 21, 22, 23, 0, 0, 25, 26, 27, 28, 29, 30, 31, 25, 26, 27,
        28, 29, 30, 31, 0, 0, 33, 34, 35, 36, 37, 38, 39, 33, 34, 35, 36, 37, 38, 39, 0, 0, 41, 42, 43, 44, 45, 46, 47,
        41, 42, 43, 44, 45, 46, 47, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

TEST_CASE("test simple json map")
{
    auto the_map = test::load_map(simple_map, simple_map_length);
    REQUIRE(!the_map.infinite());
    REQUIRE(the_map.orientation() == neutrino::tiled::tmx::orientation_t::ORTHOGONAL);
    REQUIRE(the_map.render_order() == neutrino::tiled::tmx::render_order_t::LEFT_DOWN);
    REQUIRE(the_map.tile_height() == 16);
    REQUIRE(the_map.tile_width() == 16);
    REQUIRE(the_map.height() == 16);
    REQUIRE(the_map.width() == 16);

    REQUIRE(the_map.tile_sets().size() == 1);
    const auto& ts = the_map.tile_sets()[0];
    REQUIRE(ts.first_gid() == 1);
    REQUIRE(ts.name() == "demo-tileset");

    REQUIRE(ts.tile_count() == 48);
    REQUIRE(ts.tile_height() == 16);
    REQUIRE(ts.tile_width() == 16);
    REQUIRE(ts.spacing() == 0);
    REQUIRE(ts.margin() == 0);
    REQUIRE(ts.columns() == 8);

    auto img = ts.get_image();
    REQUIRE(img);
    REQUIRE(img->source() == "../demo-tileset.png");
    REQUIRE(img->width() == 128);
    REQUIRE(img->height() == 96);

    REQUIRE(the_map.layers().size() == 1);
    const auto* tl = std::get_if<tile_layer>(&the_map.layers()[0]);
    REQUIRE(tl);
    REQUIRE(tl->width() == 16);
    REQUIRE(tl->height() == 16);
    REQUIRE(tl->name() == "simple_layer");
    REQUIRE(test::test_tiles(*tl, tiles));
}