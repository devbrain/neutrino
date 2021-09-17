//
// Created by igor on 03/08/2021.
//

#include <doctest/doctest.h>
#include "test4/example.h"
#include "test4/js_example.h"

#include "test_utils.hh"

using namespace neutrino::tiled::tmx;

static void test_props (const map& the_map) {
  static const std::map<std::string, property_t> props = {
      {"BigInteger", test::to_prop ((int64_t) 999999999)},
      {"EmptyProperty", test::to_prop (std::string{})},
      {"FalseProperty", test::to_prop (false)},
      {"FileProperty", test::to_prop (std::filesystem::path{"torches.png"})},
      {"FloatProperty", test::to_prop (0.12f)},
      {"IntProperty", test::to_prop (1234)},
      {"NegativeFloatProperty", test::to_prop (-0.12f)},
      {"NegativeIntProperty", test::to_prop (-1234)},
      {"StringProperty", test::to_prop (std::string{"A string value"})},
      {"TrueProperty", test::to_prop (true)},
      {"YellowProperty", test::to_prop (colori{"#ffffff00"})}
  };
  REQUIRE(test::check_properties (the_map, props));
}

TEST_CASE("test4/example.tmx properties")
{
  auto the_map = test::load_map (example, example_length);
  test_props (the_map);
}

TEST_CASE("test4/js_example.json properties")
{
  auto the_map = test::load_map (js_example, js_example_length);
  test_props (the_map);
}

static void test_anim (const map& the_map) {
  REQUIRE(the_map.tile_sets ().size () == 2);
  const auto& ts = the_map.tile_sets ()[1];
  REQUIRE(ts.first_gid () == 49);

  REQUIRE(ts.get_image ());
  REQUIRE(ts.get_image ()->width () == 96);
  REQUIRE(ts.get_image ()->height () == 32);

  const auto* tl = ts.get_tile (0);
  REQUIRE(tl);

  const auto* objects_layer = tl->objects ();
  REQUIRE(objects_layer);

  const auto& objects = objects_layer->objects ();

  REQUIRE(!objects.empty ());

  const auto* obj = std::get_if<object> (&objects[0]);
  REQUIRE(obj);
  REQUIRE(obj->width () == 3.875);
  REQUIRE(obj->height () == 8);

  REQUIRE(obj->origin ()[0] == 14);
  REQUIRE(obj->origin ()[1] == 16.875);

  const auto& anm = tl->get_animation ();
  REQUIRE(anm.frames ().size () == 4);
  REQUIRE(anm.frames ()[0].duration () == std::chrono::milliseconds{200});
  REQUIRE(anm.frames ()[0].id () == 0);
}

TEST_CASE("test4/example.tmx animation xml objects")
{
  auto the_map = test::load_map (example, example_length);

  test_anim (the_map);
}

TEST_CASE("test4/js_example.json animation json objects")
{
  auto the_map = test::load_map (js_example, js_example_length);
  test_anim (the_map);
}

static void test_group (const map& the_map) {
  REQUIRE(the_map.layers ().size () == 9);
  for (const auto& v : the_map.layers ()) {
    const auto* tl = std::get_if<tile_layer> (&v);
    if (tl && tl->id () == 2) {
      REQUIRE(tl->name () == "Testing Child Tile Layer");

      REQUIRE(tl->width () == 10);
      REQUIRE(tl->height () == 10);
      REQUIRE(tl->offset_x () == 11);
      REQUIRE(tl->offset_y () == 2);
      break;
    }
  }
}

TEST_CASE("test4/example.tmx testing group")
{
  auto the_map = test::load_map (example, example_length);
  test_group (the_map);
}

TEST_CASE("test4/js_example.json testing group")
{
  auto the_map = test::load_map (js_example, js_example_length);
  test_group (the_map);
}