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
    REQUIRE(the_map.render_order() == render_order_t::RIGHT_DOWN);
    REQUIRE(the_map.version() == "1.0");
    REQUIRE(the_map.background_color() == "#595959");
    REQUIRE(the_map.next_object_id() == 5);

    std::map<std::string, property_t> expected_map = {
            {"name", std::string("Test")},
            {"test", std::string("foo")}
    };
    REQUIRE(test::check_properties(the_map, expected_map));

    REQUIRE(the_map.tile_sets().size() == 2);
    const auto& ts1 = the_map.tile_sets()[0];

    REQUIRE(ts1.first_gid() == 1);
    REQUIRE(ts1.name() == "marioenemies");
    REQUIRE(ts1.tile_width() == 32);
    REQUIRE(ts1.tile_height() == 32);

    REQUIRE(ts1.get_terrains().size() == 1);
    REQUIRE(ts1.get_terrains()[0].tile() == 0);
    REQUIRE(ts1.get_terrains()[0].name() == "New Terrain");

    REQUIRE(ts1.get_tile(0));
    REQUIRE(ts1.get_tile(0)->id() == 0);
    REQUIRE(test::check_properties(*ts1.get_tile(0), {{"wall", std::string("true")}}));

    REQUIRE(ts1.get_tile(1));
    REQUIRE(ts1.get_tile(1)->id() == 1);
    REQUIRE(test::check_properties(*ts1.get_tile(1), {{"wall", std::string("true")}}));

    const auto& ts2 = the_map.tile_sets()[1];

    REQUIRE(ts2.first_gid() == 101);
    REQUIRE(ts2.name() == "marioobjects");
    REQUIRE(ts2.tile_width() == 32);
    REQUIRE(ts2.tile_height() == 32);

    REQUIRE(ts2.get_image());
    REQUIRE(ts2.get_image()->source() == "../flappyman/docs/marioobjects.png");
    REQUIRE(ts2.get_image()->width() == 576);
    REQUIRE(ts2.get_image()->height() == 336);


    std::vector<int> ids = {
            101, 100, 280, 1, 1, 0, 0, 0, 0, 0, 1, 101, 0, 0, 0, 0,
            0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0, 0, 0, 0
    };

    REQUIRE(the_map.layers().size() == 2);
    const auto* tl = std::get_if<tile_layer>(&the_map.layers()[0]);
    REQUIRE(tl);
    int k = 0;
    for (const auto c : *tl)
    {
        REQUIRE(k < ids.size());
        REQUIRE(c.gid() == ids[k++]);
    }
    REQUIRE(k == ids.size());

    REQUIRE (the_map.objects().size() == 2);

    const object_layer& oe = the_map.objects()[1];
    REQUIRE(oe.empty());
    REQUIRE(oe.name() == "TestObject2");

    const object_layer& ol = the_map.objects()[0];
    REQUIRE(ol.name() == "TestObject");
    REQUIRE(ol.draw_order() == draw_order_t::TOP_DOWN);
    REQUIRE(ol.color() == "#a0a0a4");
    REQUIRE(test::check_properties(ol, {{"objectlayerprop", std::string("666")}}));

    const auto& objects = ol.objects();
    REQUIRE(objects.size() == 4);

    const auto* o1 = std::get_if<object>(&objects[0]);
    REQUIRE(o1 != nullptr);
    REQUIRE(o1->id() == 1);
    REQUIRE(o1->origin() == neutrino::math::point2d{7, 11});
    REQUIRE(o1->width() == 51);
    REQUIRE(o1->height() == 51);
    REQUIRE(test::check_properties(*o1, {{"asquare", std::string("test")}}));


}
