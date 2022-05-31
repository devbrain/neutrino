//
// Created by igor on 29/05/2022.
//

#include <doctest/doctest.h>
#include <string>
#include <neutrino/assets/resources/resource_id.hh>


TEST_CASE("test resource id") {
  using namespace neutrino::assets;
  resource_id rid;
  REQUIRE(!rid);
  resource_id r2("zopa");
  {
    std::string name("zopa");
    rid = resource_id(name);
  }
  REQUIRE(rid.name() == "zopa");
  REQUIRE(rid == r2);
}

