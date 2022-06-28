//
// Created by igor on 02/05/2022.
//

#include "bm.hh"
#include "test_utils.hh"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <neutrino/hal/video/fonts/rom_fonts.hh>

TEST_SUITE("bitmap") {

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

#if 0
  TEST_CASE("bitmap overlap 1") {
    using bm_t = bitmask<16>;
    auto a = bm_t::create_random(130, 100);
    auto b = bm_t::create_random(41, 31);

    int k = 0;
    for (int y = 0; y<a.height() - 1; y++) {
      for (int x = 0; x<a.width() - 1; x++) {
        int rc_n = naive_overlap_area (a, b, x, y);
        int rc = overlap_area (a, b, x, y);
        if (rc != rc_n) {
          k++;
          std::cout << "TEST_CASE_FIXTURE(test_fixture_16bit,\"test overlap #" << k << "\") {" << std::endl;


          std::cout << "int rc_n = naive_overlap_area (a, b, " << x << ", " << y << ");" << std::endl;
          std::cout << "int rc_o = overlap_area (a, b, " << x << ", " << y << ");" << std::endl;

          std::cout << "REQUIRE(rc_n == rc_o);" << std::endl;
          std::cout << "}" << std::endl;
        } else {
          REQUIRE(rc == rc_n);
        }
      }
    }
  }
#endif
}

#if 0

int main(int argc, char* argv[]) {
  /*
  //neutrino::assets::load_fon ("/home/igor/proj/ares/fon/120TMS18.FON");
  try {
    auto s = neutrino::assets::load_image ("/home/igor/tmp/1/crater.ham");
    neutrino::assets::save ("/home/igor/tmp/1/crater.png", s);
  } catch (std::exception& e) {
    std::cout << "Error: " << e.what() << std::endl;
  }
   */
  auto v = neutrino::hal::get_glyph (neutrino::hal::font::IBM_8x16, 'A');
  using bm8_t = bitmask<8>;
  bm8_t bm(8, 16);
  for (int y=0; y<16; y++) {
    for (int x=0; x<8; x++) {
      if (v[y][x]) {
        bm.set (x, y);
      }
    }
  }

  for (int y=0; y<16; y++) {
    for (int x = 0; x < 8; x++) {
      if (bm.get (x, y)) {
        std::cout << "+";
      } else {
        std::cout << " ";
      }
    }
    std::cout << std::endl;
  }
  return 0;
}

#endif