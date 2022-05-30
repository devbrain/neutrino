//
// Created by igor on 29/05/2022.
//

#include <doctest/doctest.h>
#include <string>
#include <neutrino/assets/resources/resource_id.hh>
#include <neutrino/assets/resources/unique_resource.hh>

TEST_CASE("test resource id") {
  using namespace neutrino::assets;
  resource_id rid;
  resource_id r2("zopa");
  {
    std::string name("zopa");
    rid = resource_id(name);
  }
  REQUIRE(rid.name() == "zopa");
  REQUIRE(rid == r2);
}

TEST_CASE("test unique_resource") {
  using namespace neutrino::assets;
  resource_name rn("zopa");
  {
    auto r1 = make_unique_resource<std::string> ("zopa", [] () { return std::string{"ZOPA"}; });

    REQUIRE (*r1 == "ZOPA");
    REQUIRE(get_resources_holder ().exists (rn));
  }
  REQUIRE(!get_resources_holder ().exists (rn));
}