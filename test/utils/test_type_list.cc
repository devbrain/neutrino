#include <string>
#include <vector>
#include <type_traits>
#include <neutrino/utils/mp/typelist.hh>
#include <doctest/doctest.h>

using namespace neutrino::mp;

TEST_CASE("Test typelist size")
{
    using t0 = type_list<>;
    using t1 = type_list<int>;
    using t2 = type_list<int, float>;
    using t3 = type_list<int, int>;
    REQUIRE((t0::size() == 0));
    REQUIRE((t1::size() == 1));
    REQUIRE((t2::size() == 2));
    REQUIRE((t3::size() == 2));
}

TEST_CASE("Test type_at")
{
    using t = type_list<int, float, bool>;
    REQUIRE(std::is_same_v<int, type_list_at_t<0, t>>);
    REQUIRE(std::is_same_v<float, type_list_at_t<1, t>>);
    REQUIRE(std::is_same_v<bool, type_list_at_t<2, t>>);
}

TEST_CASE("Test append")
{
    using t0 = type_list<>;
    using t0_1 = type_list_append_t <int, t0>;

    using t1 = type_list_append_t <float, t0_1>;
    using t2 = type_list_append_t <bool, t1>;

    REQUIRE(t0_1::size() == 1);
    REQUIRE(std::is_same_v<int, type_list_at_t<0, t0_1>>);
    REQUIRE(t1::size() == 2);
    REQUIRE(std::is_same_v<float, type_list_at_t<1, t1>>);
    REQUIRE(t2::size() == 3);
    REQUIRE(std::is_same_v<bool, type_list_at_t<2, t2>>);
}

TEST_CASE("Test prepend")
{
    using t0 = type_list<>;
    using t0_1 = type_list_prepend_t <int, t0>;

    using t1 = type_list_prepend_t <float, t0_1>;
    using t2 = type_list_prepend_t <bool, t1>;

    REQUIRE(t0_1::size() == 1);
    REQUIRE(std::is_same_v<int, type_list_at_t<0, t0_1>>);
    REQUIRE((t1::size() == 2));
    REQUIRE(std::is_same_v<float, type_list_at_t<0, t1>>);
    REQUIRE(std::is_same_v<int, type_list_at_t<1, t1>>);
    REQUIRE((t2::size() == 3));
    REQUIRE(std::is_same_v<bool, type_list_at_t<0, t2>>);
    REQUIRE(std::is_same_v<float, type_list_at_t<1, t2>>);
    REQUIRE(std::is_same_v<int, type_list_at_t<2, t2>>);
}

TEST_CASE("Test merge")
{
    using t0 = type_list<>;
    using t1 = type_list<int>;

    using t10 = type_list_merge_t<t0, t1>;
    REQUIRE((t10::size() == 1));
    REQUIRE(std::is_same_v<int, type_list_at_t<0, t10>>);

    using t2 = type_list<bool, float>;
    using t102 = type_list_merge_t<t10, t2>;
    REQUIRE((t102::size() == 3));
    REQUIRE(std::is_same_v<int, type_list_at_t<0, t102>>);
    REQUIRE(std::is_same_v<bool, type_list_at_t<1, t102>>);
    REQUIRE(std::is_same_v<float, type_list_at_t<2, t102>>);
}

template <typename T>
struct is_integral
{
    static constexpr bool value() noexcept
    {
        return std::is_integral <T>::value;
    }
};

TEST_CASE("Test filter")
{
    using t1 = type_list <int, bool, float, std::string, std::vector <int>>;
    using t2 = type_list_filter_t<is_integral, t1>;
    REQUIRE((t2::size() == 2));
    REQUIRE(std::is_same_v<int, type_list_at_t<0, t2>>);
    REQUIRE(std::is_same_v<bool, type_list_at_t<1, t2>>);

    using t0 = type_list <float, std::string, std::vector <int>>;
    using t01 = type_list_filter_t<is_integral, t0>;
    REQUIRE((t01::empty()));

    using t00 = type_list <>;
    using t001 = type_list_filter_t<is_integral, t00>;
    REQUIRE((t001::empty()));
}

template <typename X>
struct map
{
    using type = std::vector<X>;
};

TEST_CASE("Test mapper")
{
    using t1 = type_list <int, bool, float>;
    using t2 = type_list_map_t<map, t1>;
    REQUIRE((t2::size() == 3));
    REQUIRE(std::is_same_v<std::vector<int>, type_list_at_t<0, t2>>);
    REQUIRE(std::is_same_v<std::vector<bool>, type_list_at_t<1, t2>>);
    REQUIRE(std::is_same_v<std::vector<float>, type_list_at_t<2, t2>>);

    using t0 = type_list <>;
    using t01 = type_list_map_t<map, t0>;
    REQUIRE((t01::empty()));
}


TEST_CASE("Test to_tuple")
{
    using t1 = type_list <int, bool, float>;
    using t = type_list_to_tuple_t<t1>;

    REQUIRE(std::is_same_v<typename std::tuple_element<0, t>::type, int>);
    REQUIRE(std::is_same_v<typename std::tuple_element<1, t>::type, bool>);
    REQUIRE(std::is_same_v<typename std::tuple_element<2, t>::type, float>);
}

TEST_CASE("Test from_tuple")
{
    using t1 = std::tuple <int, bool, float>;
    using t = tuple_to_type_list_t<t1>;

    REQUIRE(std::is_same_v<type_list_at_t<0, t>, int>);
    REQUIRE(std::is_same_v<type_list_at_t<1, t>, bool>);
    REQUIRE(std::is_same_v<type_list_at_t<2, t>, float>);
}

TEST_CASE("Test find_first")
{
    using t = type_list <int, bool, float>;
    constexpr auto idx = type_list_find_first_v<bool, t>;
    constexpr auto idx0 = type_list_find_first_v<char, t>;

    REQUIRE(idx == 1);
    REQUIRE(idx0 == t::npos);

    using t1 = type_list <int, bool, bool, float>;
    constexpr auto idx1 = type_list_find_first_v<bool, t1>;
    constexpr auto idx01 = type_list_find_first_v<char, t1>;

    REQUIRE(idx1 == 1);
    REQUIRE(idx01 == t::npos);
}

TEST_CASE("Test find_last")
{
    using t = type_list <int, bool, float>;
    constexpr auto idx = type_list_find_last_v<bool, t>;
    constexpr auto idx0 = type_list_find_last_v<char, t>;

    REQUIRE(idx == 1);
    REQUIRE(idx0 == t::npos);

    using t1 = type_list <int, bool, bool, float>;
    constexpr auto idx1 = type_list_find_last_v<bool, t1>;
    constexpr auto idx01 = type_list_find_last_v<char, t1>;

    REQUIRE(idx1 == 2);
    REQUIRE(idx01 == t::npos);
}

TEST_CASE("Test repeat")
{
    using t = type_list_repeat_t<int, 10>;
    
    REQUIRE((t::size () == 10));
    REQUIRE(std::is_same_v<int, type_list_at_t<0, t>>);
    REQUIRE(std::is_same_v<int, type_list_at_t<1, t>>);
    REQUIRE(std::is_same_v<int, type_list_at_t<2, t>>);
    REQUIRE(std::is_same_v<int, type_list_at_t<3, t>>);
    REQUIRE(std::is_same_v<int, type_list_at_t<4, t>>);
    REQUIRE(std::is_same_v<int, type_list_at_t<5, t>>);
    REQUIRE(std::is_same_v<int, type_list_at_t<6, t>>);
    REQUIRE(std::is_same_v<int, type_list_at_t<7, t>>);
    REQUIRE(std::is_same_v<int, type_list_at_t<8, t>>);
    REQUIRE(std::is_same_v<int, type_list_at_t<9, t>>);
}

TEST_CASE("Test flatten")
{
    
    using Types = type_list<int, type_list<float, type_list<double, type_list<char>>>>;
    using Flat = type_list_flatten_t<Types>;

    static_assert(std::is_same_v<Flat, type_list<int, float, double, char>>, "Not the same");
}

TEST_CASE("contains type")
{
    
    using A = type_list<int, float, char>;
    using B = type_list<float>;
    using C = type_list<double>;

    static_assert(type_list_contains_types_v<A, B>);
    static_assert(type_list_contains_types_v<A, A>);
    static_assert(!type_list_contains_types_v<A, C>);
}

TEST_CASE("is_subset1")
{
    
    using t1 = std::tuple<int, double>;
    using t2 = std::tuple<double, int>;
    using t3 = std::tuple<int, double, char>;

    static_assert(is_subset_of_v<t1, t1>, "err");
    static_assert(is_subset_of_v<t1, t2>, "err");
    static_assert(is_subset_of_v<t2, t1>, "err");
    static_assert(is_subset_of_v<t2, t3>, "err");
    static_assert(!is_subset_of_v<t3, t2>, "err");
}

TEST_CASE("is_subset2")
{
    
    using t1 = std::index_sequence <0, 1>;
    using t2 = std::index_sequence <1, 0>;
    using t3 = std::index_sequence <0, 1, 2>;

    static_assert(is_subset_of_v<t1, t1>, "err");
    static_assert(is_subset_of_v<t1, t2>, "err");
    static_assert(is_subset_of_v<t2, t1>, "err");
    static_assert(is_subset_of_v<t2, t3>, "err");
    static_assert(!is_subset_of_v<t3, t2>, "err");
}

TEST_CASE("is_subset3")
{
    
    using t1 = type_list<int, double>;
    using t2 = type_list<double, int>;
    using t3 = type_list<int, double, char>;

    static_assert(is_subset_of_v<t1, t1>, "err");
    static_assert(is_subset_of_v<t1, t2>, "err");
    static_assert(is_subset_of_v<t2, t1>, "err");
    static_assert(is_subset_of_v<t2, t3>, "err");
    static_assert(!is_subset_of_v<t3, t2>, "err");
}
