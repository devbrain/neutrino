//
// Created by igor on 26/06/2022.
//

#include <set>
#include <doctest/doctest.h>

#include "bm.hh"
#include "test_utils.hh"

using Bits8 = std::integral_constant<int,8>;
using Bits16 = std::integral_constant<int,16>;
using Bits32 = std::integral_constant<int,32>;
using Bits64 = std::integral_constant<int,64>;

TYPE_TO_STRING(Bits8);
TYPE_TO_STRING(Bits16);
TYPE_TO_STRING(Bits32);
TYPE_TO_STRING(Bits64);

static bool test_vectors(const std::vector<bool>& a, const std::vector<bool>& b) {
  if (a.size() != b.size()) {
    return false;
  }
  for (std::size_t i=0; i<a.size(); i++) {
    if (a[i] != b[i]) {
      return false;
    }
  }
  return true;
}

TEST_SUITE("Bitmask Tests") {

#if 0
  TEST_CASE_FIXTURE(TestFixture1,"test shift register #11") {

    using bm_t = decltype (a);
    auto max_width = std::min(a.width(), bm_t::bits_in_word);
    int k = 0;
    print_words();
    for (int ypos = 0; ypos < a.height(); ypos++) {
      for (int xpos = 0; xpos < max_width - 1; xpos++) {
        auto [req, num] = a.get_row_word (xpos, ypos);
        //num--;
        auto [word, bit] = bm_t::coords (xpos, ypos, a.width ());
        if (k == 0) {
          std::cout << "shift_register<" << bm_t::bits_in_word << "> sr(a.data() + " << word << ", " << bit << ", "
                    << xpos << ", " << num << ");" << std::endl;
        } else {
          std::cout << "sr = shift_register<" << bm_t::bits_in_word << ">(a.data() + " << word << ", " << bit << ", "
                    << xpos << ", " << num << ");" << std::endl;
        }
        std::cout << "auto r" << k << " = sr.shift();" << std::endl;
        std::cout << "REQUIRE(r" << k << " == 0b" << binary(req);
        k++;

        std::cout << ");" << std::endl;
        std::cout << std::endl;
      }
    }
  }

  TEST_CASE_FIXTURE(TestFixture1,"test shift register #1") {
    using bm_t = decltype (a);
    int xpos = 0;
    int ypos = 0;

    int w = a.width() - xpos;
    std::vector<bool> x(w, false);

    for (int i=0; i<w; i++) {
      x[i] = a.get (xpos+i, ypos);
    }

    auto [word, bit] = bm_t::coords (xpos, ypos, a.width());

    shift_register<bm_t::bits_in_word> sr(a.data() + word, bit, xpos, w);
    auto enc = sr.shift();
    REQUIRE(sr.has_bits() == w);
    std::vector<bool> y(w, false);
    for (int i=0; i<w; i++) {
      auto mask = detail::bitmask_traits<bm_t ::bits_in_word>::unit << i;
      if ((enc & mask) == mask) {
        y[i] = true;
      }
    }
    REQUIRE(test_vectors (x, y));
  }
#endif
  TEST_CASE_TEMPLATE("test bm getter/setter", Bits, Bits8, Bits16, Bits32, Bits64) {
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