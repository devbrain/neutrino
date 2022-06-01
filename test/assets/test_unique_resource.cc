//
// Created by igor on 29/05/2022.
//

#include <doctest/doctest.h>
#include <string>
#include <neutrino/assets/resources/resource_id.hh>
#include <neutrino/assets/resources/resource.hh>
#include <neutrino/assets/resources/resource_io.hh>

struct ref_cnt {
  ref_cnt()
  : count(0) {
  }

  int count;
};

struct ref_cnt_res {
  ref_cnt* ptr;
  explicit ref_cnt_res(ref_cnt* p) : ptr(p) { ptr->count++; }

  ~ref_cnt_res() {
    ptr->count--;
  }
};

namespace neutrino::assets {
  class test_resource_reader : public resource_reader<ref_cnt_res, ref_cnt*> {
    private:
      std::unique_ptr<ref_cnt_res> do_load(ref_cnt* p) override {
        return std::make_unique<ref_cnt_res>(p);
      }
  };
}

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


TEST_CASE("test resource - out of scope") {
  using namespace neutrino::assets;
  test_resource_reader loader;
  ref_cnt rc;
  {
    auto r1 = loader.load (&rc, resource_id("a"));
    auto d = resource<ref_cnt_res>::get (resource_id("a"));
  }
  bool exists = loader.exists (resource_id("a"));
  REQUIRE(!exists);
  REQUIRE(rc.count == 0);
}

TEST_CASE("test resource - assigment op") {
  using namespace neutrino::assets;
  test_resource_reader loader;
  ref_cnt rc;
  resource<ref_cnt_res> r2;
  {
    auto r1 = loader.load (&rc, resource_id("a"));
    auto d = resource<ref_cnt_res>::get (resource_id("a"));

    REQUIRE(r1.id() == resource_id("a"));
    r2 = r1;
  }
  REQUIRE(r2.id() == resource_id("a"));
  bool exists = loader.exists (resource_id("a"));
  REQUIRE(exists);
  REQUIRE(rc.count == 1);
}

TEST_CASE("test resource - copy ctor") {
  using namespace neutrino::assets;
  test_resource_reader loader;
  ref_cnt rc;
  std::unique_ptr<resource<ref_cnt_res>> r2;
  {
    auto r1 = loader.load (&rc, resource_id("a"));
    auto d = resource<ref_cnt_res>::get (resource_id("a"));

    REQUIRE(r1.id() == resource_id("a"));
    r2 = std::make_unique<resource<ref_cnt_res>>(r1);
  }
  REQUIRE(r2->id() == resource_id("a"));
  bool exists = loader.exists (resource_id("a"));
  REQUIRE(exists);
  REQUIRE(rc.count == 1);
}

TEST_CASE("test resource - move ctor") {
  using namespace neutrino::assets;
  test_resource_reader loader;
  ref_cnt rc;
  std::unique_ptr<resource<ref_cnt_res>> r2;
  {
    auto r1 = loader.load (&rc, resource_id("a"));
    auto d = resource<ref_cnt_res>::get (resource_id("a"));

    REQUIRE(r1.id() == resource_id("a"));
    r2 = std::make_unique<resource<ref_cnt_res>>(std::move(r1));
  }
  REQUIRE(r2->id() == resource_id("a"));
  bool exists = loader.exists (resource_id("a"));
  REQUIRE(exists);
  REQUIRE(rc.count == 1);
}