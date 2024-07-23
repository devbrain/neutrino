//
// Created by igor on 7/23/24.
//

#include <doctest/doctest.h>
#include <neutrino/ecs/detail/bi_map.hh>

using namespace neutrino::ecs::detail;

TEST_SUITE("Test bimap") {
	TEST_CASE("test insertion") {
		std::vector <std::pair <int, double>> data = {
			{1, 0.1},
			{2, 0.2},
			{3, 0.3},
			{4, 0.4},
		};
		bi_map <int, double> bm(16);
		for (const auto& [k, v] : data) {
			bm.bind(k, v);
		}

		for (const auto& [k, v] : data) {
			REQUIRE(bm.key_exists(k));
			REQUIRE(bm.value_exists(v));
			REQUIRE_EQ(k, bm.get_key_by_value(v));
			REQUIRE_EQ(v, bm.get_value_by_key(k));
		}
	}

	TEST_CASE("test removal") {
		std::vector <std::pair <int, double>> data;
		for (int i=0; i<100; i++) {
			data.emplace_back(i, i/10.0);
		}
		bi_map <int, double> bm(16);
		for (const auto& [k, v] : data) {
			bm.bind(k, v);
		}
		for (const auto& [k, v] : data) {
			REQUIRE(bm.key_exists(k));
			REQUIRE(bm.value_exists(v));
			REQUIRE_EQ(k, bm.get_key_by_value(v));
			REQUIRE_EQ(v, bm.get_value_by_key(k));
		}
		for (int i = 0; i <50; i++) {
			if (i % 2 == 0) {
				REQUIRE(bm.remove_by_key(data[i].first));
			} else {
				REQUIRE(bm.remove_by_value(data[i].second));
			}
		}
		for (int i = 0; i <50; i++) {
			REQUIRE_FALSE(bm.key_exists(data[i].first));
			REQUIRE_FALSE(bm.value_exists(data[i].second));
		}

		REQUIRE_EQ(50, bm.size());
		for (int i = 0; i <100; i++) {
			if (i < 50) {
				if (i % 2 == 0) {
					REQUIRE_FALSE(bm.remove_by_value(data[i].second));
				} else {
					REQUIRE_FALSE(bm.remove_by_key(data[i].first));
				}
			} else {
				REQUIRE(bm.remove_by_key(data[i].first));
				REQUIRE_FALSE(bm.value_exists(data[i].second));
			}
		}
		REQUIRE_EQ(0, bm.size());
		REQUIRE(bm.empty());
	}
}
