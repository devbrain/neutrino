#include <doctest/doctest.h>

#include <utils/lru.hh>

#include <optional>
#include <string>

using neutrino::utils::lru_index;

TEST_SUITE("neutrino::utils lru_index") {
    TEST_CASE("pop_oldest drains in least-recently-used order") {
        lru_index<int> lru(8);
        CHECK(lru.empty());
        CHECK(lru.touch(1) == std::nullopt);
        CHECK(lru.touch(2) == std::nullopt);
        CHECK(lru.touch(3) == std::nullopt);
        CHECK(lru.size() == 3);

        CHECK(lru.pop_oldest() == 1);
        CHECK(lru.pop_oldest() == 2);
        CHECK(lru.pop_oldest() == 3);
        CHECK(lru.pop_oldest() == std::nullopt);
        CHECK(lru.empty());
    }

    TEST_CASE("re-touching a present key moves it to most-recently-used") {
        lru_index<int> lru(8);
        lru.touch(1);
        lru.touch(2);
        lru.touch(3);

        CHECK(lru.touch(1) == std::nullopt); // 1 -> MRU; order is now 2, 3, 1
        CHECK(lru.size() == 3);              // no growth, no eviction

        CHECK(lru.pop_oldest() == 2);
        CHECK(lru.pop_oldest() == 3);
        CHECK(lru.pop_oldest() == 1);
    }

    TEST_CASE("a new key past capacity evicts the least-recently-used") {
        lru_index<int> lru(2);
        CHECK(lru.touch(1) == std::nullopt);
        CHECK(lru.touch(2) == std::nullopt);
        CHECK(lru.touch(3) == 1); // over capacity: 1 is evicted
        CHECK(lru.size() == 2);
        CHECK_FALSE(lru.contains(1));
        CHECK(lru.contains(2));
        CHECK(lru.contains(3));

        // Re-touching keeps size at capacity and never evicts.
        CHECK(lru.touch(2) == std::nullopt); // order now 3, 2
        CHECK(lru.touch(4) == 3);            // 3 was LRU after the re-touch
    }

    TEST_CASE("erase removes a key and it is skipped by pop_oldest") {
        lru_index<int> lru(8);
        lru.touch(1);
        lru.touch(2);
        lru.touch(3);

        lru.erase(2);
        CHECK(lru.size() == 2);
        CHECK_FALSE(lru.contains(2));
        lru.erase(99); // absent -> no-op
        CHECK(lru.size() == 2);

        CHECK(lru.pop_oldest() == 1);
        CHECK(lru.pop_oldest() == 3);
        CHECK(lru.pop_oldest() == std::nullopt);
    }

    TEST_CASE("erasing then re-touching restores most-recently-used position") {
        lru_index<std::string> lru(8);
        lru.touch("a");
        lru.touch("b");
        lru.erase("a");
        lru.touch("a"); // a is a fresh MRU insert, not a move

        CHECK(lru.pop_oldest() == "b");
        CHECK(lru.pop_oldest() == "a");
    }

    TEST_CASE("capacity 0 retains nothing") {
        lru_index<int> lru(0);
        CHECK(lru.capacity() == 0);
        CHECK(lru.touch(1) == 1); // evicted immediately, its own key comes back
        CHECK(lru.empty());
        CHECK_FALSE(lru.contains(1));
        CHECK(lru.pop_oldest() == std::nullopt);
    }

    TEST_CASE("clear empties the index") {
        lru_index<int> lru(8);
        lru.touch(1);
        lru.touch(2);
        lru.clear();
        CHECK(lru.empty());
        CHECK(lru.size() == 0);
        CHECK(lru.touch(5) == std::nullopt);
        CHECK(lru.contains(5));
    }
}
