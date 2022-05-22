//
// Created by igor on 29/07/2021.
//

#include <array>
#include <iostream>
#include <doctest/doctest.h>
#include "test_utils.hh"
#include "test-rs/tiled_flipped.h"
#include "test-rs/tilesheet.h"
#include "flip/test.h"
#include <neutrino/kernel/rc/tile_handle.hh>

using namespace neutrino::kernel::tmx;

const uint32_t FLIPPED_HORIZONTALLY_FLAG  = 0x80000000;
const uint32_t FLIPPED_VERTICALLY_FLAG    = 0x40000000;
const uint32_t FLIPPED_DIAGONALLY_FLAG    = 0x20000000;
const uint32_t ROTATED_HEXAGONAL_120_FLAG = 0x10000000;

static uint32_t global_tile_id(uint32_t v) {
  return v & ~(FLIPPED_HORIZONTALLY_FLAG |
                      FLIPPED_VERTICALLY_FLAG |
                      FLIPPED_DIAGONALLY_FLAG |
                      ROTATED_HEXAGONAL_120_FLAG);

}

static std::tuple<bool, bool, bool> get_rotation(uint32_t v) {
  bool hf = (v & FLIPPED_HORIZONTALLY_FLAG) == FLIPPED_HORIZONTALLY_FLAG;
  bool vf = (v & FLIPPED_VERTICALLY_FLAG) == FLIPPED_VERTICALLY_FLAG;
  bool df = (v & FLIPPED_DIAGONALLY_FLAG) == FLIPPED_DIAGONALLY_FLAG;
  return {hf, vf, df};
}


TEST_CASE("zopa") {
  uint32_t orig = 298;
  uint32_t hor  = 2147483946;
  uint32_t vert = 1073742122;
  uint32_t l1 = 1610613034;
  uint32_t l2 = 3221225770;
  uint32_t l3 = 2684354858;

  uint32_t r1 = 2684354858;
  uint32_t r2 = 3221225770;
  uint32_t r3 = 1610613034;

  REQUIRE(global_tile_id (hor) == orig);
  REQUIRE(global_tile_id (vert) == orig);
  REQUIRE(global_tile_id (l1) == orig);
  REQUIRE(global_tile_id (l2) == orig);
  REQUIRE(global_tile_id (l3) == orig);
  REQUIRE(global_tile_id (r1) == orig);
  REQUIRE(global_tile_id (r2) == orig);
  REQUIRE(global_tile_id (r3) == orig);

  bool hf, vf, df;
  std::tie(hf, vf, df) = get_rotation (l1); // rotate left // 270 clockwise
  REQUIRE(!hf);
  REQUIRE(vf);
  REQUIRE(df);

  std::tie(hf, vf, df) = get_rotation (l2); // 2 * rotate left // 180 clockwise

  REQUIRE(hf);
  REQUIRE(vf);
  REQUIRE(!df);

  std::tie(hf, vf, df) = get_rotation (l3); // 3 * rotate left // 90 clockwise

  REQUIRE(hf);
  REQUIRE(!vf);
  REQUIRE(df);

  std::tie(hf, vf, df) = get_rotation (hor);
  REQUIRE(hf);
  REQUIRE(!vf);
  REQUIRE(!df);

  std::tie(hf, vf, df) = get_rotation (vert);
  REQUIRE(!hf);
  REQUIRE(vf);
  REQUIRE(!df);

}

TEST_CASE("test_flipped") {
  auto resolver = [] (const std::string& p) -> std::string {
    if (p == "tilesheet.tsx") {
      return std::string ((char*) tilesheet, tilesheet_length);
    }
    return {};
  };
  auto the_map = test::load_map (tiled_flipped, tiled_flipped_length, resolver);
  REQUIRE(!the_map.layers ().empty ());
  const auto* tl = std::get_if<tile_layer> (&the_map.layers ()[0]);
  REQUIRE(tl != nullptr);

  REQUIRE(tl->cells ().size () == 4);
  const auto& t1 = tl->cells ()[0];
  const auto& t2 = tl->cells ()[1];
  const auto& t3 = tl->cells ()[2];
  const auto& t4 = tl->cells ()[3];

  REQUIRE(t1.gid () == 3);
  REQUIRE(t1.diag_flipped ());
  REQUIRE(t1.hor_flipped ());
  REQUIRE(t1.vert_flipped ());

  REQUIRE(t2.gid () == 3);
  REQUIRE(!t2.diag_flipped ());
  REQUIRE(!t2.hor_flipped ());
  REQUIRE(t2.vert_flipped ());

  REQUIRE(t3.gid () == 3);
  REQUIRE(!t3.diag_flipped ());
  REQUIRE(t3.hor_flipped ());
  REQUIRE(!t3.vert_flipped ());

  REQUIRE(t4.gid () == 3);
  REQUIRE(t4.diag_flipped ());
  REQUIRE(!t4.hor_flipped ());
  REQUIRE(!t4.vert_flipped ());
}

TEST_CASE("Test flipped 2") {
  auto resolver = [] (const std::string& p) -> std::string {
    return {};
  };
  auto the_map = test::load_map (::test, ::test_length, resolver);
  REQUIRE(!the_map.layers ().empty ());
  const auto* tl = std::get_if<tile_layer> (&the_map.layers ()[0]);
  REQUIRE(tl != nullptr);

  std::array<uint32_t, 16> data {
      1,2147483649,1073741825,3221225473,
      1610612737,3221225473,2684354561,1,
      536870913,1073741825,3758096385,2147483649,
      3758096385,2147483649,536870913,1073741825
  };

  REQUIRE(tl->cells().size() == data.size());
  std::size_t k = 0;
  for (const auto& c : tl->cells()) {
    if (k == 9) {
      int zopa = 0;
    }
    auto [hf, vf, df] = get_rotation (data[k]);
   // std::cout << "[" << hf << vf << df << "]";

    auto gid = global_tile_id (data[k]);
    REQUIRE(c.gid() == gid);
    REQUIRE(c.vert_flipped() == vf);
    REQUIRE(c.diag_flipped() == df);
    REQUIRE(c.hor_flipped() == hf);
    k++;
    neutrino::kernel::tile_handle th(neutrino::kernel::atlas_id_t(1), neutrino::kernel::cell_id_t(1), hf,vf,df);
    REQUIRE(c.vert_flipped() == th.is_vflipped());
    REQUIRE(c.hor_flipped() == th.is_hflipped());
    REQUIRE(c.diag_flipped() == th.is_dflipped());
  //  if (k %4 == 0) {
  //    std::cout << std::endl;
  //  }
  }

}