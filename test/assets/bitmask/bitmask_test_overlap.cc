//
// Created by igor on 02/05/2022.
//

#include "bitmask_test_utils.hh"

#include <doctest/doctest.h>
#include <neutrino/hal/video/fonts/rom_fonts.hh>
using namespace neutrino::assets::impl;

template <int BitsInWord>
[[nodiscard]] int
naive_overlap_area (const bitmask<BitsInWord>& a, const bitmask<BitsInWord>& b, int xoffset, int yoffset) {
  int rc = 0;

  auto w = std::min (b.width(), a.width() - xoffset);
  auto h = std::min (b.height(), a.height() - yoffset);


  for (int y = 0; y < h; y++) {
    int row = 0;
    for (int x = 0; x < w; x++) {
      int xa = x + xoffset;
      int ya = y + yoffset;
      if (a.get (xa, ya) && b.get (x, y)) {
        row++;
        rc++;
      }
    }
  }
  return rc;
}

TEST_SUITE("bitmap overlap") {

  TEST_CASE_FIXTURE(test_fixture_16bit, "test overlap #3374") {
    int rc_n = naive_overlap_area (a, b, 41, 34);
    int rc_o = overlap_area (a, b, 41, 34);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(test_fixture_8bit, "test overlap 8bit") {
    for (int y = 0; y < a.height () - 1; y++) {
      for (int x = 0; x < a.width () - 1; x++) {
        int rc_n = naive_overlap_area (a, b, x, y);
        int rc_o = overlap_area (a, b, x, y);
        REQUIRE(rc_n == rc_o);
      }
    }
  }

  TEST_CASE("bitmap overlap 16 bit #1") {
    using bm_t = bitmask<16>;
    auto a = bm_t::create_random (30, 30);
    auto b = bm_t::create_random (41, 31);
    for (int y = 0; y < a.height () - 1; y++) {
      for (int x = 0; x < a.width () - 1; x++) {
        int rc_n = naive_overlap_area (a, b, x, y);
        int rc_o = overlap_area (a, b, x, y);
        REQUIRE(rc_n == rc_o);
      }
    }
  }

  TEST_CASE("bitmap overlap 16 bit #2") {
    using bm_t = bitmask<16>;
    auto a = bm_t::create_random (130, 100);
    auto b = bm_t::create_random (41, 31);
    for (int y = 0; y < a.height () - 1; y++) {
      for (int x = 0; x < a.width () - 1; x++) {
        int rc_n = naive_overlap_area (a, b, x, y);
        int rc_o = overlap_area (a, b, x, y);
        REQUIRE(rc_n == rc_o);
      }
    }
  }

  TEST_CASE("bitmap overlap 32 bit") {
    using bm_t = bitmask<32>;
    auto a = bm_t::create_random (130, 100);
    auto b = bm_t::create_random (41, 31);
    for (int y = 0; y < a.height () - 1; y++) {
      for (int x = 0; x < a.width () - 1; x++) {
        int rc_n = naive_overlap_area (a, b, x, y);
        int rc_o = overlap_area (a, b, x, y);
        REQUIRE(rc_n == rc_o);
      }
    }
  }



  TEST_CASE("bitmap overlap 32 bit #2") {
    using bm_t = bitmask<32>;
    auto a = bm_t::create_random (430, 131);
    auto b = bm_t::create_random (41, 31);
    for (int y = 0; y < a.height () - 1; y++) {
      for (int x = 0; x < a.width () - 1; x++) {
        int rc_n = naive_overlap_area (a, b, x, y);
        int rc_o = overlap_area (a, b, x, y);
        REQUIRE(rc_n == rc_o);
      }
    }
  }
}
