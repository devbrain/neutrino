//
// Created by igor on 26/06/2022.
//

#include <set>
#include <doctest/doctest.h>

#include "bitmask_test_utils.hh"

using Bits8 = std::integral_constant<int,8>;
using Bits16 = std::integral_constant<int,16>;
using Bits32 = std::integral_constant<int,32>;


TYPE_TO_STRING(Bits8);
TYPE_TO_STRING(Bits16);
TYPE_TO_STRING(Bits32);
using namespace neutrino::assets::impl;

TEST_SUITE("Bitmask Tests") {

  TEST_CASE_TEMPLATE("test bm getter/setter", Bits, Bits8, Bits16, Bits32) {
    int w = 10 * Bits::value + 7;
    int h = 10 * Bits::value + 6;
    bitmask<Bits::value> bm (w, h);
    std::random_device random_device;
    std::mt19937 random_engine (random_device ());
    std::uniform_int_distribution<int> d (1, 100);
    std::set<std::pair<int, int>> dots;
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        if (d (random_engine) < 50) {
          bm.set (x, y);
          dots.emplace (x, y);
        }
      }
    }
    int n = 0;
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        if (bm.get (x, y)) {
          std::pair<int, int> p (x, y);
          if (dots.find (p) == dots.end ()) {
            MESSAGE("Failed for ", x, ":", y);
            REQUIRE_FALSE(true);
          }
          n++;
        }
      }
    }
    REQUIRE(n == dots.size ());
  }
}
