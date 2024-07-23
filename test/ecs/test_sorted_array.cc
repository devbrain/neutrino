//
// Created by igor on 7/22/24.
//
#include <array>
#include <iterator>
#include <random>
#include <chrono>
#include <algorithm>
#include <doctest/doctest.h>
#include <neutrino/ecs/detail/sorted_array.hh>

template< class Iter >
void fill_with_random_int_values( Iter start, Iter end)
{
	int min = 1;
	for (auto i = start; i != end; ++i) {
		*i = min++;
	}
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::shuffle(start, end, std::default_random_engine(seed));
}


using namespace neutrino::ecs::detail;

TEST_SUITE("sorted array") {
	TEST_CASE("test insertion fixed") {
		fixed_sorted_array<int, 16> arr;
		REQUIRE(arr.insert(100));
		REQUIRE(arr.insert(300));
		REQUIRE(arr.insert(200));
		REQUIRE_EQ(0, arr.index_of(100));
		REQUIRE_EQ(1, arr.index_of(200));
		REQUIRE_EQ(2, arr.index_of(300));
	}

	TEST_CASE("test insertion long fixed") {
		std::array<int, 100> rnd{};
		fill_with_random_int_values(rnd.begin(), rnd.end());
		fixed_sorted_array<int, 16> arr;
		for (int i=0; i<100; i++) {
			if (i < 16) {
				REQUIRE(arr.insert(rnd[i]));
			} else {
				REQUIRE_FALSE(arr.insert(rnd[i]));
			}
		}
		std::sort(rnd.begin(), rnd.begin() + 16);
		for (int i=0; i<16; i++) {
			REQUIRE_EQ(i, arr.index_of(rnd[i]));
		}
	}

	TEST_CASE("test insertion") {
		sorted_array<int> arr(16);
		arr.insert(100);
		arr.insert(300);
		arr.insert(200);
		REQUIRE_EQ(0, arr.index_of(100));
		REQUIRE_EQ(1, arr.index_of(200));
		REQUIRE_EQ(2, arr.index_of(300));
	}

	TEST_CASE("test insertion long") {
		std::array<int, 100> rnd{};
		fill_with_random_int_values(rnd.begin(), rnd.end());
		sorted_array<int> arr;
		for (int i=0; i<100; i++) {
			arr.insert(rnd[i]);
		}
		std::sort(rnd.begin(), rnd.end());
		for (int i=0; i<100; i++) {
			REQUIRE_EQ(i, arr.index_of(rnd[i]));
		}
	}

	TEST_CASE("test erase") {
		std::array<int, 100> rnd{};
		fill_with_random_int_values(rnd.begin(), rnd.end());
		sorted_array<int> arr;
		for (int i=0; i<100; i++) {
			arr.insert(rnd[i]);
		}

		for (int i=0; i<50; i++) {
			int old;
			REQUIRE(arr.remove(rnd[i], &old));
			REQUIRE_EQ(old, rnd[i]);
			REQUIRE_FALSE(arr.is_valid_index(arr.index_of(rnd[i])));
		}

		for (int i=0; i<50; i++) {
			REQUIRE_FALSE(arr.remove(rnd[i]));
		}

		for (int i=50; i<100; i++) {
			int old;
			REQUIRE(arr.remove(rnd[i], &old));
			REQUIRE_EQ(old, rnd[i]);
			REQUIRE_FALSE(arr.is_valid_index(arr.index_of(rnd[i])));
		}

	}
}