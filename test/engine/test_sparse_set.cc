//
// Created by igor on 24/09/2020.
//

#include <doctest/doctest.h>
#include <neutrino/kernel/ecs/sparse_set.hh>
#include <neutrino/kernel/ecs/sparse_map.hh>

using sset_t = neutrino::ecs::sparse_set<unsigned int>;

struct component {
  component ()
      : x (0) {
  }

  explicit component (int a)
      : x (a) {
  }

  int x;
};

using smap_t = neutrino::ecs::sparse_map<unsigned int, component>;

TEST_SUITE("sparse_set")
{
  TEST_CASE ("trivial")
  {
    auto pool = std::pmr::new_delete_resource ();
    sset_t x (*pool);

    REQUIRE (x.size () == 0);
    REQUIRE (x.empty ());
  }

  TEST_CASE ("insert 1")
  {
    auto pool = std::pmr::new_delete_resource ();
    sset_t x (*pool);
    x.insert (1);

    REQUIRE (x.size () == 1);
    REQUIRE (!x.empty ());
    REQUIRE(x.exists (1));
  }

  TEST_CASE ("insert 2")
  {
    auto pool = std::pmr::new_delete_resource ();
    sset_t x (*pool);
    x.insert (1);
    x.insert (2);

    REQUIRE (x.size () == 2);

    REQUIRE(x.exists (1));
    REQUIRE(x.exists (2));
    REQUIRE(!x.exists (0));
  }

  TEST_CASE ("insert 3")
  {
    auto pool = std::pmr::new_delete_resource ();
    sset_t x (*pool);
    x.insert (0);
    x.insert (2);
    x.insert (3);

    REQUIRE (x.size () == 3);

    x.erase (0);
    REQUIRE (x.size () == 2);
    REQUIRE(!x.exists (0));
    REQUIRE(x.exists (2));
    REQUIRE(x.exists (3));

    x.erase (2);
    REQUIRE (x.size () == 1);
    REQUIRE(!x.exists (0));
    REQUIRE(!x.exists (2));
    REQUIRE(x.exists (3));

    x.erase (3);
    REQUIRE (x.size () == 0);
    REQUIRE (x.empty ());
    REQUIRE(!x.exists (0));
    REQUIRE(!x.exists (2));
    REQUIRE(!x.exists (3));
  }
}
// ========================================================================================================
TEST_SUITE("sparse_map")
{
  TEST_CASE ("trivial")
  {
    auto pool = std::pmr::new_delete_resource ();
    smap_t x (*pool);

    REQUIRE (x.size () == 0);
    REQUIRE (x.empty ());
  }

  TEST_CASE ("insert 1")
  {
    auto pool = std::pmr::new_delete_resource ();
    smap_t x (*pool);
    x.insert (1, component (0));

    REQUIRE (x.size () == 1);
    REQUIRE (!x.empty ());
    REQUIRE(x.exists (1));
  }

  TEST_CASE ("insert 2")
  {
    auto pool = std::pmr::new_delete_resource ();
    smap_t x (*pool);
    x.insert (1, component (0));
    x.insert (2, component (1));

    REQUIRE (x.size () == 2);

    REQUIRE(x.exists (1));
    REQUIRE(x.exists (2));
    REQUIRE(!x.exists (0));
  }

  TEST_CASE ("insert 3")
  {
    auto pool = std::pmr::new_delete_resource ();
    smap_t x (*pool);
    x.insert (0, component (0));
    x.insert (2, component (1));
    x.insert (3, component (2));

    REQUIRE (x.size () == 3);

    x.erase (0);
    REQUIRE (x.size () == 2);
    REQUIRE(!x.exists (0));
    REQUIRE(x.exists (2));
    REQUIRE(x.exists (3));

    x.erase (2);
    REQUIRE (x.size () == 1);
    REQUIRE(!x.exists (0));
    REQUIRE(!x.exists (2));
    REQUIRE(x.exists (3));

    x.erase (3);
    REQUIRE (x.size () == 0);
    REQUIRE (x.empty ());
    REQUIRE(!x.exists (0));
    REQUIRE(!x.exists (2));
    REQUIRE(!x.exists (3));
  }

  TEST_CASE ("iteration 1")
  {
    auto pool = std::pmr::new_delete_resource ();
    smap_t x (*pool);
    x.insert (0, component (0));
    x.insert (2, component (1));
    x.insert (3, component (2));

    int res = 0 + 1 + 2;
    for (auto i: x) {
      res -= i.second.x;
    }

    REQUIRE (res == 0);
  }

  TEST_CASE ("iteration 2")
  {
    auto pool = std::pmr::new_delete_resource ();
    smap_t x (*pool);
    x.insert (0, component (0));
    x.insert (2, component (1));
    x.insert (3, component (2));

    int res = 0 + 1 + 2;
    x.erase (0);
    for (auto i: x) {
      res -= i.second.x;
    }

    REQUIRE (res == 0);
  }

  TEST_CASE ("iteration 3")
  {
    auto pool = std::pmr::new_delete_resource ();
    smap_t x (*pool);
    x.insert (0, component (0));
    x.insert (2, component (1));
    x.insert (3, component (2));

    int res = 0 + 2;
    x.erase (2);
    for (auto i: x) {
      res -= i.second.x;
    }

    REQUIRE (res == 0);
  }
}
