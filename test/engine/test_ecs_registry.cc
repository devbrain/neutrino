//
// Created by igor on 01/05/2022.
//

#include <doctest/doctest.h>
#include <neutrino/kernel/ecs/registry.hh>

namespace {
  struct A {
    int x;
  };

  struct B {
    int x, y;
  };

  struct C {
    int x, y, z;
  };
}

TEST_SUITE("test ecs registry") {

  TEST_CASE("test 1") {
    neutrino::ecs::registry reg;

    reg.attach_component (0, A{1});
    ENFORCE(reg.has_component<A>(0));
    REQUIRE(reg.get_component<A>(0).x == 1);

    reg.attach_component (0, B{1, 2});
    ENFORCE(reg.has_component<B>(0));
    REQUIRE(reg.get_component<B>(0).y == 2);
  }

  TEST_CASE("test 2") {
    neutrino::ecs::registry reg;

    reg.attach_component (0, A{1});
    ENFORCE(reg.has_component<A>(0));
    REQUIRE(reg.get_component<A>(0).x == 1);

    reg.attach_component (0, B{1, 2});
    ENFORCE(reg.has_component<B>(0));
    REQUIRE(reg.get_component<B>(0).y == 2);

    reg.get_component<B>(0).y = 10;
    REQUIRE(reg.get_component<B>(0).y == 10);

    reg.attach_component (1, B{3, 4});
    ENFORCE(reg.has_component<B>(1));
    REQUIRE(reg.get_component<B>(1).y == 4);

    reg.detach_component<B>(0);
    ENFORCE(!reg.has_component<B>(0));
    ENFORCE(reg.has_component<B>(1));
    ENFORCE(reg.has_component<A>(0));
    REQUIRE(reg.get_component<A>(0).x == 1);

    reg.delete_entity (0);
    ENFORCE(!reg.exists (0));
    ENFORCE(reg.exists (1));
  }

  TEST_CASE("test view 1") {
    neutrino::ecs::registry reg;

    reg.attach_component (0, A{10});
    reg.attach_component (0, B{1, 2});

    reg.attach_component (1, A{11});
    reg.attach_component (1, B{3, 4});

    reg.attach_component (2, A{12});

    auto v = reg.query<A, B>();

    for (const auto id : v) {
      REQUIRE((id == 0 || id == 1));
    }

    auto v2 = reg.query<A>();
    for (const auto id : v2) {
      REQUIRE((id == 0 || id == 1 || id == 2));
    }
  }

  TEST_CASE("test query for_each") {
    neutrino::ecs::registry reg;

    auto e0 = reg.create_id();
    reg.attach_component (e0, A{10});
    reg.attach_component (e0, B{1, 2});

    auto e1 = reg.create_id();
    reg.attach_component (e1, A{11});
    reg.attach_component (e1, B{3, 4});

    auto e2 = reg.create_id();
    reg.attach_component (e2, A{11});
    reg.attach_component (e2, B{3, 4});
    reg.attach_component (e2, C{3, 4, 5});


    reg.for_each<A,C>([=](auto id) {
      REQUIRE((id == e0 || id == e2));
    });

    reg.erase<C>();
    reg.for_each<A>([=](neutrino::ecs::id_t id) {
      REQUIRE((id == e0 || id == e1));
    });

    auto v = reg.query<C>();
    REQUIRE((v.begin() == v.end()));
  }

  TEST_CASE("test query for_each 2") {
    neutrino::ecs::registry reg;

    auto e0 = reg.create_id ();
    reg.attach_component (e0, A{10});
    reg.attach_component (e0, B{1, 2});

    auto e1 = reg.create_id ();
    reg.attach_component (e1, A{11});
    reg.attach_component (e1, B{3, 4});

    auto e2 = reg.create_id ();
    reg.attach_component (e2, A{110});
    reg.attach_component (e2, C{30, 4, 5});

    int count = 0;
    reg.for_each<A,B>([&](auto id, const A& a, const B& b) {
      REQUIRE((id == e0 || id == e1));
      REQUIRE((a.x == 10 || a.x == 11));
      REQUIRE((b.x == 1 || b.x == 3));
      count++;
    });
    REQUIRE(count == 2);
  }
}