//
// Created by igor on 03/08/2021.
//

#include <doctest/doctest.h>
#include "test4/example.h"

#include "test_utils.hh"

using namespace neutrino::tiled::tmx;

TEST_CASE("test4/example.tmx properties")
{
    auto the_map = test::load_map(example, example_length);

    static const std::map<std::string, property_t> props = {
            {"BigInteger", property_t{999999999}},
            {"EmptyProperty", property_t {std::string{}}},
            {"FalseProperty", property_t {false}},
            {"FileProperty", property_t {std::filesystem::path{"torches.png"}}},
            {"FloatProperty", property_t {0.12f}},
            {"IntProperty", property_t {1234}},
            {"NegativeFloatProperty", property_t {-0.12f}},
            {"NegativeIntProperty", property_t {-1234}},
            {"StringProperty", property_t {std::string{"A string value"}}},
            {"TrueProperty", property_t {true}},
            {"YellowProperty", property_t {colori{"#ffffff00"}}}
    };
    REQUIRE(test::check_properties(the_map, props));
}

TEST_CASE("test4/example.tmx animation ad objects")
{
    auto the_map = test::load_map(example, example_length);

    REQUIRE(the_map.tile_sets().size() == 2);
    const auto& ts = the_map.tile_sets()[1];
    REQUIRE(ts.first_gid() == 49);

    REQUIRE(ts.get_image());
    REQUIRE(ts.get_image()->width() == 96);
    REQUIRE(ts.get_image()->height() == 32);

    const auto* tl = ts.get_tile(0);
    REQUIRE(tl);

    const auto* objects_layer = tl->objects();
    REQUIRE(objects_layer);

    const auto& objects = objects_layer->objects();

    REQUIRE(!objects.empty());

    const auto* obj = std::get_if<object>(&objects[0]);
    REQUIRE(obj);
    REQUIRE(obj->width() == 3.875);
    REQUIRE(obj->height() == 8);

    REQUIRE(obj->origin()[0] == 14);
    REQUIRE(obj->origin()[1] == 16.875);

    const auto& anm = tl->get_animation();
    REQUIRE(anm.frames().size() == 4);
    REQUIRE(anm.frames()[0].duration() == std::chrono::milliseconds{200});
    REQUIRE(anm.frames()[0].id() == 0);
}

TEST_CASE("test4/example.tmx testing group")
{
    auto the_map = test::load_map(example, example_length);
    REQUIRE(the_map.layers().size() == 9);
    const auto* tl = std::get_if<tile_layer>(&the_map.layers()[6]);
    REQUIRE(tl);
    REQUIRE(tl->name() == "Testing Child Tile Layer");

    REQUIRE(tl->width() == 10);
    REQUIRE(tl->height() == 10);
    REQUIRE(tl->offset_x() == 11);
    REQUIRE(tl->offset_y() == 2);
}