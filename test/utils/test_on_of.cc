//
// Created by igor on 06/06/2021.
//

#include <neutrino/utils/mp/one_of.hh>
#include <neutrino/utils/mp/all_same.hh>

#include <doctest/doctest.h>

struct A {};
static void test_one_of() {
    static_assert(!neutrino::mp::one_of_v<bool, float, int, A>);
    static_assert(neutrino::mp::one_of_v<bool, float, int, A, bool>);
}

static void test_all_same() {
    static_assert(!neutrino::mp::all_same_v<bool, float, int, A>);
    static_assert(neutrino::mp::all_same_v<A,  A, A>);
}

TEST_CASE("test_one_of_all_same") {
    test_all_same();
    test_one_of();
}