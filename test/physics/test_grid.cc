//
// Created by igor on 24/06/2026.
//
// Tests for the static collision grid scaffolding in <neutrino/physics/collide/grid.hh>
// (Phase G0): the grid_coord sentinel/validity, the row-major grid_storage<T>, the
// physical->grid cell mapping and its inverse cell_box (resolution w*h over a physical
// extent [min,max], so the cell size is derived). The mappings are private, so they are
// reached through the grid_test_access friend tap -- the same pattern as the world tests.
//
// Covered: storage (set/get/clear/free-list), the physical<->cell mappings, the physical
// set/get/clear/reset API, the region `query`, and the DDA `raycast` (incl. supercover at
// exact corners and along grid lines). The cell->shape materialization stays in the world.
//
#include <doctest/doctest.h>

#include <algorithm>
#include <set>
#include <vector>

#include <neutrino/physics/collide/grid.hh>

namespace neutrino::physics {
    // Friend tap (declared in grid.hh): reach the private mappings.
    struct grid_test_access {
        template <class T>
        static detail::grid_coord physical_to_grid(const grid<T>& g, const vec& v) {
            return g.physical_to_grid(v);
        }
        template <class T>
        static aabb cell_box(const grid<T>& g, const detail::grid_coord& c) {
            return g.cell_box(c);
        }
    };
}

using namespace neutrino::physics;

TEST_SUITE("grid: grid_coord") {
    TEST_CASE("default-constructed is the invalid sentinel") {
        const detail::grid_coord c;
        CHECK_FALSE(static_cast<bool>(c));
        CHECK(c.x == detail::grid_coord::INVALID);
        CHECK(c.y == detail::grid_coord::INVALID);
    }

    TEST_CASE("a real coord is valid and carries its components") {
        const detail::grid_coord c{2, 5};
        CHECK(static_cast<bool>(c));
        CHECK(c.x == 2u);
        CHECK(c.y == 5u);
    }

    TEST_CASE("a coord with either component invalid is invalid") {
        CHECK_FALSE(static_cast<bool>(detail::grid_coord{detail::grid_coord::INVALID, 0}));
        CHECK_FALSE(static_cast<bool>(detail::grid_coord{0, detail::grid_coord::INVALID}));
    }
}

TEST_SUITE("grid: grid_storage") {
    TEST_CASE("reports its dimensions") {
        const detail::grid_storage<int> s(3, 5);
        CHECK(s.get_width() == 3u);
        CHECK(s.get_height() == 5u);
    }

    TEST_CASE("a cell is empty until set; get returns nullptr") {
        detail::grid_storage<int> s(4, 3);
        CHECK(s.get(detail::grid_coord{0, 0}) == nullptr);   // nothing set yet
        s.set(detail::grid_coord{0, 0}, 11);
        const int* p = s.get(detail::grid_coord{0, 0});
        REQUIRE(p != nullptr);
        CHECK(*p == 11);
        CHECK(s.get(detail::grid_coord{1, 0}) == nullptr);   // a neighbour stays empty
    }

    TEST_CASE("set then get round-trips per cell; overwrite updates in place") {
        detail::grid_storage<int> s(4, 3);
        s.set(detail::grid_coord{0, 0}, 11);
        s.set(detail::grid_coord{3, 2}, 99);   // opposite corner
        s.set(detail::grid_coord{1, 2}, 42);
        CHECK(*s.get(detail::grid_coord{0, 0}) == 11);
        CHECK(*s.get(detail::grid_coord{3, 2}) == 99);
        CHECK(*s.get(detail::grid_coord{1, 2}) == 42);

        s.set(detail::grid_coord{0, 0}, 7);    // overwrite an occupied cell
        CHECK(*s.get(detail::grid_coord{0, 0}) == 7);
    }

    TEST_CASE("cells are independent; (x,y) and (y,x) are distinct cells (row-major)") {
        detail::grid_storage<int> s(3, 3);
        for (uint32_t y = 0; y < 3; ++y) {
            for (uint32_t x = 0; x < 3; ++x) {
                s.set(detail::grid_coord{x, y}, static_cast<int>(y * 3 + x)); // = the flat index
            }
        }
        // (1,0) and (0,1) must be different cells -> different stored values.
        CHECK(*s.get(detail::grid_coord{1, 0}) == 1);
        CHECK(*s.get(detail::grid_coord{0, 1}) == 3);
        CHECK(*s.get(detail::grid_coord{2, 2}) == 8);
    }

    TEST_CASE("clear empties a cell; double-clear and clear-empty are safe no-ops") {
        detail::grid_storage<int> s(4, 3);
        s.set(detail::grid_coord{2, 1}, 55);
        REQUIRE(s.get(detail::grid_coord{2, 1}) != nullptr);
        s.clear(detail::grid_coord{2, 1});
        CHECK(s.get(detail::grid_coord{2, 1}) == nullptr);   // now empty
        // these must not corrupt the free list (the clear-empty / double-clear guard)
        CHECK_NOTHROW(s.clear(detail::grid_coord{2, 1}));    // already empty
        CHECK_NOTHROW(s.clear(detail::grid_coord{0, 0}));    // never set
    }

    TEST_CASE("a cleared slot is reused by a later set, with the new value") {
        detail::grid_storage<int> s(4, 3);
        s.set(detail::grid_coord{0, 0}, 1);
        s.set(detail::grid_coord{1, 0}, 2);
        s.clear(detail::grid_coord{0, 0});                   // frees a slot
        s.set(detail::grid_coord{2, 0}, 3);                  // should reuse the freed slot
        // all live cells read back correctly (no aliasing from slot reuse)
        CHECK(s.get(detail::grid_coord{0, 0}) == nullptr);
        CHECK(*s.get(detail::grid_coord{1, 0}) == 2);
        CHECK(*s.get(detail::grid_coord{2, 0}) == 3);
    }

    TEST_CASE("get out of range returns nullptr (no OOB)") {
        const detail::grid_storage<int> s(4, 3);
        CHECK(s.get(detail::grid_coord{4, 0}) == nullptr);   // x == width
        CHECK(s.get(detail::grid_coord{0, 3}) == nullptr);   // y == height
        CHECK(s.get(detail::grid_coord{999, 999}) == nullptr);
        CHECK(s.get(detail::grid_coord{}) == nullptr);       // invalid sentinel
    }
}

TEST_SUITE("grid: physical_to_grid mapping") {
    TEST_CASE("square cells: 4x4 over [0,8]^2 -> 2x2 cells") {
        grid<int> g(4, 4, vec{0, 0}, vec{8, 8});
        auto cell = [&](float x, float y) { return grid_test_access::physical_to_grid(g, vec{x, y}); };

        CHECK(cell(0.0f, 0.0f).x == 0u);   CHECK(cell(0.0f, 0.0f).y == 0u);   // min corner
        CHECK(cell(1.0f, 1.0f).x == 0u);   CHECK(cell(1.0f, 1.0f).y == 0u);   // inside cell 0
        CHECK(cell(2.0f, 2.0f).x == 1u);   CHECK(cell(2.0f, 2.0f).y == 1u);   // on a boundary -> next
        CHECK(cell(5.0f, 3.0f).x == 2u);   CHECK(cell(5.0f, 3.0f).y == 1u);
        CHECK(cell(7.9f, 7.9f).x == 3u);   CHECK(cell(7.9f, 7.9f).y == 3u);   // last cell
    }

    TEST_CASE("the exact max corner is clamped into the last cell, not out of range") {
        grid<int> g(4, 4, vec{0, 0}, vec{8, 8});
        const auto c = grid_test_access::physical_to_grid(g, vec{8.0f, 8.0f});
        REQUIRE(static_cast<bool>(c));
        CHECK(c.x == 3u);   // not 4 (which would be OOB)
        CHECK(c.y == 3u);
    }

    TEST_CASE("points outside the physical bounds map to the invalid sentinel") {
        grid<int> g(4, 4, vec{0, 0}, vec{8, 8});
        CHECK_FALSE(static_cast<bool>(grid_test_access::physical_to_grid(g, vec{-0.1f, 4.0f})));
        CHECK_FALSE(static_cast<bool>(grid_test_access::physical_to_grid(g, vec{4.0f, -0.1f})));
        CHECK_FALSE(static_cast<bool>(grid_test_access::physical_to_grid(g, vec{8.1f, 4.0f})));
        CHECK_FALSE(static_cast<bool>(grid_test_access::physical_to_grid(g, vec{4.0f, 8.1f})));
    }

    TEST_CASE("non-origin bounds: extent is measured from grid_min") {
        grid<int> g(2, 2, vec{-10, -10}, vec{-6, -6}); // 2x2 over a 4x4 region -> cell 2x2
        auto cell = [&](float x, float y) { return grid_test_access::physical_to_grid(g, vec{x, y}); };
        CHECK(cell(-10.0f, -10.0f).x == 0u); CHECK(cell(-10.0f, -10.0f).y == 0u);
        CHECK(cell(-7.0f, -9.0f).x == 1u);   CHECK(cell(-7.0f, -9.0f).y == 0u);
        CHECK(cell(-6.0f, -6.0f).x == 1u);   CHECK(cell(-6.0f, -6.0f).y == 1u); // max corner clamped
    }

    TEST_CASE("rectangular (non-square) cells: 2x4 over [0,8]^2 -> cells 4 wide, 2 tall") {
        grid<int> g(2, 4, vec{0, 0}, vec{8, 8}); // cell_dim = (8/2, 8/4) = (4, 2)
        auto cell = [&](float x, float y) { return grid_test_access::physical_to_grid(g, vec{x, y}); };
        CHECK(cell(1.0f, 1.0f).x == 0u);   CHECK(cell(1.0f, 1.0f).y == 0u);
        CHECK(cell(5.0f, 3.0f).x == 1u);   CHECK(cell(5.0f, 3.0f).y == 1u);   // x: 5/4=1, y: 3/2=1
        CHECK(cell(7.0f, 7.0f).x == 1u);   CHECK(cell(7.0f, 7.0f).y == 3u);   // x: 7/4=1, y: 7/2=3
    }
}

TEST_SUITE("grid: cell_box") {
    // exact-corner box equality (cell_box only adds/multiplies exact cell dims, so bit-exact
    // for representable inputs).
    auto box_eq = [](const aabb& b, float minx, float miny, float maxx, float maxy) {
        return b.min.x() == minx && b.min.y() == miny && b.max.x() == maxx && b.max.y() == maxy;
    };

    TEST_CASE("square cells: world AABB of a cell (4x4 over [0,8]^2 -> 2x2 cells)") {
        grid<int> g(4, 4, vec{0, 0}, vec{8, 8});
        CHECK(box_eq(grid_test_access::cell_box(g, {0, 0}), 0.0f, 0.0f, 2.0f, 2.0f)); // min corner cell
        CHECK(box_eq(grid_test_access::cell_box(g, {2, 1}), 4.0f, 2.0f, 6.0f, 4.0f));
        CHECK(box_eq(grid_test_access::cell_box(g, {3, 3}), 6.0f, 6.0f, 8.0f, 8.0f)); // last cell hits max
    }

    TEST_CASE("non-origin bounds: boxes are offset from grid_min") {
        grid<int> g(2, 2, vec{-10, -10}, vec{-6, -6}); // cell 2x2
        CHECK(box_eq(grid_test_access::cell_box(g, {0, 0}), -10.0f, -10.0f, -8.0f, -8.0f));
        CHECK(box_eq(grid_test_access::cell_box(g, {1, 1}), -8.0f, -8.0f, -6.0f, -6.0f));
    }

    TEST_CASE("rectangular cells: 2x4 over [0,8]^2 -> cells 4 wide, 2 tall") {
        grid<int> g(2, 4, vec{0, 0}, vec{8, 8}); // cell_dim = (4, 2)
        CHECK(box_eq(grid_test_access::cell_box(g, {0, 0}), 0.0f, 0.0f, 4.0f, 2.0f));
        CHECK(box_eq(grid_test_access::cell_box(g, {1, 3}), 4.0f, 6.0f, 8.0f, 8.0f)); // far cell hits max
    }

    TEST_CASE("adjacent cells tile contiguously (shared edge, no gap/overlap)") {
        grid<int> g(4, 4, vec{0, 0}, vec{8, 8});
        const aabb a = grid_test_access::cell_box(g, {0, 0});
        const aabb right = grid_test_access::cell_box(g, {1, 0});
        const aabb below = grid_test_access::cell_box(g, {0, 1});
        CHECK(a.max.x() == right.min.x()); // x-adjacent cells share the vertical edge
        CHECK(a.max.y() == below.min.y()); // y-adjacent cells share the horizontal edge
    }

    TEST_CASE("round-trip: a cell's corner (and center) map back to that cell") {
        grid<int> g(4, 4, vec{0, 0}, vec{8, 8});
        for (uint32_t y = 0; y < 4; ++y) {
            for (uint32_t x = 0; x < 4; ++x) {
                const detail::grid_coord c{x, y};
                const aabb b = grid_test_access::cell_box(g, c);
                const auto from_min = grid_test_access::physical_to_grid(g, b.min);
                CHECK(from_min.x == x);
                CHECK(from_min.y == y);
                const vec center{(b.min.x() + b.max.x()) * 0.5f, (b.min.y() + b.max.y()) * 0.5f};
                const auto from_center = grid_test_access::physical_to_grid(g, center);
                CHECK(from_center.x == x);
                CHECK(from_center.y == y);
            }
        }
    }
}

TEST_SUITE("grid: physical API (set/get/clear/reset)") {
    TEST_CASE("set then get by world position; empty / out-of-bounds read as nullptr") {
        grid<int> g(4, 4, vec{0, 0}, vec{8, 8}); // cell 2x2
        g.set(vec{1, 1}, 11);                     // cell (0,0)
        g.set(vec{5, 3}, 42);                     // cell (2,1)
        REQUIRE(g.get(vec{1, 1}) != nullptr);
        CHECK(*g.get(vec{1, 1}) == 11);
        CHECK(*g.get(vec{5, 3}) == 42);
        CHECK(*g.get(vec{0.5f, 0.5f}) == 11);    // same cell (0,0) -> same value
        CHECK(g.get(vec{7, 7}) == nullptr);      // empty cell
        CHECK(g.get(vec{-1, 0}) == nullptr);     // out of bounds -> tolerant nullptr
        CHECK(g.get(vec{100, 100}) == nullptr);
    }

    TEST_CASE("set overwrites the cell that contains the position") {
        grid<int> g(4, 4, vec{0, 0}, vec{8, 8});
        g.set(vec{1, 1}, 1);
        g.set(vec{1.5f, 0.5f}, 9); // same cell (0,0)
        CHECK(*g.get(vec{0, 0}) == 9);
    }

    TEST_CASE("clear by position empties the cell; reset empties everything") {
        grid<int> g(4, 4, vec{0, 0}, vec{8, 8});
        g.set(vec{1, 1}, 1);
        g.set(vec{5, 5}, 2);
        g.clear(vec{1, 1});
        CHECK(g.get(vec{1, 1}) == nullptr);
        CHECK(*g.get(vec{5, 5}) == 2);           // unaffected
        CHECK_NOTHROW(g.clear(vec{1, 1}));       // clear-empty no-op
        CHECK_NOTHROW(g.clear(vec{200, 0}));     // clear out-of-bounds no-op
        g.reset();
        CHECK(g.get(vec{5, 5}) == nullptr);      // all gone
    }

    TEST_CASE("works with a non-default-constructible payload") {
        struct body { int v; explicit body(int x) : v(x) {} };
        grid<body> g(2, 2, vec{0, 0}, vec{4, 4});
        g.set(vec{1, 1}, 7);                      // emplace body(7)
        REQUIRE(g.get(vec{1, 1}) != nullptr);
        CHECK(g.get(vec{1, 1})->v == 7);
    }
}

TEST_SUITE("grid: query (region enumeration)") {
    // helper: collect (value, cell_box) pairs a query yields
    struct hit { int v; aabb box; };

    TEST_CASE("enumerates occupied cells overlapping the region, with their cell_box") {
        grid<int> g(4, 4, vec{0, 0}, vec{8, 8}); // cell 2x2
        g.set(vec{1, 1}, 1);   // cell (0,0) box [0,0]..[2,2]
        g.set(vec{3, 1}, 2);   // cell (1,0) box [2,0]..[4,2]
        g.set(vec{5, 5}, 3);   // cell (2,2) box [4,4]..[6,6]  (outside the query box below)

        std::vector<hit> hits;
        g.query(aabb{{0, 0}, {3, 3}}, [&](const int& v, const aabb& b) { hits.push_back({v, b}); });

        // the region [0,3]^2 covers cells (0,0) and (1,0) (and (0,1)/(1,1) which are empty)
        REQUIRE(hits.size() == 2);
        std::sort(hits.begin(), hits.end(), [](const hit& a, const hit& b) { return a.v < b.v; });
        CHECK(hits[0].v == 1);
        CHECK(hits[0].box.min.x() == 0.0f); CHECK(hits[0].box.max.x() == 2.0f); // cell (0,0) box
        CHECK(hits[1].v == 2);
        CHECK(hits[1].box.min.x() == 2.0f); CHECK(hits[1].box.max.x() == 4.0f); // cell (1,0) box
    }

    TEST_CASE("skips empty cells") {
        grid<int> g(4, 4, vec{0, 0}, vec{8, 8});
        g.set(vec{1, 1}, 5); // only one occupied cell
        int count = 0;
        g.query(aabb{{0, 0}, {8, 8}}, [&](const int&, const aabb&) { ++count; });
        CHECK(count == 1); // whole grid queried, but only the one occupied cell reported
    }

    TEST_CASE("a region overlapping the grid edge is clipped (no OOB, no throw)") {
        grid<int> g(4, 4, vec{0, 0}, vec{8, 8});
        g.set(vec{7, 7}, 9);                      // last cell (3,3)
        int count = 0;
        // region pokes past the max corner -> clipped to the grid
        CHECK_NOTHROW(g.query(aabb{{6, 6}, {100, 100}}, [&](const int&, const aabb&) { ++count; }));
        CHECK(count == 1);
    }

    TEST_CASE("a region entirely outside the grid is a tolerant no-op") {
        grid<int> g(4, 4, vec{0, 0}, vec{8, 8});
        g.set(vec{1, 1}, 1);
        int count = 0;
        CHECK_NOTHROW(g.query(aabb{{100, 100}, {110, 110}}, [&](const int&, const aabb&) { ++count; }));
        CHECK(count == 0);
    }

    TEST_CASE("a bool callback can stop the scan early") {
        grid<int> g(4, 4, vec{0, 0}, vec{8, 8});
        for (uint32_t y = 0; y < 4; ++y)            // every cell occupied
            for (uint32_t x = 0; x < 4; ++x)
                g.set(vec{x * 2.0f + 1.0f, y * 2.0f + 1.0f}, static_cast<int>(y * 4 + x));
        int count = 0;
        g.query(aabb{{0, 0}, {8, 8}}, [&](const int&, const aabb&) {
            ++count;
            return false;                            // stop after the first cell
        });
        CHECK(count == 1);
    }
}

TEST_SUITE("grid: raycast (DDA)") {
    // dense point-sample of the ray -> the set of in-grid cells it passes through.
    static std::set<std::pair<uint32_t, uint32_t>>
    sampled_cells(uint32_t w, uint32_t h, vec mn, vec mx, vec a, vec b) {
        std::set<std::pair<uint32_t, uint32_t>> s;
        const vec cell{(mx.x() - mn.x()) / w, (mx.y() - mn.y()) / h};
        for (int i = 0; i <= 20000; ++i) {
            const float t = i / 20000.0f;
            const float px = a.x() + t * (b.x() - a.x());
            const float py = a.y() + t * (b.y() - a.y());
            if (px < mn.x() || px > mx.x() || py < mn.y() || py > mx.y()) continue;
            const auto cx = std::min(static_cast<uint32_t>((px - mn.x()) / cell.x()), w - 1u);
            const auto cy = std::min(static_cast<uint32_t>((py - mn.y()) / cell.y()), h - 1u);
            s.insert({cx, cy});
        }
        return s;
    }

    TEST_CASE("visits every cell the ray crosses (no tunnelling) in near->far order") {
        const vec mn{0, 0}, mx{8, 8};
        const uint32_t W = 4, H = 4; // cell 2x2
        // rays chosen to avoid exact cell corners (where a point-sampler is an unreliable oracle).
        const vec rays[][2] = {
            {{0.3f, 0.4f}, {7.7f, 7.2f}},   // diagonal-ish
            {{0.3f, 0.5f}, {7.7f, 2.6f}},   // shallow
            {{0.4f, 7.7f}, {7.2f, 0.3f}},   // anti-diagonal-ish
            {{1.0f, 0.3f}, {1.0f, 7.6f}},   // vertical (axis-aligned)
            {{0.3f, 3.3f}, {7.7f, 3.3f}},   // horizontal (axis-aligned)
            {{-3.0f, 3.3f}, {11.0f, 3.3f}}, // clipped (starts/ends outside)
        };
        for (const auto& r : rays) {
            grid<int> g(W, H, mn, mx);
            for (uint32_t y = 0; y < H; ++y)        // fill every cell so the DDA reports each visit
                for (uint32_t x = 0; x < W; ++x)
                    g.set(vec{x * 2.0f + 1.0f, y * 2.0f + 1.0f}, static_cast<int>(y * W + x));

            std::set<std::pair<uint32_t, uint32_t>> got;
            float last = -1.0f;
            g.raycast(r[0], r[1], [&](const int& v, const aabb&, float t) {
                CHECK(t >= last - 1e-4f);            // monotonic near -> far
                last = t;
                got.insert({static_cast<uint32_t>(v % W), static_cast<uint32_t>(v / W)});
            });
            const auto exp = sampled_cells(W, H, mn, mx, r[0], r[1]);
            for (const auto& c : exp) {
                CHECK(got.count(c) == 1);            // every sampled cell was visited (no miss)
            }
            CHECK_FALSE(got.empty());
        }
    }

    TEST_CASE("nearest-first + early-out: a bool callback stops at the first occupied cell") {
        grid<int> g(4, 4, vec{0, 0}, vec{8, 8});
        g.set(vec{3, 1}, 10);   // cell (1,0)
        g.set(vec{7, 1}, 30);   // cell (3,0) -- farther along the same row
        int first = -1, calls = 0;
        g.raycast(vec{0.5f, 1.0f}, vec{7.5f, 1.0f}, [&](const int& v, const aabb&, float) {
            ++calls;
            first = v;
            return false;        // stop at the first occupied cell
        });
        CHECK(calls == 1);
        CHECK(first == 10);      // the nearer solid, and we stopped there
    }

    TEST_CASE("a void callback visits all occupied cells along the ray") {
        grid<int> g(4, 4, vec{0, 0}, vec{8, 8});
        g.set(vec{3, 1}, 10);
        g.set(vec{7, 1}, 30);
        int calls = 0;
        g.raycast(vec{0.5f, 1.0f}, vec{7.5f, 1.0f}, [&](const int&, const aabb&, float) { ++calls; });
        CHECK(calls == 2);
    }

    TEST_CASE("a ray that misses the grid is a tolerant no-op") {
        grid<int> g(4, 4, vec{0, 0}, vec{8, 8});
        g.set(vec{1, 1}, 1);
        int calls = 0;
        CHECK_NOTHROW(g.raycast(vec{100, 100}, vec{200, 200},
                                [&](const int&, const aabb&, float) { ++calls; return true; }));
        CHECK(calls == 0);
    }
}

TEST_SUITE("grid: raycast supercover (corners & gridlines)") {
    // 1x1 cells over [0,N]^2 so cell coords == world coords (easy exact-corner setups).
    static std::vector<int> hits(grid<int>& g, vec a, vec b) {
        std::vector<int> v;
        g.raycast(a, b, [&](const int& e, const aabb&, float) { v.push_back(e); });
        return v;
    }

    TEST_CASE("diagonal through a corner hits the off-diagonal cell a plain DDA would skip") {
        grid<int> g(2, 2, vec{0, 0}, vec{2, 2}); // 1x1 cells; corner at (1,1)
        // Occupy ONLY cell (1,0) -- the side cell a tie-breaks-Y staircase would miss.
        g.set(vec{1.5f, 0.5f}, 10);
        const auto h = hits(g, vec{0.0f, 0.0f}, vec{2.0f, 2.0f}); // diagonal through the corner
        REQUIRE(h.size() == 1);
        CHECK(h[0] == 10); // supercover visits (1,0) -> no corner tunnelling
    }

    TEST_CASE("diagonal through a corner also hits the other off-diagonal cell") {
        grid<int> g(2, 2, vec{0, 0}, vec{2, 2});
        g.set(vec{0.5f, 1.5f}, 20); // cell (0,1) -- the other side cell
        const auto h = hits(g, vec{0.0f, 0.0f}, vec{2.0f, 2.0f});
        REQUIRE(h.size() == 1);
        CHECK(h[0] == 20);
    }

    TEST_CASE("a ray exactly on an internal grid line sees cells on BOTH sides") {
        grid<int> g(2, 2, vec{0, 0}, vec{2, 2}); // line y=1 between row 0 and row 1
        g.set(vec{0.5f, 0.5f}, 1); // cell (0,0), below the line
        g.set(vec{0.5f, 1.5f}, 2); // cell (0,1), above the line
        // horizontal ray running exactly along y=1
        const auto h = hits(g, vec{0.0f, 1.0f}, vec{2.0f, 1.0f});
        // both adjacent rows' first column are touched by the on-line ray
        CHECK(std::find(h.begin(), h.end(), 1) != h.end());
        CHECK(std::find(h.begin(), h.end(), 2) != h.end());
    }

    TEST_CASE("vertical ray on an internal grid line sees both columns") {
        grid<int> g(2, 2, vec{0, 0}, vec{2, 2}); // line x=1 between col 0 and col 1
        g.set(vec{0.5f, 0.5f}, 7); // col 0
        g.set(vec{1.5f, 0.5f}, 8); // col 1
        const auto h = hits(g, vec{1.0f, 0.0f}, vec{1.0f, 2.0f});
        CHECK(std::find(h.begin(), h.end(), 7) != h.end());
        CHECK(std::find(h.begin(), h.end(), 8) != h.end());
    }

    TEST_CASE("an off-line axis-aligned ray does NOT spuriously hit the neighbour row") {
        grid<int> g(2, 2, vec{0, 0}, vec{2, 2});
        g.set(vec{0.5f, 1.5f}, 99); // row 1 only
        // ray well inside row 0 (y=0.5, not on a line) -> must not see row 1
        const auto h = hits(g, vec{0.0f, 0.5f}, vec{2.0f, 0.5f});
        CHECK(h.empty());
    }
}

TEST_SUITE("grid: raycast long-ray parameter epsilon") {
    TEST_CASE("a long ray does not merge far-apart crossings into a false corner") {
        // 1x1 cells over [0,2]^2. The ray crosses x=1 then y=1 at clearly different points, but in
        // NORMALIZED parameter space those crossings differ by < 1e-6 -- a world eps would treat it
        // as a corner and spuriously visit (0,1), which the ray never touches.
        grid<int> g(2, 2, vec{0, 0}, vec{2, 2});
        g.set(vec{0.5f, 1.5f}, 42); // occupy ONLY cell (0,1)
        int calls = 0;
        g.raycast(vec{0.5f, 0.5f}, vec{1000000.5f, 500000.5f},
                  [&](const int&, const aabb&, float) { ++calls; });
        CHECK(calls == 0); // the ray goes (0,0)->(1,0)->(1,1); it must NOT report (0,1)
    }
}

TEST_SUITE("grid: raycast origin on a boundary (directional start)") {
    static std::vector<int> hits(grid<int>& g, vec a, vec b) {
        std::vector<int> v;
        g.raycast(a, b, [&](const int& e, const aabb&, float) { v.push_back(e); });
        return v;
    }

    // 2x1 grid over [0,2]x[0,1] -> 1x1 cells, vertical boundary at x=1 between (0,0) and (1,0).
    TEST_CASE("moving RIGHT off x=1: the left cell (behind) is not reported") {
        grid<int> g(2, 1, vec{0, 0}, vec{2, 1});
        g.set(vec{0.5f, 0.5f}, 100); // cell (0,0), left of the boundary -> behind a rightward ray
        const auto h = hits(g, vec{1.0f, 0.5f}, vec{1.75f, 0.5f});
        CHECK(h.empty()); // ray enters cell (1,0); (0,0) is behind, touched only at the origin
    }

    TEST_CASE("moving LEFT off x=1: the right cell (behind) is not reported") {
        grid<int> g(2, 1, vec{0, 0}, vec{2, 1});
        g.set(vec{1.5f, 0.5f}, 200); // cell (1,0), right of the boundary -> behind a leftward ray
        const auto h = hits(g, vec{1.0f, 0.5f}, vec{0.25f, 0.5f});
        CHECK(h.empty()); // start is directional -> (0,0); (1,0) behind, no spurious origin hit
    }

    TEST_CASE("moving LEFT off x=1: the forward (left) cell IS reported") {
        grid<int> g(2, 1, vec{0, 0}, vec{2, 1});
        g.set(vec{0.5f, 0.5f}, 300); // cell (0,0) -- the ray enters this going left
        const auto h = hits(g, vec{1.0f, 0.5f}, vec{0.25f, 0.5f});
        REQUIRE(h.size() == 1);
        CHECK(h[0] == 300);
    }

    // 1x2 grid over [0,1]x[0,2], horizontal boundary at y=1 between (0,0) and (0,1).
    TEST_CASE("moving DOWN off y=1: the upper cell (behind) is not reported") {
        grid<int> g(1, 2, vec{0, 0}, vec{1, 2});
        g.set(vec{0.5f, 1.5f}, 400); // cell (0,1), above the boundary -> behind a downward ray
        const auto h = hits(g, vec{0.5f, 1.0f}, vec{0.5f, 0.25f});
        CHECK(h.empty());
    }

    TEST_CASE("moving DOWN off y=1: the forward (lower) cell IS reported") {
        grid<int> g(1, 2, vec{0, 0}, vec{1, 2});
        g.set(vec{0.5f, 0.5f}, 500); // cell (0,0) -- entered going down
        const auto h = hits(g, vec{0.5f, 1.0f}, vec{0.5f, 0.25f});
        REQUIRE(h.size() == 1);
        CHECK(h[0] == 500);
    }
}

TEST_SUITE("grid: swept (moving-shape band)") {
    static std::set<std::pair<uint32_t, uint32_t>>
    band(grid<int>& g, const aabb& start_bound, vec delta) {
        std::set<std::pair<uint32_t, uint32_t>> s;
        g.swept(start_bound, delta, [&](const int& v, const aabb&) {
            s.insert({static_cast<uint32_t>(v % 4), static_cast<uint32_t>(v / 4)});
        });
        return s;
    }

    // 4x4 grid over [0,8]^2 -> 2x2 cells. Fill every cell so swept reports each visited cell.
    static grid<int> filled() {
        grid<int> g(4, 4, vec{0, 0}, vec{8, 8});
        for (uint32_t y = 0; y < 4; ++y)
            for (uint32_t x = 0; x < 4; ++x)
                g.set(vec{x * 2.0f + 1.0f, y * 2.0f + 1.0f}, static_cast<int>(y * 4 + x));
        return g;
    }

    TEST_CASE("the band covers the union of the bound at both ends (anti-tunnelling)") {
        auto g = filled();
        // a 1x1 box starting in cell (0,0), moving right by 5 -> ends spanning into cell (3,0).
        const aabb start{vec{0.5f, 0.5f}, vec{1.5f, 1.5f}};
        const auto cells = band(g, start, vec{5, 0});
        // start box touches cells x∈{0}, end box (x 5.5..6.5) touches x∈{2,3}; union spans the
        // whole swept rect rows y∈{0}, x∈{0,1,2,3} -- nothing in the path is skipped.
        for (uint32_t x = 0; x <= 3; ++x) {
            CHECK(cells.count({x, 0u}) == 1);
        }
    }

    TEST_CASE("negative delta sweeps the same band (union is direction-agnostic)") {
        auto g = filled();
        const aabb a{vec{5.5f, 0.5f}, vec{6.5f, 1.5f}}; // starts in cell (3,0)/(2,0)
        const auto fwd = band(g, a, vec{-5, 0});         // move left by 5
        const aabb b{vec{0.5f, 0.5f}, vec{1.5f, 1.5f}};
        const auto rev = band(g, b, vec{5, 0});          // move right by 5 from the other end
        CHECK(fwd == rev);                               // same swept rectangle of cells
    }

    TEST_CASE("a diagonal sweep covers the rectangle, not just a thin line") {
        auto g = filled();
        const aabb start{vec{0.5f, 0.5f}, vec{1.5f, 1.5f}};
        const auto cells = band(g, start, vec{5, 5}); // down-right diagonal move
        // the union rect spans x∈{0..3}, y∈{0..3}; a thin ray would miss the off-diagonal cells
        CHECK(cells.count({0u, 3u}) == 1);
        CHECK(cells.count({3u, 0u}) == 1);
    }

    TEST_CASE("skips empty cells and supports bool early-out") {
        grid<int> g(4, 4, vec{0, 0}, vec{8, 8});
        g.set(vec{1, 1}, 1);
        g.set(vec{3, 1}, 2);
        int seen = 0;
        g.swept(aabb{vec{0.5f, 0.5f}, vec{1.5f, 1.5f}}, vec{4, 0},
                [&](const int&, const aabb&) { ++seen; }); // void: visits both occupied
        CHECK(seen == 2);

        int calls = 0;
        g.swept(aabb{vec{0.5f, 0.5f}, vec{1.5f, 1.5f}}, vec{4, 0},
                [&](const int&, const aabb&) { ++calls; return false; }); // stop after first
        CHECK(calls == 1);
    }

    TEST_CASE("a sweep entirely outside the grid is a tolerant no-op") {
        auto g = filled();
        int calls = 0;
        CHECK_NOTHROW(g.swept(aabb{vec{100, 100}, vec{101, 101}}, vec{5, 5},
                              [&](const int&, const aabb&) { ++calls; }));
        CHECK(calls == 0);
    }
}

TEST_SUITE("grid: from_tile_size factory") {
    TEST_CASE("derives the extent from origin + tile_size * count; cell size == tile_size") {
        auto g = grid<int>::from_tile_size(vec{0, 0}, vec{2, 2}, 4, 3); // -> [0,8] x [0,6]
        // cell size is exactly tile_size, so physical_to_grid agrees with a 4x3 over [0,8]x[0,6]
        CHECK(grid_test_access::physical_to_grid(g, vec{0, 0}).x == 0u);
        CHECK(grid_test_access::physical_to_grid(g, vec{2, 2}).x == 1u);   // on a tile boundary
        CHECK(grid_test_access::physical_to_grid(g, vec{2, 2}).y == 1u);
        CHECK(grid_test_access::physical_to_grid(g, vec{7.9f, 5.9f}).x == 3u); // last column
        CHECK(grid_test_access::physical_to_grid(g, vec{7.9f, 5.9f}).y == 2u); // last row
        CHECK_FALSE(static_cast<bool>(grid_test_access::physical_to_grid(g, vec{8.1f, 0}))); // outside derived extent
    }

    TEST_CASE("non-origin tile grid") {
        auto g = grid<int>::from_tile_size(vec{-10, -10}, vec{5, 5}, 2, 2); // -> [-10,0] x [-10,0]
        g.set(vec{-9, -9}, 1);  // cell (0,0)
        g.set(vec{-1, -1}, 2);  // cell (1,1)
        CHECK(*g.get(vec{-9, -9}) == 1);
        CHECK(*g.get(vec{-1, -1}) == 2);
        CHECK(g.get(vec{1, 1}) == nullptr); // outside
    }
}

TEST_SUITE("grid: compile_runs (boundary merge)") {
    TEST_CASE("merges maximal rectangles of same-group occupied cells and clears them") {
        grid<int> g = grid<int>::from_tile_size(vec{0, 0}, vec{2, 2}, 4, 4); // 4x4 cells of 2x2
        g.set(vec{1, 1}, 5); g.set(vec{3, 1}, 5); g.set(vec{5, 1}, 5);        // 3-wide run of 5 (row 0)
        g.set(vec{1, 5}, 7); g.set(vec{3, 5}, 7); g.set(vec{1, 7}, 7); g.set(vec{3, 7}, 7); // 2x2 of 7
        g.set(vec{7, 7}, 9);                                                   // isolated 9
        g.set(vec{7, 1}, 0);                                                   // group 0 -> never merged

        const auto same = [](const int& a, const int& b, const aabb&) { return b != 0 && a == b; };
        std::vector<std::pair<aabb, int>> runs;
        g.compile_runs(same, [&](const aabb& r, const int& s) { runs.push_back({r, s}); });

        int n5 = 0, n7 = 0, n9 = 0;
        for (const auto& [r, s] : runs) { n5 += s == 5; n7 += s == 7; n9 += s == 9; }
        CHECK(runs.size() == 3u);
        CHECK((n5 == 1 && n7 == 1 && n9 == 1));
        for (const auto& [r, s] : runs) {
            if (s == 5) { CHECK(r.min.x() == 0.0f); CHECK(r.max.x() == 6.0f); CHECK(r.max.y() == 2.0f); }
            if (s == 7) { CHECK(r.max.x() == 4.0f); CHECK(r.min.y() == 4.0f); CHECK(r.max.y() == 8.0f); }
        }
        // merged cells are cleared; the un-grouped cell is kept
        CHECK(g.get(vec{1, 1}) == nullptr);
        CHECK(g.get(vec{1, 5}) == nullptr);
        CHECK(g.get(vec{7, 7}) == nullptr);
        const int* kept = g.get(vec{7, 1});
        REQUIRE(kept != nullptr);
        CHECK(*kept == 0);
    }
}
