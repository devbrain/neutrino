//
// Created by igor on 30/07/2021.
//

#include <doctest/doctest.h>
#include "test_utils.hh"
#include "test-rs/tiled_base64_gzip.h"
#include "test-rs/tiled_base64_zlib.h"
#include "test-rs/js_tiled_base64_gzip.h"
#include "test-rs/tiled_base64_zstandard.h"

using namespace neutrino::tiled::tmx;

TEST_CASE("test tmx compression") {
  auto zmap = test::load_map (tiled_base64_zlib, tiled_base64_zlib_length);
  auto gmap = test::load_map (tiled_base64_gzip, tiled_base64_gzip_length);
  auto zsmap = test::load_map (tiled_base64_zstandard, tiled_base64_zstandard_length);

  REQUIRE(!zmap.layers ().empty ());
  const auto *ztl = std::get_if<tile_layer> (&zmap.layers ()[0]);
  REQUIRE(ztl);

  REQUIRE(!gmap.layers ().empty ());
  const auto *gtl = std::get_if<tile_layer> (&gmap.layers ()[0]);
  REQUIRE(gtl);

  REQUIRE(!zsmap.layers ().empty ());
  const auto *zstl = std::get_if<tile_layer> (&zsmap.layers ()[0]);
  REQUIRE(zstl);

  REQUIRE(ztl->cells ().size () == 10000);
  REQUIRE(gtl->cells ().size () == 10000);
  REQUIRE(zstl->cells ().size () == 10000);

  REQUIRE(test::eq_cells (ztl->cells (), gtl->cells ()));
  REQUIRE(test::eq_cells (zstl->cells (), gtl->cells ()));
}

TEST_CASE("test json compression") {
  auto gmap = test::load_map (tiled_base64_gzip, tiled_base64_gzip_length);

  REQUIRE(!gmap.layers ().empty ());
  const auto *gtl = std::get_if<tile_layer> (&gmap.layers ()[0]);
  REQUIRE(gtl);
  REQUIRE(gtl->cells ().size () == 10000);

}