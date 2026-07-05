//
// Created by igor on 22/06/2026.
//
// Unit tests for the shape -> bounding-aabb helper in <neutrino/physics/geometry/enclose.hh>:
// enclose(segment), enclose(aabb) and enclose(circle). enclose only ever selects
// min/max coordinates (segment, aabb) or adds/subtracts the radius (circle), so for
// exactly-representable inputs the result is bit-exact -- the checks use exact float
// comparison on purpose. A few static_asserts pin down that enclose stays constexpr.
//
#include <doctest/doctest.h>

#include <neutrino/physics/geometry/enclose.hh>

using namespace neutrino::physics;

namespace {
    bool box_eq(const aabb& a, const aabb& b) {
        return a.min.x() == b.min.x() && a.min.y() == b.min.y()
            && a.max.x() == b.max.x() && a.max.y() == b.max.y();
    }

    // enclose must never under-cover: the returned box is well-formed (min <= max).
    bool well_formed(const aabb& b) {
        return b.min.x() <= b.max.x() && b.min.y() <= b.max.y();
    }
} // namespace

TEST_SUITE("enclose: segment") {
    TEST_CASE("axis-aligned segment (already ordered endpoints)") {
        const segment s{{1.0f, 2.0f}, {4.0f, 6.0f}};
        const aabb b = enclose(s);
        CHECK(box_eq(b, aabb{{1.0f, 2.0f}, {4.0f, 6.0f}}));
        CHECK(well_formed(b));
    }

    TEST_CASE("reversed endpoints produce the same box (to < from)") {
        const segment forward{{1.0f, 2.0f}, {4.0f, 6.0f}};
        const segment reversed{{4.0f, 6.0f}, {1.0f, 2.0f}};
        CHECK(box_eq(enclose(forward), enclose(reversed)));
        CHECK(box_eq(enclose(reversed), aabb{{1.0f, 2.0f}, {4.0f, 6.0f}}));
    }

    TEST_CASE("mixed-direction diagonal (x increases, y decreases)") {
        const segment s{{1.0f, 6.0f}, {4.0f, 2.0f}};
        const aabb b = enclose(s);
        CHECK(box_eq(b, aabb{{1.0f, 2.0f}, {4.0f, 6.0f}}));
        CHECK(well_formed(b));
    }

    TEST_CASE("negative coordinates") {
        const segment s{{-3.0f, -1.0f}, {-5.0f, 2.0f}};
        const aabb b = enclose(s);
        CHECK(box_eq(b, aabb{{-5.0f, -1.0f}, {-3.0f, 2.0f}}));
        CHECK(well_formed(b));
    }

    TEST_CASE("degenerate segment (a point) -> zero-area box") {
        const segment s{{2.5f, -2.5f}, {2.5f, -2.5f}};
        const aabb b = enclose(s);
        CHECK(box_eq(b, aabb{{2.5f, -2.5f}, {2.5f, -2.5f}}));
        CHECK(b.size().x() == 0.0f);
        CHECK(b.size().y() == 0.0f);
    }

    TEST_CASE("horizontal and vertical segments") {
        CHECK(box_eq(enclose(segment{{-2.0f, 3.0f}, {5.0f, 3.0f}}),  // horizontal
                     aabb{{-2.0f, 3.0f}, {5.0f, 3.0f}}));
        CHECK(box_eq(enclose(segment{{3.0f, 5.0f}, {3.0f, -2.0f}}),  // vertical, reversed
                     aabb{{3.0f, -2.0f}, {3.0f, 5.0f}}));
    }
}

TEST_SUITE("enclose: aabb") {
    TEST_CASE("identity: enclosing a box returns the same box") {
        const aabb in{{-1.5f, 0.0f}, {3.0f, 4.5f}};
        CHECK(box_eq(enclose(in), in));
    }

    TEST_CASE("zero-area box round-trips unchanged") {
        const aabb in{{2.0f, 2.0f}, {2.0f, 2.0f}};
        CHECK(box_eq(enclose(in), in));
    }
}

TEST_SUITE("enclose: circle") {
    TEST_CASE("circle at origin") {
        const circle c{{0.0f, 0.0f}, 2.0f};
        const aabb b = enclose(c);
        CHECK(box_eq(b, aabb{{-2.0f, -2.0f}, {2.0f, 2.0f}}));
        CHECK(well_formed(b));
        CHECK(b.size().x() == 4.0f); // 2 * radius
        CHECK(b.size().y() == 4.0f);
    }

    TEST_CASE("off-center circle, negative coordinates") {
        const circle c{{-3.0f, 5.0f}, 1.5f};
        const aabb b = enclose(c);
        CHECK(box_eq(b, aabb{{-4.5f, 3.5f}, {-1.5f, 6.5f}}));
        CHECK(box_eq(b, aabb{{c.center.x() - c.radius, c.center.y() - c.radius},
                             {c.center.x() + c.radius, c.center.y() + c.radius}}));
    }

    TEST_CASE("zero-radius circle degenerates to its center point") {
        const circle c{{7.0f, -4.0f}, 0.0f};
        const aabb b = enclose(c);
        CHECK(box_eq(b, aabb{{7.0f, -4.0f}, {7.0f, -4.0f}}));
        CHECK(b.size().x() == 0.0f);
        CHECK(b.size().y() == 0.0f);
    }

    TEST_CASE("box center equals circle center") {
        const circle c{{2.0f, -6.0f}, 3.0f};
        const aabb b = enclose(c);
        CHECK(b.center().x() == c.center.x());
        CHECK(b.center().y() == c.center.y());
    }
}

TEST_SUITE("enclose: constexpr") {
    // enclose carries no sqrt and only does min/max/+/-, so every overload is usable
    // in a constant expression. These fail at compile time if that ever regresses.
    TEST_CASE("usable in constant expressions") {
        constexpr aabb sb = enclose(segment{{4.0f, 1.0f}, {1.0f, 3.0f}});
        static_assert(sb.min.x() == 1.0f && sb.min.y() == 1.0f);
        static_assert(sb.max.x() == 4.0f && sb.max.y() == 3.0f);

        constexpr aabb ab = enclose(aabb{{0.0f, 0.0f}, {1.0f, 1.0f}});
        static_assert(ab.min.x() == 0.0f && ab.max.y() == 1.0f);

        constexpr aabb cb = enclose(circle{{1.0f, 1.0f}, 2.0f});
        static_assert(cb.min.x() == -1.0f && cb.max.x() == 3.0f);

        CHECK(true); // keep doctest happy; the real assertions are above
    }
}
