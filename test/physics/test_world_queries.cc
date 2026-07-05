//
// Created by igor on 22/06/2026.
//
// Brute-force correctness suite for the world's spatial queries
// (<neutrino/physics/collide/world.hh>): the swept `cast`, the `raycast`, `line_of_sight`,
// and the internal resident-vs-resident `overlap`. Each tree-accelerated query is
// cross-checked against a linear scan over the same scene using the Layer-1 primitives
// directly (swept_intersection / intersect_param / intersects) -- the tree must agree with
// brute force exactly. Plus targeted scenarios: anti-tunneling, initial overlap, filtering,
// self-exclusion.
//
// `cast` and `overlap` are internal (low-level / not game-facing); they are reached through
// the `world_test_access` friend tap so the suite can verify them without widening the API.
// `raycast` / `line_of_sight` are public and called directly.
//
#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <set>
#include <variant>
#include <vector>

#include <neutrino/physics/collide/world.hh>

using namespace neutrino::physics;


namespace neutrino::physics {
    // Friend tap (declared in world.hh) -- forwards to the non-public query helpers. Defined in
    // exactly ONE test TU to avoid an ODR clash when the suites link together.
    struct world_test_access {
        static std::optional<contact> cast(const world& w, uint32_t idx, collider_id::type t, vec d) {
            return w.cast(idx, t, units::displacement{d});
        }
        template <class Fn>
        static void overlap(const world& w, uint32_t idx, Fn&& fn) {
            w.overlap(idx, std::forward<Fn>(fn));
        }
        // The fat box stored in the broadphase for a resident's proxy.
        static aabb proxy_box(const world& w, collider_id cid) {
            return w.m_space_partition[w.m_bodies_storage[cid.value].proxy].box;
        }
    };
}
namespace {
    // ---- deterministic RNG -------------------------------------------------
    struct lcg {
        std::uint32_t s;
        std::uint32_t operator()() { s = s * 1664525u + 1013904223u; return s; }
    };
    float frand(lcg& r, float lo, float hi) {
        return lo + (hi - lo) * (static_cast<float>(r() & 0xFFFFFFu) / static_cast<float>(0xFFFFFFu));
    }

    bool t_should_collide(filter_props a, filter_props b) {
        return (a.category & b.mask) && (b.category & a.mask);
    }

    // ---- random shapes -----------------------------------------------------
    shape_t rand_target(lcg& r) {
        switch (r() % 3u) {
            case 0: {
                const float x = frand(r, -10, 10), y = frand(r, -10, 10);
                return aabb{{x, y}, {x + frand(r, 0.5f, 3.0f), y + frand(r, 0.5f, 3.0f)}};
            }
            case 1:
                return circle{{frand(r, -10, 10), frand(r, -10, 10)}, frand(r, 0.3f, 2.0f)};
            default:
                return segment{{frand(r, -10, 10), frand(r, -10, 10)}, {frand(r, -10, 10), frand(r, -10, 10)}};
        }
    }
    moving_shape_t rand_mover(lcg& r) {
        if (r() & 1u) {
            const float x = frand(r, -10, 10), y = frand(r, -10, 10);
            return aabb{{x, y}, {x + frand(r, 0.4f, 2.0f), y + frand(r, 0.4f, 2.0f)}};
        }
        return circle{{frand(r, -10, 10), frand(r, -10, 10)}, frand(r, 0.3f, 1.5f)};
    }

    // ---- a recorded resident (the test's ground-truth copy) ----------------
    struct rec {
        shape_t      shape;
        filter_props filter;
        collider_id  id; // the FULL handle returned by world.add (value + generation + type)
    };

    // ---- brute-force references (linear scans, mirroring the world's logic) -
    std::optional<float> brute_cast(const moving_shape_t& mover, vec delta,
                                    const std::vector<rec>& recs, filter_props f) {
        std::optional<float> best;
        for (const auto& rc : recs) {
            if (!t_should_collide(f, rc.filter)) {
                continue;
            }
            std::visit([&](const auto& mv) {
                std::visit([&](const auto& tg) {
                    if (const auto h = swept_intersection(mv, delta, tg, vec{0, 0}, 1.0f);
                        h && (!best || h->entry_time < *best)) {
                        best = h->entry_time;
                    }
                }, rc.shape);
            }, mover);
        }
        return best;
    }

    std::optional<float> brute_raycast(const segment& s, const std::vector<rec>& recs, filter_props f) {
        std::optional<float> best;
        for (const auto& rc : recs) {
            if (!t_should_collide(f, rc.filter)) {
                continue;
            }
            std::visit([&]<typename T>(const T& shape) {
                using S = std::decay_t<T>;
                const std::optional<line_hit> h = [&] {
                    if constexpr (std::is_same_v<S, segment>) {
                        return intersect_param(s, shape);
                    } else {
                        return intersect_param(shape, s);
                    }
                }();
                if (h && h->segment_overlaps()) {
                    const float toi = std::max(0.0f, h->entry_param);
                    if (!best || toi < *best) {
                        best = toi;
                    }
                }
            }, rc.shape);
        }
        return best;
    }

    static_body mk_static(const shape_t& s, filter_props f = {}) {
        static_body b;
        b.shape = s;
        b.filter = f;
        return b;
    }

    const rec* find_rec(const std::vector<rec>& recs, uint32_t idx) {
        const auto it = std::find_if(recs.begin(), recs.end(),
                                     [idx](const rec& r) { return r.id.value == idx; });
        return it == recs.end() ? nullptr : &*it;
    }

    bool approx(float a, float b) { return std::fabs(a - b) < 1e-3f; }

    // Validate the FULL returned handle against the recorded one: a right-slot/wrong-generation
    // (or wrong type) contact must NOT pass.
    void check_handle(const collider_id& got, const rec& hit) {
        CHECK(got.value == hit.id.value);
        CHECK(got.generation == hit.id.generation);
        CHECK(got.type_id == hit.id.type_id);
    }

    // Tie-safe validation of a cast contact: `who` must be a real candidate (full handle match)
    // that passed the query filter, and whose OWN swept hit reproduces the reported toi AND
    // normal. (Comparing who directly against brute force would be fragile on equal-toi ties,
    // where tree order and scan order may pick different bodies.)
    void check_cast_contact(const moving_shape_t& mover, vec delta, filter_props qfilter,
                            const std::vector<rec>& recs, const contact& got) {
        const rec* hit = find_rec(recs, got.who.value);
        REQUIRE(hit != nullptr);
        check_handle(got.who, *hit);
        CHECK(t_should_collide(qfilter, hit->filter)); // the hit must have passed the filter
        std::visit([&](const auto& mv) {
            std::visit([&](const auto& tg) {
                const auto h = swept_intersection(mv, delta, tg, vec{0, 0}, 1.0f);
                REQUIRE(h.has_value());
                CHECK(approx(h->entry_time, got.toi));
                CHECK(approx(h->entry_normal.x(), got.normal.x()));
                CHECK(approx(h->entry_normal.y(), got.normal.y()));
            }, hit->shape);
        }, mover);
    }

    void check_ray_contact(const segment& ray, filter_props qfilter,
                           const std::vector<rec>& recs, const contact& got) {
        const rec* hit = find_rec(recs, got.who.value);
        REQUIRE(hit != nullptr);
        check_handle(got.who, *hit);
        CHECK(t_should_collide(qfilter, hit->filter));
        std::visit([&]<typename T>(const T& shape) {
            using S = std::decay_t<T>;
            const std::optional<line_hit> h = [&] {
                if constexpr (std::is_same_v<S, segment>) {
                    return intersect_param(ray, shape);
                } else {
                    return intersect_param(shape, ray);
                }
            }();
            REQUIRE(h.has_value());
            REQUIRE(h->segment_overlaps());
            CHECK(approx(std::max(0.0f, h->entry_param), got.toi));
            CHECK(approx(h->entry_normal.x(), got.normal.x()));
            CHECK(approx(h->entry_normal.y(), got.normal.y()));
        }, hit->shape);
    }
} // namespace

TEST_SUITE("world queries: cast vs brute force") {
    TEST_CASE("random scenes: earliest TOI agrees with a linear swept scan") {
        lcg r{0xCAFEu};
        for (int scene = 0; scene < 300; ++scene) {
            world w;
            std::vector<rec> recs;
            const int n = 5 + static_cast<int>(r() % 20u);
            for (int i = 0; i < n; ++i) {
                const shape_t s = rand_target(r);
                filter_props f;
                f.category = static_cast<uint16_t>(1u << (r() % 4u));
                const auto cid = w.add(static_cast<entity_id_t>(i), mk_static(s, f));
                recs.push_back({s, f, cid});
            }

            // a bullet mover (not in the tree, no self-exclusion) cast across the scene
            bullet b;
            b.shape = rand_mover(r);
            b.filter.mask = static_cast<uint16_t>(0xFu); // collides with categories 1,2,4,8
            const auto bid = w.add(9999, b);
            const vec delta{frand(r, -8, 8), frand(r, -8, 8)};

            const auto got = world_test_access::cast(w, bid.value, collider_id::BULLET, delta);
            const auto exp = brute_cast(b.shape, delta, recs, b.filter);

            CHECK(got.has_value() == exp.has_value());
            if (got && exp) {
                CHECK(got->toi == doctest::Approx(*exp).epsilon(1e-4)); // earliest toi correct
                check_cast_contact(b.shape, delta, b.filter, recs, *got);          // who + normal consistent
            }
        }
    }

    TEST_CASE("anti-tunneling: fast mover through a thin wall is caught by the sweep") {
        world w;
        w.add(1, mk_static(aabb{{5.0f, -5.0f}, {5.1f, 5.0f}})); // 0.1-wide wall at x=5
        bullet b;
        b.shape = circle{{0.0f, 0.0f}, 0.1f};
        const auto bid = w.add(2, b);

        const auto hit = world_test_access::cast(w, bid.value, collider_id::BULLET, vec{20.0f, 0.0f});
        REQUIRE(hit.has_value());          // a point-sample at start/end would miss the thin wall
        CHECK(hit->toi > 0.0f);
        CHECK(hit->toi < 1.0f);
    }

    TEST_CASE("initial overlap reports toi == 0") {
        world w;
        w.add(1, mk_static(aabb{{-1.0f, -1.0f}, {1.0f, 1.0f}}));
        bullet b;
        b.shape = circle{{0.0f, 0.0f}, 0.5f}; // starts inside the box
        const auto bid = w.add(2, b);

        const auto hit = world_test_access::cast(w, bid.value, collider_id::BULLET, vec{5.0f, 0.0f});
        REQUIRE(hit.has_value());
        CHECK(hit->toi == doctest::Approx(0.0f));
    }

    TEST_CASE("filter: a mover only hits categories its mask allows") {
        world w;
        filter_props wall_f;
        wall_f.category = 0x0001;
        w.add(1, mk_static(aabb{{5.0f, -5.0f}, {6.0f, 5.0f}}, wall_f));

        bullet b;
        b.shape = circle{{0.0f, 0.0f}, 0.2f};
        b.filter.mask = 0x0002; // does NOT include the wall's category 0x0001
        const auto bid = w.add(2, b);
        CHECK_FALSE(world_test_access::cast(w, bid.value, collider_id::BULLET, vec{20.0f, 0.0f}).has_value());

        b.filter.mask = 0x0003; // now includes 0x0001
        const auto bid2 = w.add(3, b);
        CHECK(world_test_access::cast(w, bid2.value, collider_id::BULLET, vec{20.0f, 0.0f}).has_value());
    }

    TEST_CASE("self-exclusion: a BODY mover never reports itself") {
        world w;
        kinematic_body k;
        k.shape = aabb{{0.0f, 0.0f}, {1.0f, 1.0f}};
        const auto kid = w.add(1, k); // the only resident -> the tree contains just itself

        // Without self-exclusion this would hit itself at toi 0; correct result is nullopt.
        CHECK_FALSE(world_test_access::cast(w, kid.value, collider_id::BODY, vec{0.5f, 0.0f}).has_value());
    }

    TEST_CASE("filter symmetry: the TARGET's mask can reject the mover") {
        // mover mask allows the target, but the target's mask excludes the mover -> no hit.
        // Guards the full (a.cat & b.mask) && (b.cat & a.mask) contract, not just one side.
        world w;
        filter_props wall_f;
        wall_f.category = 0x0001;
        wall_f.mask = 0x0002;             // wall only collides with category 0x0002 ...
        w.add(1, mk_static(aabb{{5.0f, -5.0f}, {6.0f, 5.0f}}, wall_f));

        bullet b;
        b.shape = circle{{0.0f, 0.0f}, 0.2f};
        b.filter.category = 0x0004;       // ... but the mover is 0x0004 (rejected by the wall)
        b.filter.mask = 0xFFFF;           // mover would accept everything
        const auto bid = w.add(2, b);
        CHECK_FALSE(world_test_access::cast(w, bid.value, collider_id::BULLET, vec{20.0f, 0.0f}).has_value());

        // make the mover's category match the wall's mask -> now both directions pass -> hit.
        b.filter.category = 0x0002;
        const auto bid2 = w.add(3, b);
        CHECK(world_test_access::cast(w, bid2.value, collider_id::BULLET, vec{20.0f, 0.0f}).has_value());
    }

    TEST_CASE("public cast(moving_shape_t, delta, filter): aim an arbitrary shape") {
        world w;
        w.add(1, mk_static(aabb{{4.0f, -5.0f}, {5.0f, 5.0f}}));  // wall, left face x=4
        w.add(2, mk_static(aabb{{8.0f, -1.0f}, {9.0f, 1.0f}}));

        // box mover from origin to the right: nearest hit is the wall; box right edge x=1
        // reaches x=4 after 3 of 20 -> toi == 3/20.
        const auto hit = w.cast(moving_shape_t{aabb{{0.0f, 0.0f}, {1.0f, 1.0f}}}, vec{20.0f, 0.0f});
        REQUIRE(hit.has_value());
        CHECK(hit->toi == doctest::Approx(3.0f / 20.0f).epsilon(1e-4));
        CHECK(hit->normal.x() == doctest::Approx(-1.0f)); // left face of the wall

        CHECK_FALSE(w.cast(moving_shape_t{circle{{0.0f, 100.0f}, 0.5f}}, vec{0.0f, 5.0f}).has_value());

        filter_props none; none.mask = 0x0000; // matches nothing
        CHECK_FALSE(w.cast(moving_shape_t{aabb{{0.0f, 0.0f}, {1.0f, 1.0f}}}, vec{20.0f, 0.0f}, none).has_value());
    }
}

TEST_SUITE("world queries: raycast vs brute force") {
    TEST_CASE("random scenes: nearest crossing agrees with a linear scan") {
        lcg r{0x1357u};
        for (int scene = 0; scene < 300; ++scene) {
            world w;
            std::vector<rec> recs;
            const int n = 5 + static_cast<int>(r() % 20u);
            for (int i = 0; i < n; ++i) {
                const shape_t s = rand_target(r);
                filter_props f;
                f.category = static_cast<uint16_t>(1u << (r() % 4u));
                const auto cid = w.add(static_cast<entity_id_t>(i), mk_static(s, f));
                recs.push_back({s, f, cid});
            }
            const segment ray{{frand(r, -12, 12), frand(r, -12, 12)},
                              {frand(r, -12, 12), frand(r, -12, 12)}};
            filter_props rf;
            rf.mask = 0xFu;

            const auto got = w.raycast(ray, rf);
            const auto exp = brute_raycast(ray, recs, rf);
            CHECK(got.has_value() == exp.has_value());
            if (got && exp) {
                CHECK(got->toi == doctest::Approx(*exp).epsilon(1e-4)); // nearest toi correct
                check_ray_contact(ray, rf, recs, *got);                     // who + normal consistent
            }
        }
    }

    TEST_CASE("nearest of three shapes; segment/aabb/circle all hittable") {
        world w;
        w.add(1, mk_static(segment{{5.0f, -5.0f}, {5.0f, 5.0f}})); // wall  @ x=5
        w.add(2, mk_static(aabb{{8.0f, -1.0f}, {9.0f, 1.0f}}));    // block @ x=8
        w.add(3, mk_static(circle{{12.0f, 0.0f}, 1.0f}));          // round @ x=11..13

        const auto hit = w.raycast(segment{{0.0f, 0.0f}, {20.0f, 0.0f}});
        REQUIRE(hit.has_value());
        CHECK(hit->toi == doctest::Approx(5.0f / 20.0f)); // the segment wall is nearest
    }

    TEST_CASE("a ray pointing away from all geometry misses (no behind-origin hit)") {
        world w;
        w.add(1, mk_static(aabb{{5.0f, -1.0f}, {6.0f, 1.0f}}));
        CHECK_FALSE(w.raycast(segment{{0.0f, 0.0f}, {-20.0f, 0.0f}}).has_value());
    }

    TEST_CASE("filter excludes non-matching residents") {
        world w;
        filter_props wall_f;
        wall_f.category = 0x0001;
        w.add(1, mk_static(aabb{{5.0f, -5.0f}, {6.0f, 5.0f}}, wall_f));

        filter_props probe;
        probe.mask = 0x0002; // does not match the wall
        CHECK_FALSE(w.raycast(segment{{0.0f, 0.0f}, {20.0f, 0.0f}}, probe).has_value());
        probe.mask = 0x0003;
        CHECK(w.raycast(segment{{0.0f, 0.0f}, {20.0f, 0.0f}}, probe).has_value());
    }
}

TEST_SUITE("world queries: line_of_sight") {
    TEST_CASE("blocked by a wall, clear without one") {
        world w;
        w.add(1, mk_static(aabb{{5.0f, -5.0f}, {6.0f, 5.0f}})); // wall between the two points
        CHECK_FALSE(w.line_of_sight(vec{0.0f, 0.0f}, vec{10.0f, 0.0f}));
        CHECK(w.line_of_sight(vec{0.0f, 0.0f}, vec{4.0f, 0.0f}));   // target before the wall
        CHECK(w.line_of_sight(vec{0.0f, 8.0f}, vec{10.0f, 8.0f}));  // path above the wall
    }
}

TEST_SUITE("world queries: overlap vs brute force") {
    TEST_CASE("random scenes: overlapping-resident set agrees with a linear intersects scan") {
        lcg r{0x2468u};
        for (int scene = 0; scene < 300; ++scene) {
            world w;
            std::vector<rec> recs;
            const int n = 4 + static_cast<int>(r() % 14u);
            for (int i = 0; i < n; ++i) {
                // keep them clustered so overlaps actually happen. Build aabbs from an origin +
                // POSITIVE w/h so min <= max always holds (independent min/max could invert).
                shape_t s;
                if (r() & 1u) {
                    const float x = frand(r, -3, 3), y = frand(r, -3, 3);
                    s = aabb{{x, y}, {x + frand(r, 0.5f, 2.0f), y + frand(r, 0.5f, 2.0f)}};
                } else {
                    s = circle{{frand(r, -3, 3), frand(r, -3, 3)}, frand(r, 0.5f, 1.5f)};
                }
                filter_props f;
                f.category = static_cast<uint16_t>(1u << (r() % 3u));
                const auto cid = w.add(static_cast<entity_id_t>(i), mk_static(s, f));
                recs.push_back({s, f, cid});
            }

            for (const auto& self : recs) {
                std::set<uint32_t> got;
                world_test_access::overlap(w, self.id.value, [&](collider_id c) { got.insert(c.value); });

                std::set<uint32_t> exp;
                for (const auto& other : recs) {
                    if (other.id.value == self.id.value || !t_should_collide(other.filter, self.filter)) {
                        continue;
                    }
                    const bool hit = std::visit([&](const auto& a) {
                        return std::visit([&](const auto& b) { return intersects(a, b); }, other.shape);
                    }, self.shape);
                    if (hit) {
                        exp.insert(other.id.value);
                    }
                }
                CHECK(got == exp);
            }
        }
    }
}

TEST_SUITE("world: fat-box proxy refit") {
    // outer fully contains inner (closed bounds).
    auto contains_box = [](const aabb& outer, const aabb& inner) {
        return outer.min.x() <= inner.min.x() && outer.min.y() <= inner.min.y()
            && inner.max.x() <= outer.max.x() && inner.max.y() <= outer.max.y();
    };
    auto box_eq = [](const aabb& a, const aabb& b) {
        return a.min.x() == b.min.x() && a.min.y() == b.min.y()
            && a.max.x() == b.max.x() && a.max.y() == b.max.y();
    };
    auto tight_of = [](const shape_t& s) {
        return std::visit([](const auto& shp) { return enclose(shp); }, s);
    };
    const aabb BIG{{-1000, -1000}, {1000, 1000}};

    TEST_CASE("a small move within the fat margin does not re-fit the proxy") {
        world w; // default fatten_margin 0.1
        kinematic_body kb; kb.shape = aabb{{0, 0}, {1, 1}}; kb.velocity = vec{5, 0};
        const collider_id k = w.add(1, kb);
        const aabb before = world_test_access::proxy_box(w, k);

        (void) w.run(BIG, 0.01f);                       // moves 0.05 (< 0.1 margin)
        const aabb after = world_test_access::proxy_box(w, k);
        CHECK(box_eq(before, after));                    // stored box unchanged -> no re-fit
        CHECK(contains_box(after, tight_of(w.get_shape(k)))); // still bounds the body
    }

    TEST_CASE("accumulated movement escapes the fat box and re-fits (still always bounding)") {
        world w;
        kinematic_body kb; kb.shape = aabb{{0, 0}, {1, 1}}; kb.velocity = vec{5, 0};
        const collider_id k = w.add(1, kb);
        const aabb before = world_test_access::proxy_box(w, k);

        bool refit = false;
        for (int i = 0; i < 10; ++i) {                  // 10 * 0.05 = 0.5 >> 0.1 margin
            (void) w.run(BIG, 0.01f);
            const aabb box = world_test_access::proxy_box(w, k);
            if (!box_eq(before, box)) refit = true;
            CHECK(contains_box(box, tight_of(w.get_shape(k)))); // invariant: proxy bounds shape
        }
        CHECK(refit);                                   // escaped at least once -> re-fit happened
    }
}
