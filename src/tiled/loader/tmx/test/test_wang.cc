//
// Created by igor on 23/08/2021.
//

#include <doctest/doctest.h>
#include "test_utils.hh"
#include "wang/isometric_staggered_grass_and_water.h"
#include "wang/js_isometric_staggered_grass_and_water.h"

using namespace neutrino::tiled::tmx;

static void test_wang (const map& the_map) {
  REQUIRE(the_map.orientation () == orientation_t::STAGGERED);
  REQUIRE(the_map.render_order () == render_order_t::RIGHT_DOWN);
  REQUIRE(the_map.infinite ());
  REQUIRE(the_map.stagger_axis () == stagger_axis_t::Y);
  REQUIRE(the_map.stagger_index () == stagger_index_t::ODD);
  REQUIRE(the_map.tile_sets ().size () == 1);

  const auto& ts = the_map.tile_sets ()[0];
  REQUIRE(ts.wang_sets ().size () == 1);
  auto grid = ts.grid_info ();
  REQUIRE(grid);
  REQUIRE(!grid->is_orthogonal ());
  REQUIRE(grid->width () == 64);
  REQUIRE(grid->height () == 32);

  const auto& ws = ts.wang_sets ()[0];
  REQUIRE(ws.name () == "Terrains");
  REQUIRE(ws.local_tile () == 15);

  REQUIRE(ws.colors ().size () == 2);
  const auto& ws_color = ws.colors ()[1];
  REQUIRE(ws_color.color () == "#729fcf");
  REQUIRE(ws_color.name () == "Water");
  REQUIRE(ws_color.local_tile () == 22);
  REQUIRE(ws_color.prob () == 1);

  const auto& ws_tiles = ws.tiles ();
  REQUIRE(ws_tiles.size () == 24);
  const auto wtl = ws_tiles[8];
  REQUIRE(wtl.gid () == 8);
  const auto& w = wtl.wang_id ();
  REQUIRE(w[wang_tile::TOP] == 0);
  REQUIRE(w[wang_tile::TOP_RIGHT] == 2);
  REQUIRE(w[wang_tile::RIGHT] == 0);
  REQUIRE(w[wang_tile::BOTTOM_RIGHT] == 2);
  REQUIRE(w[wang_tile::BOTTOM] == 0);
  REQUIRE(w[wang_tile::BOTTOM_LEFT] == 2);
  REQUIRE(w[wang_tile::LEFT] == 0);
  REQUIRE(w[wang_tile::TOP_LEFT] == 1);
}

TEST_CASE("Test Wang") {
  SUBCASE("wang json test") {
    auto the_map = test::load_map (js_isometric_staggered_grass_and_water, js_isometric_staggered_grass_and_water_length);
    test_wang (the_map);
  }

  SUBCASE("wang xml test") {
    auto the_map = test::load_map (isometric_staggered_grass_and_water, isometric_staggered_grass_and_water_length);
    test_wang (the_map);
  }
}