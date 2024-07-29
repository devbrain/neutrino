//
// Created by igor on 7/28/24.
//

#include <doctest/doctest.h>
#include <neutrino/ecs/registry.hh>

using namespace neutrino::ecs;

namespace {
    struct A {
        explicit A(int x)
            : x(x) {
        }

        int x;
    };

    struct B {
        explicit B(float x)
            : x(x) {
        }

        float x;
    };

    struct C {
        explicit C(bool x)
            : x(x) {
        }

        bool x;
    };
}

TEST_SUITE("ECS Registry test") {
    TEST_CASE("Test simple") {
        registry r;

        entity_builder b(r);
        auto e1 = b.with_component <A>(10).with_component <B>(3.14).build();
        auto e2 = b.with_component <A>(100).with_component <C>(true).build();

        REQUIRE(r.exists(e1));
        REQUIRE(r.exists(e2));

        REQUIRE(r.has_component<A>(e1));
        REQUIRE(r.has_component<B>(e1));
        REQUIRE_FALSE(r.has_component<C>(e1));

        REQUIRE(r.has_component<A>(e2));
        REQUIRE(r.has_component<C>(e2));
        REQUIRE_FALSE(r.has_component<B>(e2));

        REQUIRE_EQ(10, r.get_component<A>(e1).x);
        REQUIRE_EQ(100, r.get_component<A>(e2).x);
        auto e1_components = r.list_components(e1);
        REQUIRE(e1_components.size() == 2);
    }

    TEST_CASE("Test iterator") {
        registry r;

        entity_builder b(r);
        auto e1 = b.with_component <A>(10).with_component <B>(3.14).build();
        auto e2 = b.with_component <A>(100).with_component <C>(true).build();

        auto itr = r.iterator <A, B>();
        while (itr.has_next()) {
            bool all_is_full = true;
            auto z = itr.cnext(&all_is_full);
            auto eid = std::get <0>(z);
            if (all_is_full) {
                REQUIRE(eid == e1);
            } else {
                REQUIRE(eid == e2);
            }
        }

        r.bind_component <B>(e2, -1.0);

        auto itr2 = r.iterator <A, B>();
        int count = 0;
        while (itr2.has_next()) {
            bool all_is_full = false;
            count++;
            auto z = itr2.cnext(&all_is_full);
            REQUIRE(all_is_full);
        }
        REQUIRE(count == 2);

        count = 0;
        auto itr3 = r.iterator <A>();
        while (itr3.has_next()) {
            bool all_is_full = true;
            auto z = itr3.cnext(&all_is_full);
            auto eid = std::get <0>(z);
            count++;
            REQUIRE(all_is_full);
            if (eid == e1) {
                REQUIRE(std::get<1>(z)->x == 10);
            } else {
                REQUIRE(std::get<1>(z)->x == 100);
            }
        }
        REQUIRE(count == 2);

        count = 0;

        r.iterate([&count, e1](entity_id_t eid, const A& a, const B& b) {
            count++;
            if (eid == e1) {
                REQUIRE(a.x == 10);
                REQUIRE(b.x == doctest::Approx(3.14));
            } else {
                REQUIRE(a.x == 100);
                REQUIRE(b.x == doctest::Approx(-1.0));
            }
        });

        REQUIRE(count == 2);

        count = 0;

        r.iterate([&count, e1](entity_id_t eid, const A& a) {
            count++;
            if (eid == e1) {
                REQUIRE(a.x == 10);
            } else {
                REQUIRE(a.x == 100);
            }
        });

        REQUIRE(count == 2);
    }

    TEST_CASE("Test remove") {
        registry r;

        entity_builder b(r);
        auto e1 = b.with_component <A>(10).with_component <B>(3.14).build();
        auto e2 = b.with_component <A>(100).with_component <C>(true).build();
        auto e3 = b.with_component <A>(1000).with_component <B>(2.71).build();

        r.remove_entity(e2);
        int count = 0;
        r.iterate([&count, e2](entity_id_t eid, const A& a) {
            count++;
            REQUIRE(eid != e2);
        });
        REQUIRE(count == 2);
        count = 0;
        r.remove_entity(e3);
        r.iterate([&count, e2, e3](entity_id_t eid, const A& a) {
            count++;
            REQUIRE(eid != e2);
            REQUIRE(eid != e3);
        });
        REQUIRE(count == 1);
    }
}
