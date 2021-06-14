//
// Created by igor on 09/06/2021.
//

#include <doctest/doctest.h>
#include <neutrino/utils/mp/type_name/type_name.hpp>

TEST_CASE("test type name") {
    static_assert(type_name_v<int> == "int");
    REQUIRE(type_hash_v<int> == 340908721);
}