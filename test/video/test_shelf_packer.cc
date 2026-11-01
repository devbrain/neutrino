//
// Tests for the private shelf packer behind build_atlas: placement geometry, page splits,
// trimming, input order, zero-size entries, and input validation.
//
#include <doctest/doctest.h>

#include "video/atlas/shelf_packer.hh"

#include <cstddef>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

using namespace neutrino;
using neutrino::details::shelf_layout;
using neutrino::details::shelf_pack;

template<>
struct neutrino::details::dimensions_access <dim> {
    static dim load_dim(const dim& d) { return d; }
};

namespace {
    shelf_layout pack(const std::vector <dim>& sizes, dim max_page, int gutter) {
        return shelf_pack(std::span <const dim>(sizes), max_page, gutter);
    }

    // The cell a placement occupies: its rect grown by the gutter on every side.
    rect cell_of(const rect& r, int gutter) {
        return rect{r.x - gutter, r.y - gutter, r.w + 2 * gutter, r.h + 2 * gutter};
    }

    bool overlaps(const rect& a, const rect& b) {
        return a.x < b.x + b.w && b.x < a.x + a.w && a.y < b.y + b.h && b.y < a.y + a.h;
    }

    // Properties every layout must have, whatever the input.
    void check_invariants(const std::vector <dim>& sizes, dim max_page, int gutter, const shelf_layout& l) {
        REQUIRE(l.placements.size() == sizes.size());

        std::vector <bool> touches_right(l.pages.size(), false);
        std::vector <bool> touches_bottom(l.pages.size(), false);

        for (std::size_t i = 0; i < sizes.size(); i++) {
            const auto& p = l.placements[i];
            CAPTURE(i);
            if (sizes[i].width == 0 || sizes[i].height == 0) {
                CHECK(p.dest.w == 0);
                CHECK(p.dest.h == 0);
                continue;
            }
            CHECK(p.dest.w == sizes[i].width);
            CHECK(p.dest.h == sizes[i].height);
            REQUIRE(p.page < l.pages.size());

            const dim page = l.pages[p.page];
            const rect c = cell_of(p.dest, gutter);
            CHECK(c.x >= 0);
            CHECK(c.y >= 0);
            CHECK(c.x + c.w <= page.width);
            CHECK(c.y + c.h <= page.height);
            if (c.x + c.w == page.width) {
                touches_right[p.page] = true;
            }
            if (c.y + c.h == page.height) {
                touches_bottom[p.page] = true;
            }

            for (std::size_t j = i + 1; j < sizes.size(); j++) {
                const auto& q = l.placements[j];
                if (q.page != p.page || q.dest.w == 0 || q.dest.h == 0) {
                    continue;
                }
                CAPTURE(j);
                CHECK_FALSE(overlaps(c, cell_of(q.dest, gutter)));
            }
        }

        for (std::size_t k = 0; k < l.pages.size(); k++) {
            CAPTURE(k);
            CHECK(l.pages[k].width <= max_page.width);
            CHECK(l.pages[k].height <= max_page.height);
            // trimmed: some cell reaches the right edge and some reaches the bottom
            CHECK(touches_right[k]);
            CHECK(touches_bottom[k]);
        }
    }
}

TEST_SUITE("neutrino::details::shelf_pack") {
    TEST_CASE("empty input gives no pages and no placements") {
        const std::vector <dim> sizes;
        const auto l = pack(sizes, {64, 64}, 1);
        CHECK(l.pages.empty());
        CHECK(l.placements.empty());
    }

    TEST_CASE("a single picture sits inside its gutter on a page of exactly its cell") {
        const std::vector <dim> sizes{{10, 7}};
        const auto l = pack(sizes, {64, 64}, 1);
        check_invariants(sizes, {64, 64}, 1, l);
        REQUIRE(l.pages.size() == 1);
        CHECK(l.pages[0] == dim{12, 9});
        CHECK(l.placements[0].page == 0);
        CHECK(l.placements[0].dest == rect{1, 1, 10, 7});
    }

    TEST_CASE("uniform pictures form a squarish grid in input order") {
        const std::vector <dim> sizes(300, dim{16, 16});
        const auto l = pack(sizes, {4096, 4096}, 1);
        check_invariants(sizes, {4096, 4096}, 1, l);

        // cells are 18x18; sqrt(300 * 324) ~ 312 -> 17 columns, 18 rows
        REQUIRE(l.pages.size() == 1);
        CHECK(l.pages[0] == dim{306, 324});
        for (std::size_t i = 0; i < sizes.size(); i++) {
            CAPTURE(i);
            const int col = static_cast <int>(i % 17);
            const int row = static_cast <int>(i / 17);
            CHECK(l.placements[i].dest == rect{col * 18 + 1, row * 18 + 1, 16, 16});
        }
    }

    TEST_CASE("taller pictures go on earlier shelves; positions stay with their inputs") {
        const std::vector <dim> sizes{{5, 5}, {5, 20}, {5, 10}};
        const auto l = pack(sizes, {64, 64}, 0);
        check_invariants(sizes, {64, 64}, 0, l);

        // page width = ceil(sqrt(175)) = 14: the 20 and the 10 share shelf 1, the 5 starts shelf 2
        REQUIRE(l.pages.size() == 1);
        CHECK(l.pages[0] == dim{10, 25});
        CHECK(l.placements[1].dest == rect{0, 0, 5, 20});
        CHECK(l.placements[2].dest == rect{5, 0, 5, 10});
        CHECK(l.placements[0].dest == rect{0, 20, 5, 5});
    }

    TEST_CASE("pictures of equal height keep their input order") {
        const std::vector <dim> sizes{{3, 4}, {9, 4}, {1, 4}, {6, 4}};
        const auto l = pack(sizes, {64, 64}, 0);
        check_invariants(sizes, {64, 64}, 0, l);
        for (std::size_t i = 1; i < sizes.size(); i++) {
            const auto& a = l.placements[i - 1].dest;
            const auto& b = l.placements[i].dest;
            CAPTURE(i);
            CHECK((b.y > a.y || (b.y == a.y && b.x > a.x)));
        }
    }

    TEST_CASE("the page is at least as wide as the widest picture") {
        std::vector <dim> sizes{{100, 4}};
        sizes.insert(sizes.end(), 5, dim{4, 4});
        const auto l = pack(sizes, {256, 256}, 0);
        check_invariants(sizes, {256, 256}, 0, l);
        REQUIRE(l.pages.size() == 1);
        CHECK(l.pages[0] == dim{100, 8});
    }

    TEST_CASE("a full page starts a new one, and the last page is trimmed") {
        const std::vector <dim> sizes(10, dim{30, 30});
        const auto l = pack(sizes, {64, 64}, 0);
        check_invariants(sizes, {64, 64}, 0, l);

        // two cells per shelf, two shelves per page
        REQUIRE(l.pages.size() == 3);
        CHECK(l.pages[0] == dim{60, 60});
        CHECK(l.pages[1] == dim{60, 60});
        CHECK(l.pages[2] == dim{60, 30});
        for (std::size_t i = 0; i < sizes.size(); i++) {
            CAPTURE(i);
            CHECK(l.placements[i].page == i / 4);
            const int x = static_cast <int>(i % 2) * 30;
            const int y = static_cast <int>((i % 4) / 2) * 30;
            CHECK(l.placements[i].dest == rect{x, y, 30, 30});
        }
    }

    TEST_CASE("a cell exactly the page size fits") {
        const std::vector <dim> sizes{{64, 64}, {62, 62}};
        const auto l0 = pack({sizes[0]}, {64, 64}, 0);
        REQUIRE(l0.pages.size() == 1);
        CHECK(l0.pages[0] == dim{64, 64});

        const auto l1 = pack({sizes[1]}, {64, 64}, 1);
        REQUIRE(l1.pages.size() == 1);
        CHECK(l1.pages[0] == dim{64, 64});
        CHECK(l1.placements[0].dest == rect{1, 1, 62, 62});
    }

    TEST_CASE("a cell one pixel over the page throws, naming the entry") {
        CHECK_THROWS_AS((void)pack({{63, 10}}, {64, 64}, 1), std::runtime_error);
        CHECK_THROWS_AS((void)pack({{10, 63}}, {64, 64}, 1), std::runtime_error);
        try {
            (void)pack({{4, 4}, {65, 4}}, {64, 64}, 0);
            FAIL("expected a throw");
        } catch (const std::runtime_error& e) {
            const std::string what = e.what();
            CHECK(what.find("Entry 1") != std::string::npos);
            CHECK(what.find("65") != std::string::npos);
        }
    }

    TEST_CASE("zero-size pictures keep their position with an empty rect") {
        const std::vector <dim> sizes{{0, 0}, {8, 8}, {0, 5}, {8, 8}};
        const auto l = pack(sizes, {64, 64}, 0);
        check_invariants(sizes, {64, 64}, 0, l);
        REQUIRE(l.placements.size() == 4);
        CHECK(l.placements[0].dest == rect{0, 0, 0, 0});
        CHECK(l.placements[2].dest == rect{0, 0, 0, 0});
        REQUIRE(l.pages.size() == 1);
        CHECK(l.pages[0] == dim{8, 16});
    }

    TEST_CASE("only zero-size pictures: placements for each, no pages") {
        const std::vector <dim> sizes{{0, 0}, {0, 3}};
        const auto l = pack(sizes, {64, 64}, 1);
        CHECK(l.pages.empty());
        CHECK(l.placements.size() == 2);
    }

    TEST_CASE("invalid arguments throw") {
        CHECK_THROWS_AS((void)pack({{4, 4}}, {64, 64}, -1), std::runtime_error);
        CHECK_THROWS_AS((void)pack({{-1, 4}}, {64, 64}, 0), std::runtime_error);
        CHECK_THROWS_AS((void)pack({{4, 4}}, {0, 64}, 0), std::runtime_error);
        CHECK_THROWS_AS((void)pack({{4, 4}}, {64, 0}, 0), std::runtime_error);
    }

    TEST_CASE("the same input gives the same layout") {
        const std::vector <dim> sizes{{7, 3}, {2, 9}, {7, 3}, {5, 5}, {1, 1}};
        const auto a = pack(sizes, {16, 16}, 1);
        const auto b = pack(sizes, {16, 16}, 1);
        REQUIRE(a.pages == b.pages);
        REQUIRE(a.placements.size() == b.placements.size());
        for (std::size_t i = 0; i < a.placements.size(); i++) {
            CHECK(a.placements[i].page == b.placements[i].page);
            CHECK(a.placements[i].dest == b.placements[i].dest);
        }
    }

    TEST_CASE("random inputs keep every invariant") {
        std::mt19937 rng(12345);
        auto roll = [&rng](int lo, int hi) { return std::uniform_int_distribution <int>(lo, hi)(rng); };

        for (int round = 0; round < 300; round++) {
            const int gutter = roll(0, 2);
            const dim max_page{roll(16, 128), roll(16, 128)};
            std::vector <dim> sizes(static_cast <std::size_t>(roll(0, 60)));
            for (auto& s : sizes) {
                s = dim{roll(0, max_page.width - 2 * gutter), roll(0, max_page.height - 2 * gutter)};
            }
            CAPTURE(round);
            check_invariants(sizes, max_page, gutter, pack(sizes, max_page, gutter));
        }
    }
}
