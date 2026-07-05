//
// Created by igor on 25/06/2026.
//
// Solid-triangle narrow-phase (<neutrino/physics/geometry/triangle.hh>): contains / enclose / translate,
// the intersects matrix (triangle vs segment/circle/aabb/triangle, both orders), intersect_param
// (raycast), and swept_intersection (a moving aabb/circle vs a triangle). The triangle is solid,
// so it must block from every side -- the directional sweep cases below are the "no leak" check.
//
#include <doctest/doctest.h>

#include <cmath>

#include <neutrino/physics/geometry/shapes.hh>

using namespace neutrino::physics;

namespace {
    // lower-left right triangle: legs on the axes, hypotenuse (4,0)-(0,4); interior is x+y<4, x,y>0.
    const triangle T{vec{0, 0}, vec{4, 0}, vec{0, 4}};
}

TEST_SUITE("triangle: contains / enclose / translate") {
    TEST_CASE("contains is inclusive of edges and vertices") {
        CHECK(contains(T, vec{1, 1}));
        CHECK(contains(T, vec{2, 2}));   // on the hypotenuse
        CHECK(contains(T, vec{0, 0}));   // vertex
        CHECK_FALSE(contains(T, vec{3, 3}));
        CHECK_FALSE(contains(T, vec{-1, 1}));
    }

    TEST_CASE("enclose is the vertex bounding box; translate shifts all vertices") {
        const aabb b = enclose(T);
        CHECK(b.min.x() == 0.0f); CHECK(b.min.y() == 0.0f);
        CHECK(b.max.x() == 4.0f); CHECK(b.max.y() == 4.0f);
        const triangle s = translate(T, vec{10, -3});
        CHECK(enclose(s).min.x() == 10.0f);
        CHECK(enclose(s).min.y() == -3.0f);
    }
}

TEST_SUITE("triangle: intersects matrix") {
    TEST_CASE("vs circle") {
        CHECK(intersects(T, circle{vec{1, 1}, 0.5f}));   // inside
        CHECK(intersects(T, circle{vec{3, 3}, 2.0f}));   // crosses the hypotenuse
        CHECK_FALSE(intersects(T, circle{vec{5, 5}, 0.5f}));
        CHECK(intersects(circle{vec{1, 1}, 0.5f}, T));   // order-independent
    }
    TEST_CASE("vs aabb") {
        CHECK(intersects(T, aabb{vec{-1, -1}, vec{0.5f, 0.5f}})); // partial overlap
        CHECK(intersects(T, aabb{vec{1, 1}, vec{1.5f, 1.5f}}));   // box inside triangle
        CHECK(intersects(T, aabb{vec{-1, -1}, vec{5, 5}}));       // box contains triangle
        CHECK_FALSE(intersects(T, aabb{vec{5, 5}, vec{6, 6}}));
        CHECK(intersects(aabb{vec{-1, -1}, vec{0.5f, 0.5f}}, T)); // order-independent
    }
    TEST_CASE("vs segment") {
        CHECK(intersects(T, segment{vec{-1, 1}, vec{5, 1}}));     // crosses
        CHECK(intersects(T, segment{vec{1, 1}, vec{1.5f, 1.5f}})); // inside
        CHECK_FALSE(intersects(T, segment{vec{5, 5}, vec{6, 6}}));
        CHECK(intersects(segment{vec{-1, 1}, vec{5, 1}}, T));     // order-independent
    }
    TEST_CASE("vs triangle") {
        CHECK(intersects(T, triangle{vec{1, 1}, vec{2, 1}, vec{1, 2}}));
        CHECK_FALSE(intersects(T, triangle{vec{10, 10}, vec{12, 10}, vec{10, 12}}));
    }
}

TEST_SUITE("triangle: intersect_param (raycast)") {
    TEST_CASE("entry/exit params and outward entry normal") {
        const auto h = intersect_param(T, segment{vec{-1, 1}, vec{5, 1}}); // horizontal at y=1
        REQUIRE(h.has_value());
        CHECK(h->segment_overlaps());
        CHECK(h->entry_param == doctest::Approx(1.0f / 6.0f)); // enters the left leg x=0
        CHECK(h->exit_param == doctest::Approx(4.0f / 6.0f));  // exits the hypotenuse at x=3
        CHECK(h->entry_normal.x() == doctest::Approx(-1.0f));  // left leg's outward normal
        CHECK(h->entry_normal.y() == doctest::Approx(0.0f));
    }
    TEST_CASE("a miss returns nullopt") {
        CHECK_FALSE(intersect_param(T, segment{vec{-1, 5}, vec{5, 5}}).has_value());
    }
    TEST_CASE("brute-force: entry param matches the first densely-sampled inside point") {
        const vec rays[][2] = {
            {{-1, 1}, {5, 1}}, {{2, -1}, {2, 5}}, {{-1, -1}, {3, 3}}, {{5, 0.5f}, {-1, 0.5f}},
        };
        for (const auto& r : rays) {
            const segment s{r[0], r[1]};
            const auto h = intersect_param(T, s);
            // dense scan for the first t in [0,1] whose point is inside the triangle
            float first = -1.0f;
            for (int i = 0; i <= 20000; ++i) {
                const float t = i / 20000.0f;
                if (contains(T, s.point_in_time(t))) { first = t; break; }
            }
            if (first < 0.0f) {
                const bool overlaps = h.has_value() && h->segment_overlaps();
                CHECK_FALSE(overlaps); // sampler found nothing -> no hit
            } else {
                REQUIRE(h.has_value());
                const float toi = std::max(0.0f, h->entry_param);
                CHECK(toi == doctest::Approx(first).epsilon(0.01)); // entry ~ first inside sample
            }
        }
    }
}

TEST_SUITE("triangle: swept_intersection (no leak from any side)") {
    static bool hits(aabb m, vec d) { return swept_intersection(m, d, T, vec{0, 0}, 1.0f).has_value(); }

    TEST_CASE("a moving box is blocked approaching from every side") {
        CHECK(hits(aabb{vec{-2, 0.6f}, vec{-1.5f, 1.0f}}, vec{6, 0}));  // from left  -> left leg
        CHECK(hits(aabb{vec{1.0f, -2}, vec{1.4f, -1.5f}}, vec{0, 6}));  // from below -> bottom leg
        CHECK(hits(aabb{vec{5, 5}, vec{5.5f, 5.5f}}, vec{-6, -6}));     // upper-right-> hypotenuse
        CHECK_FALSE(hits(aabb{vec{10, 10}, vec{10.5f, 10.5f}}, vec{1, 0})); // moving away -> miss
    }

    TEST_CASE("entry normal opposes the approach (outward from the solid)") {
        const auto h = swept_intersection(aabb{vec{-2, 0.6f}, vec{-1.5f, 1.0f}}, vec{6, 0}, T, vec{0, 0}, 1.0f);
        REQUIRE(h.has_value());
        CHECK(h->entry_normal.x() < -0.5f); // outward (-x), against the +x motion
        CHECK(h->entry_time > 0.0f);
        CHECK(h->entry_time < 1.0f);
    }

    TEST_CASE("a circle mover is handled too") {
        CHECK(swept_intersection(circle{vec{-2, 1}, 0.3f}, vec{6, 0}, T, vec{0, 0}, 1.0f).has_value());
    }

    TEST_CASE("a mover starting inside reports a toi-0 contact") {
        const auto h = swept_intersection(aabb{vec{0.9f, 0.9f}, vec{1.1f, 1.1f}}, vec{0, 0}, T, vec{0, 0}, 1.0f);
        REQUIRE(h.has_value());
        CHECK(h->entry_time == doctest::Approx(0.0f));
    }

    TEST_CASE("a mover fully inside with NONZERO velocity still reports toi 0 (not a future edge)") {
        // box inside the triangle, moving toward the left leg: the penetration it starts in wins.
        const auto h = swept_intersection(aabb{vec{1.0f, 1.0f}, vec{1.4f, 1.4f}}, vec{-4, 0}, T, vec{0, 0}, 1.0f);
        REQUIRE(h.has_value());
        CHECK(h->entry_time == doctest::Approx(0.0f));
        const auto hc = swept_intersection(circle{vec{1.0f, 1.0f}, 0.3f}, vec{0, -4}, T, vec{0, 0}, 1.0f);
        REQUIRE(hc.has_value());
        CHECK(hc->entry_time == doctest::Approx(0.0f));
    }

    TEST_CASE("a mover starting inside and LEAVING reports the real exit interval, not time") {
        // box inside T, moving left at -4/unit-time. It clears the left leg (x=0) when its right
        // edge (x=1.4) passes 0 -> t = 1.4/4 = 0.35, well before time == 1.
        const auto h = swept_intersection(aabb{vec{1.0f, 1.0f}, vec{1.4f, 1.4f}}, vec{-4, 0}, T, vec{0, 0}, 1.0f);
        REQUIRE(h.has_value());
        CHECK(h->entry_time == doctest::Approx(0.0f));
        CHECK(h->exit_time == doctest::Approx(0.35f).epsilon(0.02)); // NOT 1.0
    }

    TEST_CASE("a mover fully inside that STAYS inside the whole sweep exits at time") {
        // tiny box deep inside, tiny motion -> never reaches an edge -> overlaps for the whole window
        const auto h = swept_intersection(aabb{vec{0.9f, 0.9f}, vec{1.1f, 1.1f}}, vec{0.01f, 0.0f}, T, vec{0, 0}, 1.0f);
        REQUIRE(h.has_value());
        CHECK(h->entry_time == doctest::Approx(0.0f));
        CHECK(h->exit_time == doctest::Approx(1.0f));
    }

    TEST_CASE("a mover that enters and ENDS inside exits at time, not the entry edge's exit") {
        // box left of T moving right, ending inside: enters the left leg ~0.53, stays inside at t=1.
        const auto h = swept_intersection(aabb{vec{-2.0f, 1.0f}, vec{-1.6f, 1.4f}}, vec{3, 0}, T, vec{0, 0}, 1.0f);
        REQUIRE(h.has_value());
        CHECK(h->entry_time > 0.0f);
        CHECK(h->entry_time < 1.0f);
        CHECK(h->exit_time == doctest::Approx(1.0f)); // ends inside -> no separation in the window
    }

    TEST_CASE("a MOVING triangle anchors the end normal against its end position (tv != 0)") {
        // stationary mover at (6,0.5); triangle slides right by (3,0). At t=1 the mover is inside the
        // moved triangle {(3,0),(7,0),(3,4)}, nearest to its HYPOTENUSE -> outward ~ (+,+). Computing
        // the nearest edge against the original (un-moved) triangle would pick the bottom leg (~0,-1).
        const aabb mover{vec{5.9f, 0.4f}, vec{6.1f, 0.6f}};
        const auto h = swept_intersection(mover, vec{0, 0}, T, vec{3, 0}, 1.0f);
        REQUIRE(h.has_value());
        CHECK(h->exit_time == doctest::Approx(1.0f));   // ends inside the moved triangle
        CHECK(h->exit_normal.x() > 0.5f);               // hypotenuse outward (+x,+y), not the bottom leg
        CHECK(h->exit_normal.y() > 0.5f);
    }

    TEST_CASE("a pass-through mover's exit is the far boundary, not the entry edge's exit") {
        // box left of T moving right all the way across: enters the left leg, exits the hypotenuse.
        const auto h = swept_intersection(aabb{vec{-2.0f, 0.4f}, vec{-1.6f, 0.8f}}, vec{12, 0}, T, vec{0, 0}, 1.0f);
        REQUIRE(h.has_value());
        CHECK(h->entry_time > 0.0f);
        CHECK(h->exit_time > h->entry_time);   // a real interval
        CHECK(h->exit_time < 1.0f);            // it has fully cleared the triangle before the end
    }
}

TEST_SUITE("triangle: degenerate (collinear) behaves like its longest edge") {
    TEST_CASE("contains accepts only points on the longest edge, not the whole line") {
        const triangle deg{vec{0, 0}, vec{2, 0}, vec{4, 0}}; // collinear on the x-axis; longest edge [0,4]
        CHECK(contains(deg, vec{1, 0}));    // on the segment
        CHECK(contains(deg, vec{4, 0}));    // endpoint
        CHECK_FALSE(contains(deg, vec{5, 0}));  // on the line but past the longest edge
        CHECK_FALSE(contains(deg, vec{-1, 0})); // on the line but before it
        CHECK_FALSE(contains(deg, vec{1, 0.5f})); // off the line
    }
}
