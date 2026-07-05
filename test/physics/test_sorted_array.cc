//
// Created by igor on 21/06/2026.
//
// Tests for the flat sorted-set containers in <neutrino/physics/collide/sorted_array.hh>:
// sorted_array<T> (dynamic) and fixed_sorted_array<T, N> (inline, bounded). Both keep
// elements in ascending order with no duplicates; the suite checks ordering, uniqueness,
// lookup, removal, capacity behaviour, bounds-precondition guards (ENFORCE throws), and a
// randomized cross-check against std::set as ground truth.
//
#include <doctest/doctest.h>

#include <cstdint>
#include <memory>
#include <set>
#include <vector>

#include <neutrino/physics/collide/sorted_array.hh>

using namespace neutrino::physics;

namespace {
    // A keyed payload type: ordered and compared by `key` only, so remove()'s out-param
    // must hand back the *stored* element (with its payload), not the probe.
    struct kv {
        int key{};
        int payload{};
    };

    bool operator<(const kv& a, const kv& b) { return a.key < b.key; }
    bool operator==(const kv& a, const kv& b) { return a.key == b.key; }

    // A keyed, resource-owning element (ordered by key, payload is a shared_ptr) used to
    // verify that vacated slots release their resources promptly.
    struct res {
        int key{};
        std::shared_ptr<int> payload{};
    };

    bool operator<(const res& a, const res& b) { return a.key < b.key; }
    bool operator==(const res& a, const res& b) { return a.key == b.key; }

    template <class Container>
    bool is_sorted_unique(const Container& c) {
        for (std::size_t i = 1; i < c.size(); ++i) {
            if (!(c.get(i - 1) < c.get(i))) { // must be strictly increasing
                return false;
            }
        }
        return true;
    }

    template <class Container>
    std::vector<int> to_vector(const Container& c) {
        std::vector<int> v;
        for (const int x : c) { // exercises begin()/end()
            v.push_back(x);
        }
        return v;
    }
} // namespace

TEST_SUITE("sorted_array: dynamic") {
    TEST_CASE("empty on construction") {
        sorted_array<int> a;
        CHECK(a.empty());
        CHECK(a.size() == 0);
        CHECK_FALSE(a.exists(0));
        CHECK(a.index_of(0) == a.size());
    }

    TEST_CASE("insert keeps sorted order and rejects duplicates") {
        sorted_array<int> a;
        for (const int x : {5, 1, 3, 9, 2, 7}) { // inserted out of order
            CHECK(a.insert(x));
        }
        CHECK(a.size() == 6);
        CHECK(to_vector(a) == std::vector<int>{1, 2, 3, 5, 7, 9});
        CHECK(is_sorted_unique(a));

        CHECK_FALSE(a.insert(5)); // duplicates rejected, size unchanged
        CHECK_FALSE(a.insert(1));
        CHECK(a.size() == 6);
    }

    TEST_CASE("exists and index_of") {
        sorted_array<int> a;
        for (const int x : {10, 20, 30, 40}) {
            a.insert(x);
        }
        CHECK(a.exists(30));
        CHECK_FALSE(a.exists(25));
        CHECK(a.index_of(10) == 0);
        CHECK(a.index_of(40) == 3);
        CHECK(a.index_of(25) == a.size()); // not-found sentinel
    }

    TEST_CASE("remove by value, with and without old_val") {
        sorted_array<int> a;
        for (const int x : {1, 2, 3, 4, 5}) {
            a.insert(x);
        }
        CHECK(a.remove(3));
        CHECK_FALSE(a.exists(3));
        CHECK(to_vector(a) == std::vector<int>{1, 2, 4, 5});

        int taken = -1;
        CHECK(a.remove(4, &taken));
        CHECK(taken == 4);

        CHECK_FALSE(a.remove(42)); // absent
        CHECK(to_vector(a) == std::vector<int>{1, 2, 5});
        CHECK(is_sorted_unique(a));
    }

    TEST_CASE("remove_by_index and pop_back") {
        sorted_array<int> a;
        for (const int x : {2, 4, 6, 8}) {
            a.insert(x);
        }
        a.remove_by_index(0); // drops 2
        CHECK(to_vector(a) == std::vector<int>{4, 6, 8});
        CHECK(a.pop_back() == 8); // largest first
        CHECK(a.pop_back() == 6);
        CHECK(to_vector(a) == std::vector<int>{4});
    }

    TEST_CASE("clear and reserve-only constructor") {
        sorted_array<int> a(16); // reserve only, no elements
        CHECK(a.empty());
        a.insert(1);
        a.insert(2);
        a.clear();
        CHECK(a.empty());
        CHECK(a.size() == 0);
    }

    TEST_CASE("range constructor fills [min, max)") {
        sorted_array<int> a(2, 6);
        CHECK(to_vector(a) == std::vector<int>{2, 3, 4, 5});
    }

    TEST_CASE("range constructor: empty and inverted ranges terminate and are empty") {
        const sorted_array<int> empty_range(5, 5);
        CHECK(empty_range.empty());
        const sorted_array<int> inverted(7, 3); // min > max -> empty, must NOT loop
        CHECK(inverted.empty());
    }

    TEST_CASE("get is bounds-checked; index/empty guards throw") {
        sorted_array<int> a;
        a.insert(11);
        a.insert(22);
        CHECK(a.get(0) == 11);
        CHECK(a.get(1) == 22);
        CHECK(a.is_valid_index(1));
        CHECK_FALSE(a.is_valid_index(2));

        CHECK_THROWS((void)a.get(2));       // out of range (cast: get() is [[nodiscard]])
        CHECK_THROWS(a.remove_by_index(2)); // out of range

        sorted_array<int> empty;
        CHECK_THROWS(empty.pop_back()); // empty
    }

    TEST_CASE("keyed payload: remove returns the stored element; get mutates payload") {
        sorted_array<kv> a;
        a.insert(kv{2, 200});
        a.insert(kv{1, 100});
        a.insert(kv{3, 300});

        kv removed{};
        CHECK(a.remove(kv{1, 0}, &removed)); // probe payload ignored (matched by key)
        CHECK(removed.payload == 100);       // out-param carries the stored payload

        a.get(a.index_of(kv{2, 0})).payload = 999; // mutate payload, not the key
        CHECK(a.get(a.index_of(kv{2, 0})).payload == 999);
        CHECK(is_sorted_unique(a));
    }

    TEST_CASE("matches std::set under random insert/remove") {
        sorted_array<int> a;
        std::set<int> ref;
        std::uint32_t seed = 0xC0FFEEu;
        auto next = [&seed]() { seed = seed * 1664525u + 1013904223u; return seed; };

        for (int op = 0; op < 4000; ++op) {
            const int v = static_cast<int>(next() % 200u);
            if (next() & 1u) {
                CHECK(a.insert(v) == ref.insert(v).second); // same accept/reject
            } else {
                CHECK(a.remove(v) == (ref.erase(v) != 0)); // same found/not-found
            }
            REQUIRE(a.size() == ref.size());
            CHECK(to_vector(a) == std::vector<int>(ref.begin(), ref.end())); // identical & sorted
        }
    }
}

TEST_SUITE("sorted_array: fixed") {
    TEST_CASE("insert respects capacity; full() disambiguates the false return") {
        fixed_sorted_array<int, 4> a;
        CHECK(a.capacity() == 4);
        CHECK(a.insert(3));
        CHECK(a.insert(1));
        CHECK(a.insert(2));
        CHECK(a.insert(4));
        CHECK(a.full());
        CHECK(to_vector(a) == std::vector<int>{1, 2, 3, 4});

        CHECK_FALSE(a.insert(5)); // full -> reject a NEW element ...
        CHECK(a.full());          // ... and full() explains the false
        CHECK(a.size() == 4);

        a.remove(4);               // make room
        CHECK_FALSE(a.full());
        CHECK_FALSE(a.insert(1));   // duplicate although there is room -> not a "full" false
        CHECK_FALSE(a.full());
    }

    TEST_CASE("exists, index_of, get with bounds guard") {
        fixed_sorted_array<int, 8> a;
        for (const int x : {40, 10, 30, 20}) {
            CHECK(a.insert(x));
        }
        CHECK(to_vector(a) == std::vector<int>{10, 20, 30, 40});
        CHECK(a.exists(30));
        CHECK_FALSE(a.exists(99));
        CHECK(a.index_of(10) == 0);
        CHECK(a.index_of(99) == a.size()); // not-found sentinel is size(), not N
        CHECK(a.get(2) == 30);
        CHECK_THROWS((void)a.get(4)); // out of range (size 4, capacity 8)
    }

    TEST_CASE("is_valid_index tracks live size") {
        fixed_sorted_array<int, 4> a;
        CHECK_FALSE(a.is_valid_index(0));
        a.insert(2);
        a.insert(4);
        CHECK(a.is_valid_index(0));
        CHECK(a.is_valid_index(1));
        CHECK_FALSE(a.is_valid_index(2));
    }

    TEST_CASE("remove, remove_by_index, pop_back, clear") {
        fixed_sorted_array<int, 8> a;
        for (const int x : {1, 2, 3, 4, 5}) {
            a.insert(x);
        }
        int taken = -1;
        CHECK(a.remove(3, &taken));
        CHECK(taken == 3);
        CHECK(to_vector(a) == std::vector<int>{1, 2, 4, 5});
        CHECK_FALSE(a.remove(3)); // already gone

        a.remove_by_index(0); // drops 1
        CHECK(to_vector(a) == std::vector<int>{2, 4, 5});
        CHECK(a.pop_back() == 5);
        CHECK(to_vector(a) == std::vector<int>{2, 4});

        CHECK_THROWS(a.remove_by_index(5)); // out of range
        a.clear();
        CHECK(a.empty());
        CHECK_THROWS(a.pop_back()); // empty
    }

    TEST_CASE("refills to capacity after removals (no stale slots)") {
        fixed_sorted_array<int, 3> a;
        CHECK(a.insert(1));
        CHECK(a.insert(2));
        CHECK(a.insert(3));
        CHECK(a.full());
        a.remove(2);
        CHECK(a.insert(2)); // slot reclaimed
        CHECK(a.full());
        CHECK(to_vector(a) == std::vector<int>{1, 2, 3});
    }

    TEST_CASE("releases resources from vacated slots (no retention after remove/pop/clear)") {
        auto sp = std::make_shared<int>(42);
        CHECK(sp.use_count() == 1);
        {
            fixed_sorted_array<res, 4> a;
            a.insert(res{1, sp}); // each insert copies the shared_ptr
            a.insert(res{2, sp});
            a.insert(res{3, sp});
            CHECK(sp.use_count() == 4); // owner + three copies

            a.remove(res{2, nullptr}); // removed copy released, trailing slot reset
            CHECK(sp.use_count() == 3);

            a.pop_back(); // popped copy released (the retention bug this guards)
            CHECK(sp.use_count() == 2);

            a.clear(); // remaining copies released
            CHECK(sp.use_count() == 1);
        }
        CHECK(sp.use_count() == 1); // container destruction adds nothing extra
    }

    TEST_CASE("matches std::set under random insert/remove within capacity") {
        constexpr std::size_t cap = 32;
        fixed_sorted_array<int, cap> a;
        std::set<int> ref;
        std::uint32_t seed = 0x1234u;
        auto next = [&seed]() { seed = seed * 1664525u + 1013904223u; return seed; };

        for (int op = 0; op < 5000; ++op) {
            const int v = static_cast<int>(next() % 50u);
            if (next() & 1u) {
                const bool would_be_new = ref.find(v) == ref.end();
                const bool inserted = a.insert(v);
                if (would_be_new && ref.size() < cap) {
                    CHECK(inserted);
                    ref.insert(v);
                } else {
                    CHECK_FALSE(inserted); // duplicate or full
                }
            } else {
                CHECK(a.remove(v) == (ref.erase(v) != 0));
            }
            REQUIRE(a.size() == ref.size());
            CHECK(to_vector(a) == std::vector<int>(ref.begin(), ref.end()));
        }
    }
}
