//
// Created by igor on 02/05/2022.
//

#include "bm.hh"
#include "test_utils.hh"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <neutrino/hal/video/fonts/rom_fonts.hh>

TEST_SUITE("bitmap") {

  TEST_CASE_FIXTURE(TestFixture1, "test overlap #1") {
    int rc_n = naive_overlap_area (a, b, 0, 0);
    int rc_o = overlap_area (a, b, 0, 0);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1, "test overlap #2") {

    int rc_n = naive_overlap_area (a, b, 1, 0);

    int rc_o = overlap_area (a, b, 1, 0);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1, "test overlap #3") {

    int rc_n = naive_overlap_area (a, b, 2, 0);
    int rc_o = overlap_area (a, b, 2, 0);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #4") {

    int rc_n = naive_overlap_area (a, b, 3, 0);
    int rc_o = overlap_area (a, b, 3, 0);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #5") {

    int rc_n = naive_overlap_area (a, b, 4, 0);
    int rc_o = overlap_area (a, b, 4, 0);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #6") {

    int rc_n = naive_overlap_area (a, b, 5, 0);
    int rc_o = overlap_area (a, b, 5, 0);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #7") {

    int rc_n = naive_overlap_area (a, b, 6, 0);
    int rc_o = overlap_area (a, b, 6, 0);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #8") {

    int rc_n = naive_overlap_area (a, b, 7, 0);
    int rc_o = overlap_area (a, b, 7, 0);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #9") {

    int rc_n = naive_overlap_area (a, b, 0, 1);
    int rc_o = overlap_area (a, b, 0, 1);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #10") {

    int rc_n = naive_overlap_area (a, b, 1, 1);
    int rc_o = overlap_area (a, b, 1, 1);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #11") {

    int rc_n = naive_overlap_area (a, b, 2, 1);
    int rc_o = overlap_area (a, b, 2, 1);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #12") {

    int rc_n = naive_overlap_area (a, b, 3, 1);
    int rc_o = overlap_area (a, b, 3, 1);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #13") {

    int rc_n = naive_overlap_area (a, b, 4, 1);
    int rc_o = overlap_area (a, b, 4, 1);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #14") {

    int rc_n = naive_overlap_area (a, b, 5, 1);
    int rc_o = overlap_area (a, b, 5, 1);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #15") {

    int rc_n = naive_overlap_area (a, b, 7, 1);
    int rc_o = overlap_area (a, b, 7, 1);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #16") {

    int rc_n = naive_overlap_area (a, b, 0, 2);
    int rc_o = overlap_area (a, b, 0, 2);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #17") {

    int rc_n = naive_overlap_area (a, b, 1, 2);
    int rc_o = overlap_area (a, b, 1, 2);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #18") {

    int rc_n = naive_overlap_area (a, b, 2, 2);
    int rc_o = overlap_area (a, b, 2, 2);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #19") {

    int rc_n = naive_overlap_area (a, b, 3, 2);
    int rc_o = overlap_area (a, b, 3, 2);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #20") {

    int rc_n = naive_overlap_area (a, b, 4, 2);
    int rc_o = overlap_area (a, b, 4, 2);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #21") {

    int rc_n = naive_overlap_area (a, b, 5, 2);
    int rc_o = overlap_area (a, b, 5, 2);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #22") {

    int rc_n = naive_overlap_area (a, b, 7, 2);
    int rc_o = overlap_area (a, b, 7, 2);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #23") {

    int rc_n = naive_overlap_area (a, b, 0, 3);
    int rc_o = overlap_area (a, b, 0, 3);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #24") {

    int rc_n = naive_overlap_area (a, b, 1, 3);
    int rc_o = overlap_area (a, b, 1, 3);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #25") {

    int rc_n = naive_overlap_area (a, b, 2, 3);
    int rc_o = overlap_area (a, b, 2, 3);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #26") {

    int rc_n = naive_overlap_area (a, b, 3, 3);
    int rc_o = overlap_area (a, b, 3, 3);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #27") {

    int rc_n = naive_overlap_area (a, b, 4, 3);
    int rc_o = overlap_area (a, b, 4, 3);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #28") {

    int rc_n = naive_overlap_area (a, b, 5, 3);
    int rc_o = overlap_area (a, b, 5, 3);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #29") {

    int rc_n = naive_overlap_area (a, b, 7, 3);
    int rc_o = overlap_area (a, b, 7, 3);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #30") {

    int rc_n = naive_overlap_area (a, b, 0, 4);
    int rc_o = overlap_area (a, b, 0, 4);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #31") {

    int rc_n = naive_overlap_area (a, b, 1, 4);
    int rc_o = overlap_area (a, b, 1, 4);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #32") {

    int rc_n = naive_overlap_area (a, b, 2, 4);
    int rc_o = overlap_area (a, b, 2, 4);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #33") {

    int rc_n = naive_overlap_area (a, b, 3, 4);
    int rc_o = overlap_area (a, b, 3, 4);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #34") {

    int rc_n = naive_overlap_area (a, b, 6, 4);
    int rc_o = overlap_area (a, b, 6, 4);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #35") {

    int rc_n = naive_overlap_area (a, b, 7, 4);
    int rc_o = overlap_area (a, b, 7, 4);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #36") {

    int rc_n = naive_overlap_area (a, b, 0, 5);
    int rc_o = overlap_area (a, b, 0, 5);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #37") {

    int rc_n = naive_overlap_area (a, b, 1, 5);
    int rc_o = overlap_area (a, b, 1, 5);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #38") {

    int rc_n = naive_overlap_area (a, b, 2, 5);
    int rc_o = overlap_area (a, b, 2, 5);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #39") {

    int rc_n = naive_overlap_area (a, b, 3, 5);
    int rc_o = overlap_area (a, b, 3, 5);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #40") {

    int rc_n = naive_overlap_area (a, b, 4, 5);
    int rc_o = overlap_area (a, b, 4, 5);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #41") {

    int rc_n = naive_overlap_area (a, b, 5, 5);
    int rc_o = overlap_area (a, b, 5, 5);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #42") {

    int rc_n = naive_overlap_area (a, b, 6, 5);
    int rc_o = overlap_area (a, b, 6, 5);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #43") {

    int rc_n = naive_overlap_area (a, b, 7, 5);
    int rc_o = overlap_area (a, b, 7, 5);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #44") {

    int rc_n = naive_overlap_area (a, b, 0, 6);
    int rc_o = overlap_area (a, b, 0, 6);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #45") {

    int rc_n = naive_overlap_area (a, b, 1, 6);
    int rc_o = overlap_area (a, b, 1, 6);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #46") {

    int rc_n = naive_overlap_area (a, b, 2, 6);
    int rc_o = overlap_area (a, b, 2, 6);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #47") {

    int rc_n = naive_overlap_area (a, b, 3, 6);
    int rc_o = overlap_area (a, b, 3, 6);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #48") {

    int rc_n = naive_overlap_area (a, b, 4, 6);
    int rc_o = overlap_area (a, b, 4, 6);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #49") {

    int rc_n = naive_overlap_area (a, b, 5, 6);
    int rc_o = overlap_area (a, b, 5, 6);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #50") {

    int rc_n = naive_overlap_area (a, b, 6, 6);
    int rc_o = overlap_area (a, b, 6, 6);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #51") {

    int rc_n = naive_overlap_area (a, b, 7, 6);
    int rc_o = overlap_area (a, b, 7, 6);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #52") {

    int rc_n = naive_overlap_area (a, b, 1, 7);
    int rc_o = overlap_area (a, b, 1, 7);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #53") {

    int rc_n = naive_overlap_area (a, b, 2, 7);
    int rc_o = overlap_area (a, b, 2, 7);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #54") {

    int rc_n = naive_overlap_area (a, b, 3, 7);
    int rc_o = overlap_area (a, b, 3, 7);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #55") {

    int rc_n = naive_overlap_area (a, b, 4, 7);
    int rc_o = overlap_area (a, b, 4, 7);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #56") {

    int rc_n = naive_overlap_area (a, b, 5, 7);
    int rc_o = overlap_area (a, b, 5, 7);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE_FIXTURE(TestFixture1,"test overlap #57") {
    int rc_n = naive_overlap_area (a, b, 6, 7);
    int rc_o = overlap_area (a, b, 6, 7);
    REQUIRE(rc_n == rc_o);
  }

#if 0
  TEST_CASE_FIXTURE(TestFixture1,"bitmap overlap 1") {
    using bm_t = bitmask<8>;
    auto a = bm_t::create_random(9, 9);
    auto b = bm_t::create_random(9, 9);

    int k = 0;
    for (int y = 0; y<a.height() - 1; y++) {
      for (int x = 0; x<a.width() - 1; x++) {
        int rc_n = naive_overlap_area (a, b, x, y);
        int rc = overlap_area (a, b, x, y);
        if (rc != rc_n) {
          k++;
          std::cout << "TEST_CASE_FIXTURE(TestFixture1,\"test overlap #" << k << "\") {" << std::endl;

          a.debug ("a");
          b.debug ("b");

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