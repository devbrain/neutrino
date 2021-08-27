//
// Created by igor on 24/08/2021.
//

#include <doctest/doctest.h>
#include "test_utils.hh"
#include "test-rs/tiled_image_layers.h"
#include "test-rs/js_tiled_image_layers.h"

using namespace neutrino::tiled::tmx;

static void test_image_layers (const map &the_map) {
  REQUIRE(the_map.layers ().size () == 2);
  const auto *l = std::get_if<image_layer> (&the_map.layers ()[0]);
  REQUIRE(l);
  REQUIRE(l->name () == "Image Layer 1");
  REQUIRE(!l->get_image ());

  l = std::get_if<image_layer> (&the_map.layers ()[1]);
  REQUIRE(l);
  REQUIRE(l->name () == "Image Layer 2");
  REQUIRE(l->get_image ());
  auto w = l->get_image ()->width ();
  auto h = l->get_image ()->height ();

  if (w) {
    REQUIRE(w == 448);
  }
  if (h) {
    REQUIRE(h == 192);
  }
  REQUIRE(l->get_image ()->source () == "tilesheet.png");

}

TEST_CASE("test tmx image layers") {
  SUBCASE("test image layers xml") {
    auto the_map = test::load_map (tiled_image_layers, tiled_image_layers_length);
    test_image_layers (the_map);
  }
  SUBCASE("test image layers json") {
    auto the_map = test::load_map (js_tiled_image_layers, js_tiled_image_layers_length);
    test_image_layers (the_map);
  }
}