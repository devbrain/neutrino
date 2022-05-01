//
// Created by igor on 01/05/2022.
//

#include <doctest/doctest.h>
#include <iostream>
#include <neutrino/kernel/ecs/registry.hh>

namespace {
  struct A {
    int x;
  };

  struct B {
    int x, y;
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
      std::cout << id << std::endl;
    }
    REQUIRE(true);


  }
}