//
// Created by igor on 05/06/2020.
//

#include <doctest/doctest.h>
#include <string>
#include <neutrino/utils/mp/introspection.hh>

namespace
{
    struct A
    {
        static constexpr int zopa = 0xAA;
        int f(char*);
        static int g(float);
        static int k(float);

        float ff(const std::string& x);
    };
}

GENERATE_HAS_MEMBER_FUNCTION(f);
GENERATE_HAS_MEMBER_FUNCTION(g);
TEST_CASE ("Test member function")
{
            REQUIRE(has_f<A, int(char*)>::value);
            REQUIRE(!has_g<A, int(char*)>::value);
            REQUIRE(has_g<A, int(float)>::value);
}

GENERATE_HAS_MEMBER_STATIC(zopa);

GENERATE_HAS_MEMBER_STATIC(k);

TEST_CASE ("Test static decl")
{
            REQUIRE(has_zopa<A, int>::value);
            REQUIRE(has_k<A, decltype(A::k)>::value);
}

GENERATE_HAS_MEMBER_WEAK(ff);

TEST_CASE("Test member function by args")
{
    REQUIRE(has_ff<A, const std::string&>::value);
    REQUIRE(std::is_same<return_type_of_ff<A, const std::string&>, float>::value);
    REQUIRE(!std::is_same<return_type_of_ff<A, const std::string&>, double>::value);
    REQUIRE(!has_ff<A, int>::value);
}
