//
// Created by igor on 29/05/2022.
//

#include <doctest/doctest.h>
#include <neutrino/utils/id.hh>

TEST_CASE("test id") {
  auto id1 = neutrino::utils::id("AAA");
  auto id2 = neutrino::utils::id("PIZDA");
  auto id3 = neutrino::utils::id("AAA");
  REQUIRE(id1 == id3);
  REQUIRE(id2 != id1);
}