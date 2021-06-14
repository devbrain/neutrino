#include <neutrino/utils/sorted_array.hh>
#include <doctest/doctest.h>

TEST_CASE("sorted_array - test insert")
{
    using vec_t = std::vector<int>;
    vec_t v;

    neutrino::utils::sorted_array<vec_t>::insert(v, 3);
    neutrino::utils::sorted_array<vec_t>::insert(v, 2);
    neutrino::utils::sorted_array<vec_t>::insert(v, 1);

    REQUIRE(v.size() == 3);
    REQUIRE(v[0] == 1);
    REQUIRE(v[1] == 2);
    REQUIRE(v[2] == 3);

    v.resize(0);


    neutrino::utils::sorted_array<vec_t>::insert(v, 1);
    neutrino::utils::sorted_array<vec_t>::insert(v, 2);
    neutrino::utils::sorted_array<vec_t>::insert(v, 3);

    REQUIRE(v.size() == 3);
    REQUIRE(v[0] == 1);
    REQUIRE(v[1] == 2);
    REQUIRE(v[2] == 3);


    v.resize(0);


    neutrino::utils::sorted_array<vec_t>::insert(v, 3);
    neutrino::utils::sorted_array<vec_t>::insert(v, 1);
    neutrino::utils::sorted_array<vec_t>::insert(v, 2);


    REQUIRE(v.size() == 3);
    REQUIRE(v[0] == 1);
    REQUIRE(v[1] == 2);
    REQUIRE(v[2] == 3);

}

TEST_CASE("sorted_array - test find")
{
    using vec_t = std::vector<int>;
    vec_t v {2,3,1};
    neutrino::utils::sorted_array<vec_t>::sort(v);

    REQUIRE(neutrino::utils::sorted_array<vec_t>::exists(v, 1));
    REQUIRE(neutrino::utils::sorted_array<vec_t>::exists(v, 2));
    REQUIRE(neutrino::utils::sorted_array<vec_t>::exists(v, 3));

}

TEST_CASE("sorted_array - test erase")
{
    using vec_t = std::vector<int>;
    vec_t v {2,3,1};
    neutrino::utils::sorted_array<vec_t>::sort(v);

    neutrino::utils::sorted_array<vec_t>::erase(v, 1);
    REQUIRE(v.size() == 2);
    REQUIRE(v[0] == 2);
    REQUIRE(v[1] == 3);

    v = {1,2,3};

    neutrino::utils::sorted_array<vec_t>::erase(v, 2);
    REQUIRE(v.size() == 2);
    REQUIRE(v[0] == 1);
    REQUIRE(v[1] == 3);


    v = {1,2,3};

    neutrino::utils::sorted_array<vec_t>::erase(v, 3);
    REQUIRE(v.size() == 2);
    REQUIRE(v[0] == 1);
    REQUIRE(v[1] == 2);

    v = {1,2,3};

    neutrino::utils::sorted_array<vec_t>::erase(v, 3);
    neutrino::utils::sorted_array<vec_t>::erase(v, 1);
    neutrino::utils::sorted_array<vec_t>::erase(v, 2);

    REQUIRE(v.size() == 0);
}
