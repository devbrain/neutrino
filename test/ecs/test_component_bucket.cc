//
// Created by igor on 7/26/24.
//
#include <set>
#include <sstream>
#include <doctest/doctest.h>
#include <neutrino/ecs/detail/component_bucket.hh>
#include <utility>

using namespace neutrino::ecs::detail;

struct A {
    A(std::string s, int i)
        : s(std::move(s)),
          i(i) {
    }
    ~A() {
        destruct_count++;
    }
    static int destruct_count;

    std::string s;
    int i;
};

int A::destruct_count = 0;

bool operator<(const A& lhs, const A& rhs) {
    if (lhs.s < rhs.s)
        return true;
    if (rhs.s < lhs.s)
        return false;
    return lhs.i < rhs.i;
}

bool operator==(const A& lhs, const A& rhs) {
    return lhs.s == rhs.s
           && lhs.i == rhs.i;
}

bool operator!=(const A& lhs, const A& rhs) {
    return !(lhs == rhs);
}

TEST_SUITE("Test components bucket") {
    TEST_CASE("test creation and iteration") {
        auto bucket = typed_component_bucket <A>::create(10);
        std::set<A> created;
        for (int i = 0; i < 10; i++) {
            std::ostringstream os;
            os << "s-" << i;
            created.emplace(os.str(), i);
            typed_component_bucket <A>::construct(*bucket, neutrino::ecs::entity_id_t(i), os.str(), i);
        }
        REQUIRE_EQ(10, bucket->size());
        REQUIRE_THROWS(typed_component_bucket<A>::construct(*bucket, neutrino::ecs::entity_id_t(100), "", 100));
        auto itr = typed_component_bucket <A>::iterator(*bucket);
        int count = 0;
        std::set<A> extracted;
        while (itr.has_next()) {
            const auto& elt = itr.get_next();
            REQUIRE_EQ(itr.get_entity().value_of(), elt.i);
            extracted.insert(elt);
            count++;
        }
        REQUIRE(count == 10);
        REQUIRE_EQ(created, extracted);
        for (int i = 0; i < 10; i++) {
            typed_component_bucket <A>::destruct(*bucket, neutrino::ecs::entity_id_t(i));
        }
        REQUIRE_EQ(10, A::destruct_count);
        A::destruct_count = 0;
        REQUIRE_EQ(0, bucket->size());
    }

    TEST_CASE("test iteration on empty bucket") {
        auto bucket = typed_component_bucket <A>::create(10);
        auto itr = typed_component_bucket <A>::iterator(*bucket);
        int count = 0;
        while (itr.has_next()) {
            itr.get_next();
        }
        REQUIRE(count == 0);
    }

    TEST_CASE("test iteration where free on start") {
        auto bucket = typed_component_bucket <A>::create(10);
        std::set<A> created;
        for (int i = 0; i < 10; i++) {
            std::ostringstream os;
            os << "s-" << i;
            if (i< 7) {
                created.emplace(os.str(), i);
            }
            typed_component_bucket <A>::construct(*bucket, neutrino::ecs::entity_id_t(i), os.str(), i);
        }
        A::destruct_count = 0;
        for (int i = 7; i < 10; i++) {
            typed_component_bucket <A>::destruct(*bucket, neutrino::ecs::entity_id_t(i));
        }
        REQUIRE_EQ(3, A::destruct_count);
        REQUIRE_EQ(7, bucket->size());

        auto itr = typed_component_bucket <A>::iterator(*bucket);
        int count = 0;
        std::set<A> extracted;
        while (itr.has_next()) {
            const auto& elt = itr.get_next();
            REQUIRE_EQ(itr.get_entity().value_of(), elt.i);
            extracted.insert(elt);
            count++;
        }
        REQUIRE(count == 7);
        REQUIRE_EQ(created, extracted);
    }

    TEST_CASE("test iteration where free on end") {
        auto bucket = typed_component_bucket <A>::create(10);
        std::set<A> created;
        for (int i = 0; i < 10; i++) {
            std::ostringstream os;
            os << "s-" << i;
            if (i >= 3) {
                created.emplace(os.str(), i);
            }
            typed_component_bucket <A>::construct(*bucket, neutrino::ecs::entity_id_t(i), os.str(), i);
        }
        A::destruct_count = 0;
        for (int i = 0; i < 3; i++) {
            typed_component_bucket <A>::destruct(*bucket, neutrino::ecs::entity_id_t(i));
        }
        REQUIRE_EQ(3, A::destruct_count);
        REQUIRE_EQ(7, bucket->size());

        auto itr = typed_component_bucket <A>::iterator(*bucket);
        int count = 0;
        std::set<A> extracted;
        while (itr.has_next()) {
            const auto& elt = itr.get_next();
            REQUIRE_EQ(itr.get_entity().value_of(), elt.i);
            extracted.insert(elt);
            count++;
        }
        REQUIRE(count == 7);
        REQUIRE_EQ(created, extracted);
    }

    TEST_CASE("test iteration where free in middle") {
        auto bucket = typed_component_bucket <A>::create(10);
        std::set<A> created;
        for (int i = 0; i < 10; i++) {
            std::ostringstream os;
            os << "s-" << i;
            if (i < 3 || i >= 7) {
                created.emplace(os.str(), i);
            }
            typed_component_bucket <A>::construct(*bucket, neutrino::ecs::entity_id_t(i), os.str(), i);
        }
        A::destruct_count = 0;
        for (int i = 3; i < 7; i++) {
            typed_component_bucket <A>::destruct(*bucket, neutrino::ecs::entity_id_t(i));
        }

        REQUIRE_EQ(4, A::destruct_count);
        REQUIRE_EQ(6, bucket->size());

        auto itr = typed_component_bucket <A>::iterator(*bucket);
        int count = 0;
        std::set<A> extracted;
        while (itr.has_next()) {
            const auto& elt = itr.get_next();
            REQUIRE_EQ(itr.get_entity().value_of(), elt.i);
            extracted.insert(elt);
            count++;
        }
        REQUIRE(count == 6);
        REQUIRE_EQ(created, extracted);
    }

    TEST_CASE("test iteration where free alternates 1") {
        auto bucket = typed_component_bucket <A>::create(10);
        std::set<A> created;
        for (int i = 0; i < 10; i++) {
            std::ostringstream os;
            os << "s-" << i;
            if (i % 2 == 0) {
                created.emplace(os.str(), i);
            }
            typed_component_bucket <A>::construct(*bucket, neutrino::ecs::entity_id_t(i), os.str(), i);
        }
        A::destruct_count = 0;
        for (int i = 0; i < 10; i++) {
            if (i % 2 == 1) {
                typed_component_bucket <A>::destruct(*bucket, neutrino::ecs::entity_id_t(i));
            }
        }

        REQUIRE_EQ(5, A::destruct_count);
        REQUIRE_EQ(5, bucket->size());

        auto itr = typed_component_bucket <A>::iterator(*bucket);
        int count = 0;
        std::set<A> extracted;
        while (itr.has_next()) {
            const auto& elt = itr.get_next();
            REQUIRE_EQ(itr.get_entity().value_of(), elt.i);
            extracted.insert(elt);
            count++;
        }
        REQUIRE(count == 5);
        REQUIRE_EQ(created, extracted);
    }

    TEST_CASE("test iteration where free alternates 2") {
        auto bucket = typed_component_bucket <A>::create(10);
        std::set<A> created;
        for (int i = 0; i < 10; i++) {
            std::ostringstream os;
            os << "s-" << i;
            if (i % 2 == 1) {
                created.emplace(os.str(), i);
            }
            typed_component_bucket <A>::construct(*bucket, neutrino::ecs::entity_id_t(i), os.str(), i);
        }
        A::destruct_count = 0;
        for (int i = 0; i < 10; i++) {
            if (i % 2 == 0) {
                typed_component_bucket <A>::destruct(*bucket, neutrino::ecs::entity_id_t(i));
            }
        }

        REQUIRE_EQ(5, A::destruct_count);
        REQUIRE_EQ(5, bucket->size());

        auto itr = typed_component_bucket <A>::iterator(*bucket);
        int count = 0;
        std::set<A> extracted;
        while (itr.has_next()) {
            const auto& elt = itr.get_next();
            REQUIRE_EQ(itr.get_entity().value_of(), elt.i);
            extracted.insert(elt);
            count++;
        }
        REQUIRE(count == 5);
        REQUIRE_EQ(created, extracted);
    }
}
