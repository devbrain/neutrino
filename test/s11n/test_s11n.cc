//
// Created by igor on 8/31/24.
//

#include <doctest/doctest.h>
#include <neutrino/s11n/s11n.hh>

namespace {
    struct B {
        friend bool operator==(const B& lhs, const B& rhs) {
            return lhs.a == rhs.a;
        }

        friend bool operator!=(const B& lhs, const B& rhs) {
            return !(lhs == rhs);
        }

        std::string a;
        SERIALIZATION_SCHEMA(a)
    };
    struct A {
        friend bool operator==(const A& lhs, const A& rhs) {
            return lhs.x == rhs.x
                   && lhs.y == rhs.y
                   && lhs.b == rhs.b;
        }

        friend bool operator!=(const A& lhs, const A& rhs) {
            return !(lhs == rhs);
        }

        int x;
        int y;
        std::vector<B> b;

        SERIALIZATION_SCHEMA(x, y, b)
    };

}

TEST_SUITE("Test s11n") {
    TEST_CASE("Test yaml s11n") {
        std::ostringstream out;
        A a{1, 10, {{"a"}, {"b"}, {"c"}}};

        neutrino::serialize_to_yaml(a, out);
        auto s = out.str();
        A x;
        std::istringstream is (out.str());
        neutrino::deserialize_from_yaml(x, is);

        REQUIRE_EQ(a, x);
    }

    TEST_CASE("Test json s11n") {
        std::ostringstream out;
        A a{1, 10, {{"a"}, {"b"}, {"c"}}};

        neutrino::serialize_to_json(a, out);
        auto s = out.str();
        A x;
        std::istringstream is (out.str());
        neutrino::deserialize_from_json(x, is);

        REQUIRE_EQ(a, x);

    }
}