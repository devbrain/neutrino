#include <doctest/doctest.h>

#include <neutrino/world/content_key.hh>

#include "video/sprite/resource_cache_core.hh"

#include <utility>

using namespace neutrino;

namespace {
    // A move-only stand-in for a real bundle that counts its own destruction, so tests
    // can observe exactly when the cache tears an entry down -- no GPU/sprite context.
    struct fake_bundle {
        int* destructions{nullptr};

        fake_bundle() = default;
        explicit fake_bundle(int* d) : destructions(d) {}
        fake_bundle(const fake_bundle&)            = delete;
        fake_bundle& operator=(const fake_bundle&) = delete;
        fake_bundle(fake_bundle&& o) noexcept : destructions(o.destructions) { o.destructions = nullptr; }
        fake_bundle& operator=(fake_bundle&& o) noexcept {
            if (this != &o) {
                if (destructions) ++*destructions;
                destructions = o.destructions;
                o.destructions = nullptr;
            }
            return *this;
        }
        ~fake_bundle() { if (destructions) ++*destructions; }
    };

    content_key key(std::uint64_t n) { return content_key{n, 0}; }
}

TEST_SUITE("neutrino::video resource_cache_core") {
    TEST_CASE("acquire builds on a miss, shares on a hit, and cools at refcount zero") {
        resource_cache_core <fake_bundle> core(4);
        int destroyed = 0;
        auto build = [&] { return fake_bundle{&destroyed}; };

        const auto a = core.acquire(key(1), build);
        const auto b = core.acquire(key(1), build); // hit: same entry, no second build
        CHECK(a.bundle == b.bundle);
        CHECK(core.resident_count() == 1);
        CHECK(core.cold_count() == 0);

        core.release(a);
        CHECK(core.cold_count() == 0);              // still one live ref (b)
        core.release(b);
        CHECK(core.resident_count() == 1);          // resident, but now cold
        CHECK(core.cold_count() == 1);
        CHECK(destroyed == 0);                       // cold != torn down
    }

    TEST_CASE("a lease copy retains the entry; the last drop releases it") {
        resource_cache_core <fake_bundle> core(4);
        int destroyed = 0;
        auto build = [&] { return fake_bundle{&destroyed}; };

        // The (core, handle) ctor adopts acquire's refcount; copying retains a second.
        resource_lease <fake_bundle> outer(core, core.acquire(key(1), build));
        CHECK(core.resident_count() == 1);
        {
            resource_lease <fake_bundle> inner = outer; // copy -> retain (refcount 2)
            CHECK(inner.valid());
        }                                                // inner drops -> release (refcount 1)
        CHECK(core.cold_count() == 0);                   // outer still holds it: not cold

        // A move transfers the lease without touching the refcount.
        resource_lease <fake_bundle> moved = std::move(outer);
        CHECK(core.cold_count() == 0);
        CHECK(moved.valid());
    }

    TEST_CASE("dropping the last lease cools the entry; overflowing the cold pool evicts LRU") {
        resource_cache_core <fake_bundle> core(1); // cold budget of one
        int destroyed = 0;
        auto build = [&] { return fake_bundle{&destroyed}; };

        {
            resource_lease <fake_bundle> a(core, core.acquire(key(1), build));
        } // a drops -> refcount 0 -> cold (budget 1, fits)
        CHECK(core.cold_count() == 1);
        CHECK(destroyed == 0);

        // A second distinct entry going cold overflows the budget-1 pool and evicts the LRU.
        {
            resource_lease <fake_bundle> b(core, core.acquire(key(2), build));
            CHECK(core.resident_count() == 2);
        }
        CHECK(core.cold_count() == 1);      // only the newest cold entry remains
        CHECK(core.resident_count() == 1);  // the LRU entry was torn down
        CHECK(destroyed == 1);              // ...and its bundle destructed exactly once
    }
}
