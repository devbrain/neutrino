//
// Created by igor on 03/08/2021.
//

#include <doctest/doctest.h>
#include "test4/example.h"

#include "test_utils.hh"

using namespace neutrino::tiled::tmx;

TEST_CASE("test4/example.tmx properties and objects")
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
