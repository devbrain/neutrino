//
// Created by igor on 23/09/2021.
//
#include <doctest/doctest.h>
#include <string>
#include <neutrino/utils/ini/config_parser.hh>

TEST_SUITE("ini test case") {
  TEST_CASE("test from json simple") {
    struct A {
      int x {};
      std::optional<bool> CONFIG_VAR(y);
      float CONFIG_VAR(z);
      std::string w;
      char CONFIG_VAR(u);

      struct B {
        int CONFIG_VAR(b);
        int CONFIG_VAR(a);
      };

      B CONFIG_SECTION(b);
    };

    const std::string txt =R"(
    {
      "y" : true,
      "z" : 0.1,
      "u" : "A",
      "b" : {
        "a" : 10,
        "b" : 20
      }
    }
                           )";
    A a;
    neutrino::utils::config_parser::parse (a, txt, neutrino::utils::config_parser::JSON);
    REQUIRE(a.y.has_value());
    REQUIRE(*a.y);
    REQUIRE(a.z == doctest::Approx(0.1).epsilon(0.00001));
    REQUIRE(a.u == 'A');
    REQUIRE(a.b.b == 20);
    REQUIRE(a.b.a == 10);
  }

  TEST_CASE("test ini parser with simple arrays") {
    struct A {
      std::vector<int> CONFIG_VAR(a);
    };

    const std::string txt =R"(
    {
      "a" : [1,2,3]
    }
                           )";
    A a;
    neutrino::utils::config_parser::parse (a, txt, neutrino::utils::config_parser::JSON);
    REQUIRE(a.a.size() == 3);
    REQUIRE(a.a[1] == 2);
  }

  TEST_CASE("test ini parser with complex arrays") {
    struct A {
      struct B {
        int CONFIG_VAR(b);
        int CONFIG_VAR(a);
      };
      std::vector<B> CONFIG_VAR(a);
    };

    const std::string txt =R"(
    {
      "a" : [
        {"a" : 1, "b" : 11},
        {"a" : 2, "b" : 22},
        {"a" : 3, "b" : 33}
      ]
    }
                           )";
    A a;
    neutrino::utils::config_parser::parse (a, txt, neutrino::utils::config_parser::JSON);
    REQUIRE(a.a.size() == 3);
    REQUIRE(a.a[1].a == 2);
    REQUIRE(a.a[1].b == 22);
  }

  TEST_CASE("parse ini to map") {
    const std::string txt = R"(
 ---
 doe: "a deer, a female deer"
 ray: "a drop of golden sun"
 pi: 3.14159
 xmas: true
 french-hens: 3
 calling-birds:
   - huey
   - dewey
   - louie
   - fred
 xmas-fifth-day:
   calling-birds: four
   french-hens: 3
   golden-rings: 5
   partridges:
     count: 1
     location: "a pear tree"
   turtle-doves: two
)";
    using cfg_t = std::map<std::string, std::any>;
    std::vector<cfg_t> out;
    neutrino::utils::config_parser::parse (out, txt, neutrino::utils::config_parser::YAML);

    REQUIRE(out.size() == 1);

    auto& mp = out[0];
    REQUIRE(std::any_cast<std::string>(mp["doe"]) == "a deer, a female deer");
    auto vec = std::any_cast<std::vector<std::any>>(mp["calling-birds"]);
    REQUIRE(vec.size() == 4);
    REQUIRE(std::any_cast<std::string>(vec[0]) == "huey");
    auto c = std::any_cast<cfg_t>(mp["xmas-fifth-day"]);
    REQUIRE(std::any_cast<int>(c["french-hens"]) == 3);
    auto c1 = std::any_cast<cfg_t>(c["partridges"]);
    REQUIRE(std::any_cast<int>(c1["count"]) == 1);
  }

  TEST_CASE("test ini parser") {
    const auto data = R"(; This is an INI file
[section1]  ; section comment
one=This is a test  ; name=value comment
two = 1234
; x=y

[ section 2 ]
happy  =  4
sad =

[empty]
; do nothing

[comment_test]
test2 = 2
test3 = 2

)";
    auto cfg = neutrino::utils::detail::load_from_ini (data);

    REQUIRE(cfg["section1"]["two"].get<int>() == 1234);
    REQUIRE(cfg["comment_test"]["test2"].get<int>() == 2);
    REQUIRE(cfg["comment_test"]["test3"].get<int>() == 2);
  }
}
