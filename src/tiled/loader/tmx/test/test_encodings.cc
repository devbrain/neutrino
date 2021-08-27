//
// Created by igor on 29/07/2021.
//

#include <doctest/doctest.h>
#include "test1/test_base64_level.h"
#include "test1/test_csv_level.h"
#include "test1/test_xml_level.h"
#include "test_utils.hh"

using namespace neutrino::tiled::tmx;

static std::vector<int> tiles = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                                 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
                                 1621, 1622, 1623, 1624, 1625, 1626, 1627, 1628, 1629, 1630,
                                 1639, 1640, 1641, 1642, 1643, 1644, 1645, 1646, 1647, 1648,
                                 61, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

TEST_CASE("Test encodings") {
  auto b64_map = test::load_map (test_base64_level, test_base64_level_length);
  auto csv_map = test::load_map (test_csv_level, test_csv_level_length);
  auto xml_map = test::load_map (test_xml_level, test_xml_level_length);

  REQUIRE(!csv_map.layers ().empty ());
  const auto *csv_tl = std::get_if<tile_layer> (&csv_map.layers ()[0]);
  REQUIRE(csv_tl);
  REQUIRE(test::test_tiles (*csv_tl, tiles));

  REQUIRE(!xml_map.layers ().empty ());
  const auto *xml_tl = std::get_if<tile_layer> (&xml_map.layers ()[0]);
  REQUIRE(xml_tl);
  REQUIRE(test::test_tiles (*xml_tl, tiles));

  REQUIRE(!b64_map.layers ().empty ());
  const auto *b64_tl = std::get_if<tile_layer> (&b64_map.layers ()[0]);
  REQUIRE(b64_tl);
  REQUIRE(test::test_tiles (*b64_tl, tiles));
}
