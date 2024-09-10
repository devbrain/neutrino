//
// Created by igor on 8/31/24.
//

#include <iostream>

#include <doctest/doctest.h>
#include <neutrino/s11n/s11n.hh>
#include <neutrino/s11n/detail/archive.hh>

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

        neutrino::s11n::serialize_to_yaml(a, out);
        auto s = out.str();
        A x;
        std::istringstream is (out.str());
        neutrino::s11n::deserialize_from_yaml(x, is);

        REQUIRE_EQ(a, x);
    }

    TEST_CASE("Test json s11n") {
        std::ostringstream out;
        A a{1, 10, {{"a"}, {"b"}, {"c"}}};

        neutrino::s11n::serialize_to_json(a, out);
        auto s = out.str();
        A x;
        std::istringstream is (out.str());
        neutrino::s11n::deserialize_from_json(x, is);

        REQUIRE_EQ(a, x);
    }

    TEST_CASE("test yaml_serialization_impl") {
        auto s = neutrino::s11n::detail::serialization_archive::create_yaml();

        s.start_object();
        s.write_key("b");
        s.write_value(1);
        s.write_key("c");
        s.write_null();
        s.end_object();
        std::ostringstream out;
        s.serialize(out);
        constexpr auto expected = R"(b: 1
c: ~)";
        REQUIRE_EQ(out.str(), expected);
    }

    TEST_CASE("test json_serialization_impl 1") {
        auto s = neutrino::s11n::detail::serialization_archive::create_json();

        s.start_object();
        s.write_key("b");
        s.write_value(1);
        s.write_key("c");
        s.write_null();
        s.end_object();
        std::ostringstream out;
        s.serialize(out);
        constexpr auto expected = R"({
    "b": 1,
    "c": null
})";
        REQUIRE_EQ(out.str(), expected);
    }
    TEST_CASE("test json_serialization_impl 2") {
        auto s = neutrino::s11n::detail::serialization_archive::create_json();
        s.start_array();
        s.start_array();
        s.end_array();

        s.start_array();
        s.end_array();
        s.end_array();
        std::ostringstream out;
        s.serialize(out);
        constexpr auto expected = R"([
    [],
    []
])";
        REQUIRE_EQ(out.str(), expected);
    }

    TEST_CASE("test json_serialization_impl 3") {
        auto s = neutrino::s11n::detail::serialization_archive::create_json();
        s.start_array();
        s.start_array();
        s.end_array();

        s.start_object();
        s.end_object();
        s.end_array();
        std::ostringstream out;
        s.serialize(out);
        constexpr auto expected = R"([
    [],
    {}
])";
        REQUIRE_EQ(out.str(), expected);
    }

    TEST_CASE("test json_serialization_impl 4") {
        auto s = neutrino::s11n::detail::serialization_archive::create_json();
        s.start_array();
        s.start_array();
        s.write_value(1);
        s.write_value(3.14);
        s.end_array();

        s.start_object();
        s.write_key("a");
        s.write_value(7);
        s.end_object();
        s.end_array();
        std::ostringstream out;
        s.serialize(out);
        constexpr auto expected = R"([
    [
        1,
        3.14
    ],
    {
        "a": 7
    }
])";
        REQUIRE_EQ(out.str(), expected);
//        std::cout << "R\"(" << out.str() << ")\"" << std::endl;
    }

    TEST_CASE("test json_serialization_impl 5") {
        auto s = neutrino::s11n::detail::serialization_archive::create_json();

        s.start_array();
        s.start_array();
        s.write_value(1);
        s.write_value(3.14);
        s.end_array();

        s.start_object();
        s.write_key("a");
        s.write_value(7);
        s.write_key("b");
        s.start_array();
        s.write_value(false);
        s.write_value(true);
        s.end_array();
        s.end_object();
        s.end_array();
        std::ostringstream out;
        s.serialize(out);
        constexpr auto expected = R"([
    [
        1,
        3.14
    ],
    {
        "a": 7,
        "b": [
            false,
            true
        ]
    }
])";
        REQUIRE_EQ(out.str(), expected);
    }

    TEST_CASE("test json_serialization_impl 6") {
        auto s = neutrino::s11n::detail::serialization_archive::create_json();
        s.start_object();
        s.write_key("a");
        s.write_value(7);
        s.write_key("b");
        s.start_array();
        s.write_value(false);
        s.write_value(true);
        s.end_array();
        s.end_object();
        std::ostringstream out;
        s.serialize(out);
        constexpr auto expected = R"({
    "a": 7,
    "b": [
        false,
        true
    ]
})";
        REQUIRE_EQ(out.str(), expected);
        //std::cout << "R\"(" << out.str() << ")\"" << std::endl;
    }
}