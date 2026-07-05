//
// Unit tests for neutrino::physics — pure world-space collision math.
//
// Covers the full public surface of <neutrino/physics/geometry/shapes.hh>: containment,
// static overlap tests, the slab AABB/segment sweep, closest-point / squared-
// distance queries, circle/segment and segment/segment intersection, and the
// three continuous-collision-detection (CCD) overloads. Also pins the edge
// cases surfaced in review: no NaN for zero-duration sweeps that start
// overlapped, point-on-segment for degenerate segments, numerically stable
// point-to-segment distance, scale-consistent degeneracy thresholds, tangent
// circle hits, deterministic collinear normals, and convex exit selection.
//
// Query categories under test:
//   intersects(A, B)              -> bool       static overlap
//   intersect_param(shape, seg)   -> line_hit   raw geometry (segment parameters)
//   swept_intersection(A,av,B,bv,t) -> swept_hit  CCD (absolute seconds)
//

#include <doctest/doctest.h>
#include <cmath>

#include <neutrino/physics/geometry/shapes.hh>

using namespace neutrino::physics;

namespace {
    bool vapprox(const vec& a, const vec& b, float eps = 1e-4f) {
        return std::fabs(a.x() - b.x()) <= eps && std::fabs(a.y() - b.y()) <= eps;
    }

    bool is_unit(const vec& v, float eps = 1e-4f) {
        return std::fabs(std::sqrt(v.x() * v.x() + v.y() * v.y()) - 1.0f) <= eps;
    }

    bool finite(float f) { return std::isfinite(f); }

    bool finite(const line_hit& e) {
        return finite(e.entry_param) && finite(e.exit_param) &&
               finite(e.entry_normal.x()) && finite(e.entry_normal.y()) &&
               finite(e.exit_normal.x()) && finite(e.exit_normal.y());
    }

    bool finite(const swept_hit& e) {
        return finite(e.entry_time) && finite(e.exit_time) &&
               finite(e.entry_normal.x()) && finite(e.entry_normal.y()) &&
               finite(e.exit_normal.x()) && finite(e.exit_normal.y());
    }
}

// Compile-time contract: the queries documented as constexpr must evaluate at compile time.
namespace constexpr_checks {
    constexpr aabb box{{0.0f, 0.0f}, {4.0f, 2.0f}};
    constexpr circle circ{{1.0f, 1.0f}, 1.0f};
    constexpr segment seg{{0.0f, 0.0f}, {10.0f, 4.0f}};

    static_assert(box.size().x() == 4.0f && box.center().y() == 1.0f);
    static_assert(seg.point_in_time(0.5f).x() == 5.0f);
    static_assert(contains(box, vec{2.0f, 1.0f}));
    static_assert(intersects(box, aabb{{1.0f, 1.0f}, {3.0f, 3.0f}}));
    static_assert(intersects(circ, box) && intersects(box, circ));   // both overloads, constexpr
    static_assert(squared_distance(vec{6.0f, 1.0f}, box) == 4.0f);
    static_assert(intersect_param(box, seg).has_value());
    static_assert(closest_point(vec{-1.0f, 1.0f}, box).x() == 0.0f);

    // Newly constexpr now that euler's expression evaluation is constexpr:
    // point/segment distance + closest point, and the aabb/aabb sweep.
    constexpr segment hseg{{0.0f, 0.0f}, {10.0f, 0.0f}};
    static_assert(closest_parameter(vec{5.0f, 5.0f}, hseg) == 0.5f);
    static_assert(closest_point(vec{5.0f, 5.0f}, hseg).x() == 5.0f);
    static_assert(squared_distance(vec{5.0f, 3.0f}, hseg) == 9.0f);
    static_assert(swept_intersection(aabb{{0.0f, 0.0f}, {2.0f, 2.0f}}, vec{10.0f, 0.0f},
                                     aabb{{5.0f, 0.0f}, {7.0f, 2.0f}}, vec{0.0f, 0.0f}, 1.0f)
                      ->entry_time == 0.3f);

    // New static-overlap matrix entries (all constexpr):
    static_assert(contains(hseg, vec{5.0f, 0.0f}));
    static_assert(intersects(hseg, aabb{{4.0f, -1.0f}, {6.0f, 1.0f}}));
    static_assert(intersects(box, hseg));
    // overlap(aabb,aabb) MTV is pure float -> constexpr.
    static_assert(overlap(aabb{{0.0f, 0.0f}, {2.0f, 2.0f}}, aabb{{1.0f, 0.0f}, {3.0f, 2.0f}})
                      ->depth == 1.0f);
    static_assert(!overlap(aabb{{0.0f, 0.0f}, {2.0f, 2.0f}}, aabb{{5.0f, 5.0f}, {7.0f, 7.0f}}));

    // Shape-to-shape squared_distance that avoids sqrt is constexpr.
    static_assert(squared_distance(aabb{{0.0f, 0.0f}, {2.0f, 2.0f}}, aabb{{5.0f, 0.0f}, {7.0f, 2.0f}}) == 9.0f);
    static_assert(squared_distance(aabb{{0.0f, 0.0f}, {2.0f, 2.0f}}, aabb{{1.0f, 1.0f}, {3.0f, 3.0f}}) == 0.0f);
    static_assert(squared_distance(segment{{5.0f, 0.0f}, {5.0f, 2.0f}}, aabb{{0.0f, 0.0f}, {2.0f, 2.0f}}) == 9.0f);
}

TEST_SUITE("neutrino::physics") {
    // ------------------------------------------------------------------
    // Basic shape helpers
    // ------------------------------------------------------------------
    TEST_CASE("aabb size and center") {
        aabb b{{1.0f, 2.0f}, {5.0f, 8.0f}};
        CHECK(vapprox(b.size(), vec{4.0f, 6.0f}));
        CHECK(vapprox(b.center(), vec{3.0f, 5.0f}));
    }

    TEST_CASE("segment point_in_time") {
        segment s{{0.0f, 0.0f}, {10.0f, 4.0f}};
        CHECK(vapprox(s.point_in_time(0.0f), vec{0.0f, 0.0f}));
        CHECK(vapprox(s.point_in_time(1.0f), vec{10.0f, 4.0f}));
        CHECK(vapprox(s.point_in_time(0.5f), vec{5.0f, 2.0f}));
        // Extrapolation beyond [0,1] is allowed.
        CHECK(vapprox(s.point_in_time(2.0f), vec{20.0f, 8.0f}));
    }

    // ------------------------------------------------------------------
    // Containment
    // ------------------------------------------------------------------
    TEST_CASE("contains(aabb, point)") {
        aabb b{{0.0f, 0.0f}, {4.0f, 4.0f}};
        CHECK(contains(b, vec{2.0f, 2.0f}));   // interior
        CHECK(contains(b, vec{0.0f, 0.0f}));   // corner (boundary)
        CHECK(contains(b, vec{4.0f, 2.0f}));   // edge (boundary)
        CHECK_FALSE(contains(b, vec{4.001f, 2.0f}));
        CHECK_FALSE(contains(b, vec{-0.001f, 2.0f}));
    }

    TEST_CASE("contains(circle, point)") {
        circle c{{0.0f, 0.0f}, 2.0f};
        CHECK(contains(c, vec{0.0f, 0.0f}));   // center
        CHECK(contains(c, vec{2.0f, 0.0f}));   // on boundary
        CHECK(contains(c, vec{1.0f, 1.0f}));   // interior
        CHECK_FALSE(contains(c, vec{2.0f, 2.0f}));
    }

    // ------------------------------------------------------------------
    // Static overlap tests
    // ------------------------------------------------------------------
    TEST_CASE("intersects(aabb, aabb)") {
        aabb a{{0.0f, 0.0f}, {2.0f, 2.0f}};
        CHECK(intersects(a, aabb{{1.0f, 1.0f}, {3.0f, 3.0f}}));   // overlap
        CHECK(intersects(a, aabb{{2.0f, 0.0f}, {4.0f, 2.0f}}));   // edge touch
        CHECK_FALSE(intersects(a, aabb{{2.001f, 0.0f}, {4.0f, 2.0f}}));
        CHECK_FALSE(intersects(a, aabb{{5.0f, 5.0f}, {6.0f, 6.0f}}));
    }

    TEST_CASE("intersects(circle, circle)") {
        circle a{{0.0f, 0.0f}, 1.0f};
        CHECK(intersects(a, circle{{1.0f, 0.0f}, 1.0f}));   // overlap
        CHECK(intersects(a, circle{{2.0f, 0.0f}, 1.0f}));   // exact touch (r1+r2 == d)
        CHECK_FALSE(intersects(a, circle{{2.001f, 0.0f}, 1.0f}));
    }

    TEST_CASE("intersects(circle, aabb) static") {
        circle c{{0.0f, 0.0f}, 1.0f};
        CHECK(intersects(c, aabb{{0.5f, 0.0f}, {3.0f, 3.0f}}));    // center near box
        CHECK(intersects(c, aabb{{1.0f, -1.0f}, {3.0f, 1.0f}}));   // touch left face
        CHECK_FALSE(intersects(c, aabb{{2.0f, 2.0f}, {3.0f, 3.0f}}));   // far corner
    }

    // ------------------------------------------------------------------
    // line_hit interval predicates (methods)
    // ------------------------------------------------------------------
    TEST_CASE("line_hit overlap predicates") {
        CHECK(line_hit{0.2f, 0.8f, {}, {}}.line_overlaps());
        CHECK_FALSE(line_hit{0.8f, 0.2f, {}, {}}.line_overlaps());   // entry > exit

        CHECK(line_hit{0.2f, 0.8f, {}, {}}.segment_overlaps());
        CHECK(line_hit{-0.3f, 0.5f, {}, {}}.segment_overlaps());     // starts inside
        CHECK_FALSE(line_hit{1.2f, 1.5f, {}, {}}.segment_overlaps()); // entry past end
        CHECK_FALSE(line_hit{-0.9f, -0.2f, {}, {}}.segment_overlaps());// exit before start
    }

    // ------------------------------------------------------------------
    // Slab method: intersect_param(aabb, segment)
    // ------------------------------------------------------------------
    TEST_CASE("intersect_param(aabb, segment)") {
        aabb box{{0.0f, 0.0f}, {2.0f, 2.0f}};

        SUBCASE("horizontal crossing through the middle") {
            segment s{{-1.0f, 1.0f}, {3.0f, 1.0f}};   // dx = 4
            auto e = intersect_param(box, s);
            REQUIRE(e.has_value());
            CHECK(e->entry_param == doctest::Approx(0.25f));
            CHECK(e->exit_param == doctest::Approx(0.75f));
            CHECK(vapprox(e->entry_normal, vec{-1.0f, 0.0f}));
            CHECK(vapprox(e->exit_normal, vec{1.0f, 0.0f}));
            CHECK(e->segment_overlaps());
        }

        SUBCASE("vertical segment inside the x-slab (dx == 0)") {
            segment s{{1.0f, -1.0f}, {1.0f, 3.0f}};   // dx = 0, x in [0,2]
            auto e = intersect_param(box, s);
            REQUIRE(e.has_value());
            CHECK(e->entry_param == doctest::Approx(0.25f));
            CHECK(e->exit_param == doctest::Approx(0.75f));
            CHECK(e->segment_overlaps());
        }

        SUBCASE("vertical segment outside the x-slab misses") {
            segment s{{5.0f, -1.0f}, {5.0f, 3.0f}};   // dx = 0, x outside [0,2]
            CHECK_FALSE(intersect_param(box, s).has_value());
        }

        SUBCASE("segment starting inside the box yields entry < 0") {
            segment s{{1.0f, 1.0f}, {5.0f, 1.0f}};
            auto e = intersect_param(box, s);
            REQUIRE(e.has_value());
            CHECK(e->entry_param < 0.0f);
            CHECK(e->exit_param > 0.0f);
            CHECK(e->segment_overlaps());
        }

        SUBCASE("parallel miss above the box") {
            segment s{{-1.0f, 5.0f}, {3.0f, 5.0f}};   // dy = 0, y outside [0,2]
            CHECK_FALSE(intersect_param(box, s).has_value());
        }
    }

    // ------------------------------------------------------------------
    // Closest point / parameter
    // ------------------------------------------------------------------
    TEST_CASE("closest_parameter / closest_point(segment)") {
        segment s{{0.0f, 0.0f}, {10.0f, 0.0f}};
        CHECK(closest_parameter(vec{5.0f, 5.0f}, s) == doctest::Approx(0.5f));
        CHECK(closest_parameter(vec{-5.0f, 0.0f}, s) == doctest::Approx(0.0f));   // clamp low
        CHECK(closest_parameter(vec{15.0f, 0.0f}, s) == doctest::Approx(1.0f));   // clamp high
        CHECK(vapprox(closest_point(vec{5.0f, 5.0f}, s), vec{5.0f, 0.0f}));
        CHECK(vapprox(closest_point(vec{-5.0f, 9.0f}, s), vec{0.0f, 0.0f}));
    }

    TEST_CASE("closest_parameter on a short (non-degenerate) segment") {
        // 0.01 units long — well above the 1e-6 degeneracy threshold; must NOT
        // collapse to parameter 0.
        segment s{{0.0f, 0.0f}, {0.01f, 0.0f}};
        CHECK(closest_parameter(vec{0.005f, 0.0f}, s) == doctest::Approx(0.5f));
    }

    TEST_CASE("closest_parameter on a truly degenerate segment") {
        segment s{{3.0f, 3.0f}, {3.0f, 3.0f}};
        CHECK(closest_parameter(vec{9.0f, 9.0f}, s) == doctest::Approx(0.0f));
    }

    TEST_CASE("closest_point(aabb)") {
        aabb b{{0.0f, 0.0f}, {4.0f, 4.0f}};
        CHECK(vapprox(closest_point(vec{2.0f, 2.0f}, b), vec{2.0f, 2.0f}));   // inside
        CHECK(vapprox(closest_point(vec{-1.0f, 2.0f}, b), vec{0.0f, 2.0f}));  // left
        CHECK(vapprox(closest_point(vec{6.0f, 6.0f}, b), vec{4.0f, 4.0f}));   // corner
    }

    TEST_CASE("closest_point(circle)") {
        circle c{{0.0f, 0.0f}, 2.0f};
        CHECK(vapprox(closest_point(vec{1.0f, 0.0f}, c), vec{1.0f, 0.0f}));   // inside -> itself
        CHECK(vapprox(closest_point(vec{4.0f, 0.0f}, c), vec{2.0f, 0.0f}));   // project to rim
        CHECK(vapprox(closest_point(vec{0.0f, -5.0f}, c), vec{0.0f, -2.0f}));
    }

    // ------------------------------------------------------------------
    // Squared distance
    // ------------------------------------------------------------------
    TEST_CASE("squared_distance(point, segment)") {
        segment s{{0.0f, 0.0f}, {10.0f, 0.0f}};
        CHECK(squared_distance(vec{5.0f, 3.0f}, s) == doctest::Approx(9.0f));    // perpendicular
        CHECK(squared_distance(vec{-5.0f, 0.0f}, s) == doctest::Approx(25.0f));  // before start
        CHECK(squared_distance(vec{15.0f, 0.0f}, s) == doctest::Approx(25.0f));  // past end
        CHECK(squared_distance(vec{5.0f, 0.0f}, s) == doctest::Approx(0.0f));    // on segment
    }

    TEST_CASE("squared_distance(point, segment) is stable for large coordinates") {
        // |ac|^2 - e^2/f loses all precision here; the cross-product form must not.
        segment s{{0.0f, 0.0f}, {100000000.0f, 1.0f}};
        CHECK(squared_distance(vec{50000000.0f, 10.0f}, s) == doctest::Approx(90.25f).epsilon(0.01));
    }

    TEST_CASE("squared_distance(point, aabb)") {
        aabb b{{0.0f, 0.0f}, {4.0f, 4.0f}};
        CHECK(squared_distance(vec{2.0f, 2.0f}, b) == doctest::Approx(0.0f));   // inside
        CHECK(squared_distance(vec{6.0f, 2.0f}, b) == doctest::Approx(4.0f));   // right of edge
        CHECK(squared_distance(vec{6.0f, 6.0f}, b) == doctest::Approx(8.0f));   // off a corner
        CHECK(squared_distance(vec{-3.0f, 2.0f}, b) == doctest::Approx(9.0f));  // left of edge
    }

    TEST_CASE("squared_distance(point, circle)") {
        circle c{{0.0f, 0.0f}, 2.0f};
        CHECK(squared_distance(vec{1.0f, 0.0f}, c) == doctest::Approx(0.0f));   // inside
        CHECK(squared_distance(vec{2.0f, 0.0f}, c) == doctest::Approx(0.0f));   // on rim
        CHECK(squared_distance(vec{5.0f, 0.0f}, c) == doctest::Approx(9.0f));   // (5-2)^2
    }

    // ------------------------------------------------------------------
    // intersect_param(circle, segment)
    // ------------------------------------------------------------------
    TEST_CASE("intersect_param(circle, segment)") {
        circle c{{0.0f, 0.0f}, 1.0f};

        SUBCASE("secant line through the circle") {
            segment s{{-2.0f, 0.0f}, {2.0f, 0.0f}};
            auto r = intersect_param(c, s);
            REQUIRE(r.has_value());
            CHECK(r->entry_param == doctest::Approx(0.25f));
            CHECK(r->exit_param == doctest::Approx(0.75f));
            CHECK(vapprox(r->entry_normal, vec{-1.0f, 0.0f}));
            CHECK(vapprox(r->exit_normal, vec{1.0f, 0.0f}));
            CHECK(r->segment_overlaps());
        }

        SUBCASE("line misses the circle entirely") {
            segment s{{-2.0f, 5.0f}, {2.0f, 5.0f}};
            CHECK_FALSE(intersect_param(c, s).has_value());
        }

        SUBCASE("tangent line yields a single grazing contact") {
            segment s{{-2.0f, 1.0f}, {2.0f, 1.0f}};   // touches top of circle at (0,1)
            auto r = intersect_param(c, s);
            REQUIRE(r.has_value());
            CHECK(r->entry_param == doctest::Approx(0.5f));
            CHECK(r->exit_param == doctest::Approx(0.5f));
            CHECK(vapprox(r->entry_normal, vec{0.0f, 1.0f}));
        }

        SUBCASE("degenerate (point) segment inside the circle") {
            segment s{{0.0f, 0.0f}, {0.0f, 0.0f}};
            auto r = intersect_param(c, s);
            REQUIRE(r.has_value());
            CHECK(r->entry_param == constants::NEG_INF);
            CHECK(r->exit_param == constants::INF);
        }

        SUBCASE("degenerate (point) segment outside the circle") {
            segment s{{5.0f, 5.0f}, {5.0f, 5.0f}};
            CHECK_FALSE(intersect_param(c, s).has_value());
        }

        SUBCASE("far-from-origin miss is not swallowed by discriminant cancellation") {
            // Unit circle at origin; a long horizontal segment at y = 10 misses by 9.
            // The b^2 - 4ac discriminant cancels catastrophically at this scale and used
            // to (wrongly) report a tangent hit at param 0.5.
            CHECK_FALSE(intersect_param(c, segment{{-10000.0f, 10.0f}, {10000.0f, 10.0f}}).has_value());
            // A near-grazing pass just inside the radius must still register.
            auto graze = intersect_param(c, segment{{-10000.0f, 0.99f}, {10000.0f, 0.99f}});
            REQUIRE(graze.has_value());
            CHECK(graze->segment_overlaps());
        }

        SUBCASE("far-from-origin secant reports a correct chord") {
            // Circle of radius 5 centred far from the origin; horizontal secant at the
            // centre height crosses at center.x +/- 5.
            circle big{{100000.0f, 100000.0f}, 5.0f};
            auto r = intersect_param(big, segment{{90000.0f, 100000.0f}, {110000.0f, 100000.0f}});
            REQUIRE(r.has_value());
            // entry at x = 99995 => param (99995-90000)/20000 = 0.49975
            CHECK(r->entry_param == doctest::Approx(0.49975f).epsilon(0.001));
            CHECK(r->exit_param == doctest::Approx(0.50025f).epsilon(0.001));
        }
    }

    // ------------------------------------------------------------------
    // Segment / segment
    // ------------------------------------------------------------------
    TEST_CASE("intersect_param(segment, segment)") {
        SUBCASE("crossing diagonals") {
            segment a{{0.0f, 0.0f}, {2.0f, 2.0f}};
            segment b{{0.0f, 2.0f}, {2.0f, 0.0f}};
            auto r = intersect_param(a, b);
            REQUIRE(r.has_value());
            CHECK(r->entry_param == doctest::Approx(0.5f));
            CHECK(r->exit_param == doctest::Approx(0.5f));
            CHECK(is_unit(r->entry_normal));
            CHECK(intersects(a, b));
        }

        SUBCASE("T-intersection") {
            segment a{{0.0f, 0.0f}, {4.0f, 0.0f}};
            segment b{{2.0f, -1.0f}, {2.0f, 1.0f}};
            auto r = intersect_param(a, b);
            REQUIRE(r.has_value());
            CHECK(r->entry_param == doctest::Approx(0.5f));
            CHECK(intersects(a, b));
        }

        SUBCASE("parallel, non-collinear miss") {
            segment a{{0.0f, 0.0f}, {1.0f, 0.0f}};
            segment b{{2.0f, 1.0f}, {3.0f, 1.0f}};
            CHECK_FALSE(intersect_param(a, b).has_value());
            CHECK_FALSE(intersects(a, b));
        }

        SUBCASE("disjoint, non-parallel miss") {
            segment a{{0.0f, 0.0f}, {1.0f, 0.0f}};
            segment b{{5.0f, -1.0f}, {5.0f, 1.0f}};   // would cross at x=5, off segment a
            CHECK_FALSE(intersect_param(a, b).has_value());
            CHECK_FALSE(intersects(a, b));
        }

        SUBCASE("collinear overlap has a deterministic unit normal") {
            segment a{{0.0f, 0.0f}, {4.0f, 0.0f}};
            segment b{{2.0f, 0.0f}, {6.0f, 0.0f}};   // overlap [2,4]
            auto r = intersect_param(a, b);
            REQUIRE(r.has_value());
            CHECK(r->entry_param == doctest::Approx(0.5f));
            CHECK(r->exit_param == doctest::Approx(1.0f));
            CHECK(is_unit(r->entry_normal));
            CHECK(vapprox(r->entry_normal, r->exit_normal));
            CHECK(intersects(a, b));
        }

        SUBCASE("degenerate segment a is a point lying mid-segment of b") {
            segment a{{0.0f, 0.0f}, {0.0f, 0.0f}};
            segment b{{-1.0f, 0.0f}, {1.0f, 0.0f}};
            auto r = intersect_param(a, b);
            REQUIRE(r.has_value());
            CHECK(r->entry_param == doctest::Approx(0.0f));
            CHECK(intersects(a, b));
        }

        SUBCASE("degenerate segment a is a point off segment b") {
            segment a{{0.0f, 5.0f}, {0.0f, 5.0f}};
            segment b{{-1.0f, 0.0f}, {1.0f, 0.0f}};
            CHECK_FALSE(intersect_param(a, b).has_value());
            CHECK_FALSE(intersects(a, b));
        }

        SUBCASE("point b on segment a reports its parameter along a") {
            segment a{{0.0f, 0.0f}, {4.0f, 0.0f}};
            segment b{{2.0f, 0.0f}, {2.0f, 0.0f}};   // point at the midpoint of a
            auto r = intersect_param(a, b);
            REQUIRE(r.has_value());
            CHECK(r->entry_param == doctest::Approx(0.5f));
            CHECK(r->exit_param == doctest::Approx(0.5f));
        }

        SUBCASE("intersects matches intersect_param.has_value()") {
            segment a{{0.0f, 0.0f}, {2.0f, 2.0f}};
            segment b{{0.0f, 2.0f}, {2.0f, 0.0f}};
            CHECK(intersects(a, b) == intersect_param(a, b).has_value());
            segment c{{10.0f, 10.0f}, {11.0f, 11.0f}};
            CHECK(intersects(a, c) == intersect_param(a, c).has_value());
        }
    }

    // ------------------------------------------------------------------
    // CCD: moving AABB vs moving AABB
    // ------------------------------------------------------------------
    TEST_CASE("swept_intersection(aabb, aabb)") {
        SUBCASE("head-on approach collides partway through") {
            aabb a{{0.0f, 0.0f}, {2.0f, 2.0f}};
            aabb b{{5.0f, 0.0f}, {7.0f, 2.0f}};
            auto r = swept_intersection(a, vec{10.0f, 0.0f}, b, vec{0.0f, 0.0f}, 1.0f);
            REQUIRE(r.has_value());
            CHECK(r->entry_time == doctest::Approx(0.3f));
            CHECK(r->exit_time == doctest::Approx(0.7f));
            CHECK(r->entry_time >= 0.0f);
            CHECK(r->exit_time <= 1.0f);
        }

        SUBCASE("moving apart never collides") {
            aabb a{{0.0f, 0.0f}, {2.0f, 2.0f}};
            aabb b{{5.0f, 0.0f}, {7.0f, 2.0f}};
            CHECK_FALSE(swept_intersection(a, vec{-10.0f, 0.0f}, b, vec{0.0f, 0.0f}, 1.0f).has_value());
        }

        SUBCASE("zero-duration sweep of overlapping boxes does not produce NaN") {
            aabb a{{0.0f, 0.0f}, {2.0f, 2.0f}};
            aabb b{{1.0f, 1.0f}, {3.0f, 3.0f}};
            auto r = swept_intersection(a, vec{0.0f, 0.0f}, b, vec{0.0f, 0.0f}, 0.0f);
            REQUIRE(r.has_value());
            CHECK(finite(*r));
            CHECK(r->entry_time == doctest::Approx(0.0f));
            CHECK(r->exit_time == doctest::Approx(0.0f));
        }

        SUBCASE("already-overlapping boxes with zero relative velocity") {
            aabb a{{0.0f, 0.0f}, {2.0f, 2.0f}};
            aabb b{{1.0f, 1.0f}, {3.0f, 3.0f}};
            auto r = swept_intersection(a, vec{0.0f, 0.0f}, b, vec{0.0f, 0.0f}, 0.5f);
            REQUIRE(r.has_value());
            CHECK(finite(*r));
            CHECK(r->entry_time == doctest::Approx(0.0f));
            CHECK(r->exit_time <= 0.5f);
        }
    }

    // ------------------------------------------------------------------
    // CCD: moving circle vs moving circle
    // ------------------------------------------------------------------
    TEST_CASE("swept_intersection(circle, circle)") {
        SUBCASE("head-on approach collides partway through") {
            circle a{{0.0f, 0.0f}, 1.0f};
            circle b{{5.0f, 0.0f}, 1.0f};
            auto r = swept_intersection(a, vec{10.0f, 0.0f}, b, vec{0.0f, 0.0f}, 1.0f);
            REQUIRE(r.has_value());
            CHECK(r->entry_time == doctest::Approx(0.3f));
            CHECK(r->exit_time == doctest::Approx(0.7f));
        }

        SUBCASE("passing wide misses") {
            circle a{{0.0f, 0.0f}, 1.0f};
            circle b{{5.0f, 10.0f}, 1.0f};
            CHECK_FALSE(swept_intersection(a, vec{10.0f, 0.0f}, b, vec{0.0f, 0.0f}, 1.0f).has_value());
        }

        SUBCASE("resting overlap with zero relative velocity does not produce NaN") {
            circle a{{0.0f, 0.0f}, 1.0f};
            circle b{{0.5f, 0.0f}, 1.0f};   // already overlapping
            auto r = swept_intersection(a, vec{0.0f, 0.0f}, b, vec{0.0f, 0.0f}, 0.016f);
            REQUIRE(r.has_value());
            CHECK(finite(*r));
            CHECK(r->entry_time == doctest::Approx(0.0f));
            CHECK(r->exit_time <= 0.016f);
        }

        SUBCASE("zero-duration overlap does not produce NaN") {
            circle a{{0.0f, 0.0f}, 1.0f};
            circle b{{0.5f, 0.0f}, 1.0f};
            auto r = swept_intersection(a, vec{0.0f, 0.0f}, b, vec{0.0f, 0.0f}, 0.0f);
            REQUIRE(r.has_value());
            CHECK(finite(*r));
            CHECK(r->entry_time == doctest::Approx(0.0f));
            CHECK(r->exit_time == doctest::Approx(0.0f));
        }
    }

    // ------------------------------------------------------------------
    // CCD: moving circle vs moving AABB
    // ------------------------------------------------------------------
    TEST_CASE("swept_intersection(circle, aabb)") {
        SUBCASE("axis-aligned approach hits the left face") {
            circle c{{0.0f, 0.0f}, 1.0f};
            aabb b{{5.0f, -1.0f}, {7.0f, 1.0f}};
            auto r = swept_intersection(c, vec{10.0f, 0.0f}, b, vec{0.0f, 0.0f}, 1.0f);
            REQUIRE(r.has_value());
            CHECK(r->entry_time == doctest::Approx(0.4f));   // center reaches x = 4 (min.x - R)
            CHECK(vapprox(r->entry_normal, vec{-1.0f, 0.0f}));
            CHECK(r->entry_time >= 0.0f);
            CHECK(r->exit_time <= 1.0f);
            CHECK(r->exit_time >= r->entry_time);
            CHECK(finite(*r));
        }

        SUBCASE("moving away misses") {
            circle c{{0.0f, 0.0f}, 1.0f};
            aabb b{{5.0f, -1.0f}, {7.0f, 1.0f}};
            CHECK_FALSE(swept_intersection(c, vec{-10.0f, 0.0f}, b, vec{0.0f, 0.0f}, 1.0f).has_value());
        }

        SUBCASE("diagonal approach toward a corner collides") {
            circle c{{0.0f, 0.0f}, 1.0f};
            aabb b{{5.0f, 5.0f}, {7.0f, 7.0f}};
            auto r = swept_intersection(c, vec{10.0f, 10.0f}, b, vec{0.0f, 0.0f}, 1.0f);
            REQUIRE(r.has_value());
            CHECK(finite(*r));
            CHECK(r->entry_time >= 0.0f);
            CHECK(r->exit_time <= 1.0f);
            CHECK(r->exit_time >= r->entry_time);
            CHECK(is_unit(r->entry_normal));   // corner-circle contact -> radial unit normal
        }

        SUBCASE("zero-duration overlap does not produce NaN") {
            circle c{{0.0f, 0.0f}, 1.0f};
            aabb b{{-0.5f, -0.5f}, {0.5f, 0.5f}};   // box overlaps the circle center
            auto r = swept_intersection(c, vec{0.0f, 0.0f}, b, vec{0.0f, 0.0f}, 0.0f);
            REQUIRE(r.has_value());
            CHECK(finite(*r));
            CHECK(r->entry_time == doctest::Approx(0.0f));
            CHECK(r->exit_time == doctest::Approx(0.0f));
        }
    }

    // ------------------------------------------------------------------
    // Slab method: axis-parallel boundary cases (the original slab bug)
    // ------------------------------------------------------------------
    TEST_CASE("intersect_param(aabb, segment) boundary-parallel cases") {
        aabb box{{0.0f, 0.0f}, {2.0f, 2.0f}};

        SUBCASE("vertical segment exactly on x == min grazes the edge") {
            auto e = intersect_param(box, segment{{0.0f, -1.0f}, {0.0f, 3.0f}});
            REQUIRE(e.has_value());
            CHECK(e->segment_overlaps());
            CHECK(e->entry_param == doctest::Approx(0.25f));
            CHECK(e->exit_param == doctest::Approx(0.75f));
        }

        SUBCASE("vertical segment exactly on x == max grazes the edge") {
            auto e = intersect_param(box, segment{{2.0f, -1.0f}, {2.0f, 3.0f}});
            REQUIRE(e.has_value());
            CHECK(e->segment_overlaps());
            CHECK(e->entry_param == doctest::Approx(0.25f));
            CHECK(e->exit_param == doctest::Approx(0.75f));
        }

        SUBCASE("horizontal segment exactly on y == min grazes the edge") {
            auto e = intersect_param(box, segment{{-1.0f, 0.0f}, {3.0f, 0.0f}});
            REQUIRE(e.has_value());
            CHECK(e->segment_overlaps());
            CHECK(e->entry_param == doctest::Approx(0.25f));
            CHECK(e->exit_param == doctest::Approx(0.75f));
        }

        SUBCASE("horizontal segment exactly on y == max grazes the edge") {
            auto e = intersect_param(box, segment{{-1.0f, 2.0f}, {3.0f, 2.0f}});
            REQUIRE(e.has_value());
            CHECK(e->segment_overlaps());
            CHECK(e->entry_param == doctest::Approx(0.25f));
            CHECK(e->exit_param == doctest::Approx(0.75f));
        }

        SUBCASE("degenerate point segment inside the box") {
            auto e = intersect_param(box, segment{{1.0f, 1.0f}, {1.0f, 1.0f}});
            REQUIRE(e.has_value());
            CHECK(e->segment_overlaps());
            CHECK(e->entry_param == constants::NEG_INF);
            CHECK(e->exit_param == constants::INF);
        }

        SUBCASE("degenerate point segment on the boundary") {
            auto e = intersect_param(box, segment{{0.0f, 1.0f}, {0.0f, 1.0f}});
            REQUIRE(e.has_value());
            CHECK(e->segment_overlaps());
        }

        SUBCASE("degenerate point segment outside the box misses") {
            CHECK_FALSE(intersect_param(box, segment{{5.0f, 5.0f}, {5.0f, 5.0f}}).has_value());
        }
    }

    // ------------------------------------------------------------------
    // intersect_param(circle, segment): infinite line hits, finite segment does not
    // ------------------------------------------------------------------
    TEST_CASE("intersect_param(circle, segment) finite-segment miss with a line hit") {
        circle c{{0.0f, 0.0f}, 1.0f};
        // The x-axis crosses the circle at x = +/-1, but this finite segment lives
        // entirely at x in [-10, -5]: the line hits, the segment does not.
        auto r = intersect_param(c, segment{{-10.0f, 0.0f}, {-5.0f, 0.0f}});
        REQUIRE(r.has_value());          // infinite line intersects
        CHECK(r->entry_param > 1.0f);    // both crossings lie beyond the segment
        CHECK(r->exit_param > 1.0f);
        CHECK_FALSE(r->segment_overlaps());   // finite segment does not reach the circle
    }

    // ------------------------------------------------------------------
    // Zero-radius circle (a point): a common degenerate that must not produce NaN
    // ------------------------------------------------------------------
    TEST_CASE("zero-radius circle") {
        circle z{{0.0f, 0.0f}, 0.0f};

        SUBCASE("contains only the exact center") {
            CHECK(contains(z, vec{0.0f, 0.0f}));
            CHECK_FALSE(contains(z, vec{0.001f, 0.0f}));
        }

        SUBCASE("closest_point collapses to the center") {
            CHECK(vapprox(closest_point(vec{3.0f, 4.0f}, z), vec{0.0f, 0.0f}));
            CHECK(vapprox(closest_point(vec{0.0f, 0.0f}, z), vec{0.0f, 0.0f}));
        }

        SUBCASE("squared_distance is the distance to the point") {
            CHECK(squared_distance(vec{3.0f, 4.0f}, z) == doctest::Approx(25.0f));
            CHECK(squared_distance(vec{0.0f, 0.0f}, z) == doctest::Approx(0.0f));
        }

        SUBCASE("segment through the point is a single finite contact, no NaN") {
            auto r = intersect_param(z, segment{{-2.0f, 0.0f}, {2.0f, 0.0f}});
            REQUIRE(r.has_value());
            CHECK(finite(*r));
            CHECK(r->entry_param == doctest::Approx(0.5f));
            CHECK(r->exit_param == doctest::Approx(0.5f));
            CHECK(r->segment_overlaps());
        }

        SUBCASE("segment missing the point returns nullopt") {
            CHECK_FALSE(intersect_param(z, segment{{-2.0f, 1.0f}, {2.0f, 1.0f}}).has_value());
        }
    }

    // ------------------------------------------------------------------
    // Segment / segment: the full collinear / degenerate matrix
    // ------------------------------------------------------------------
    TEST_CASE("intersect_param(segment, segment) collinear and degenerate matrix") {
        segment a{{0.0f, 0.0f}, {4.0f, 0.0f}};

        SUBCASE("collinear but disjoint") {
            CHECK_FALSE(intersect_param(a, segment{{6.0f, 0.0f}, {8.0f, 0.0f}}).has_value());
            CHECK_FALSE(intersects(a, segment{{6.0f, 0.0f}, {8.0f, 0.0f}}));
        }

        SUBCASE("collinear endpoint-only touch") {
            auto r = intersect_param(a, segment{{4.0f, 0.0f}, {8.0f, 0.0f}});
            REQUIRE(r.has_value());
            CHECK(r->entry_param == doctest::Approx(1.0f));
            CHECK(r->exit_param == doctest::Approx(1.0f));
        }

        SUBCASE("identical segments fully overlap") {
            auto r = intersect_param(a, segment{{0.0f, 0.0f}, {4.0f, 0.0f}});
            REQUIRE(r.has_value());
            CHECK(r->entry_param == doctest::Approx(0.0f));
            CHECK(r->exit_param == doctest::Approx(1.0f));
        }

        SUBCASE("reversed segment overlapping in the middle") {
            // b runs from x=6 back to x=2, overlapping a on [2,4] => params [0.5,1.0] of a.
            auto r = intersect_param(a, segment{{6.0f, 0.0f}, {2.0f, 0.0f}});
            REQUIRE(r.has_value());
            CHECK(r->entry_param == doctest::Approx(0.5f));
            CHECK(r->exit_param == doctest::Approx(1.0f));
        }

        SUBCASE("both degenerate, coincident") {
            auto r = intersect_param(segment{{1.0f, 1.0f}, {1.0f, 1.0f}},
                                     segment{{1.0f, 1.0f}, {1.0f, 1.0f}});
            REQUIRE(r.has_value());
            CHECK(r->entry_param == doctest::Approx(0.0f));
            CHECK(r->exit_param == doctest::Approx(0.0f));
        }

        SUBCASE("both degenerate, distinct") {
            CHECK_FALSE(intersect_param(segment{{1.0f, 1.0f}, {1.0f, 1.0f}},
                                        segment{{2.0f, 2.0f}, {2.0f, 2.0f}}).has_value());
        }
    }

    // ------------------------------------------------------------------
    // CCD boundary timing
    // ------------------------------------------------------------------
    TEST_CASE("CCD boundary timing (aabb)") {
        aabb a{{0.0f, 0.0f}, {2.0f, 2.0f}};
        aabb b{{5.0f, 0.0f}, {7.0f, 2.0f}};   // 3-unit gap to a's right edge

        SUBCASE("collision exactly at time == time is reported") {
            // relative displacement v*T = 3 just reaches contact at the final instant.
            auto r = swept_intersection(a, vec{3.0f, 0.0f}, b, vec{0.0f, 0.0f}, 1.0f);
            REQUIRE(r.has_value());
            CHECK(r->entry_time == doctest::Approx(1.0f));   // entry == time
        }

        SUBCASE("contact just after the window is a miss") {
            CHECK_FALSE(swept_intersection(a, vec{3.0f, 0.0f}, b, vec{0.0f, 0.0f}, 0.9f).has_value());
        }

        SUBCASE("zero-duration query on non-overlapping boxes returns nullopt") {
            CHECK_FALSE(swept_intersection(a, vec{3.0f, 0.0f}, b, vec{0.0f, 0.0f}, 0.0f).has_value());
        }
    }

    // ------------------------------------------------------------------
    // CCD depends only on relative velocity (both objects moving)
    // ------------------------------------------------------------------
    TEST_CASE("CCD with both objects moving") {
        SUBCASE("aabb: only relative velocity matters") {
            aabb a{{0.0f, 0.0f}, {2.0f, 2.0f}};
            aabb b{{5.0f, 0.0f}, {7.0f, 2.0f}};
            // a moves +5, b moves -5 => relative speed 10, same as the head-on case.
            auto r = swept_intersection(a, vec{5.0f, 0.0f}, b, vec{-5.0f, 0.0f}, 1.0f);
            REQUIRE(r.has_value());
            CHECK(r->entry_time == doctest::Approx(0.3f));
            CHECK(r->exit_time == doctest::Approx(0.7f));
        }

        SUBCASE("circle: only relative velocity matters") {
            circle a{{0.0f, 0.0f}, 1.0f};
            circle b{{5.0f, 0.0f}, 1.0f};
            auto r = swept_intersection(a, vec{5.0f, 0.0f}, b, vec{-5.0f, 0.0f}, 1.0f);
            REQUIRE(r.has_value());
            CHECK(r->entry_time == doctest::Approx(0.3f));
            CHECK(r->exit_time == doctest::Approx(0.7f));
        }
    }

    // ------------------------------------------------------------------
    // CCD circle/AABB: convex exit selection (numeric exit + exit normal)
    // ------------------------------------------------------------------
    TEST_CASE("swept_intersection(circle, aabb) pass-through pins exit time and normal") {
        circle c{{0.0f, 0.0f}, 1.0f};
        aabb b{{5.0f, -1.0f}, {7.0f, 1.0f}};
        // Center sweeps along y = 0 from x=0 to x=20. The rounded rectangle spans
        // x in [4, 8] on the center line, so entry/exit are exact and the exit comes
        // from the latest-exiting sub-shape (the horizontal expanded box).
        auto r = swept_intersection(c, vec{20.0f, 0.0f}, b, vec{0.0f, 0.0f}, 1.0f);
        REQUIRE(r.has_value());
        CHECK(r->entry_time == doctest::Approx(0.2f));          // x = 4 at t = 0.2
        CHECK(r->exit_time == doctest::Approx(0.4f));           // x = 8 at t = 0.4
        CHECK(vapprox(r->entry_normal, vec{-1.0f, 0.0f}));
        CHECK(vapprox(r->exit_normal, vec{1.0f, 0.0f}));
    }

    // ------------------------------------------------------------------
    // CCD: moving circle / AABB vs a segment (capsule / feature decomposition)
    // ------------------------------------------------------------------
    TEST_CASE("swept_intersection(circle, segment)") {
        segment hx{{-5.0f, 0.0f}, {5.0f, 0.0f}}; // wall along x at y=0

        SUBCASE("approach the flat side from above") {
            auto r = swept_intersection(circle{{0.0f, 5.0f}, 1.0f}, vec{0.0f, -10.0f}, hx, vec{0.0f, 0.0f}, 1.0f);
            REQUIRE(r.has_value());
            CHECK(r->entry_time == doctest::Approx(0.4f)); // centre reaches y=1
            CHECK(r->exit_time == doctest::Approx(0.6f));  // centre reaches y=-1
            CHECK(vapprox(r->entry_normal, vec{0.0f, 1.0f}));
        }
        SUBCASE("miss a short segment") {
            CHECK_FALSE(swept_intersection(circle{{0.0f, 5.0f}, 1.0f}, vec{0.0f, -10.0f},
                                           segment{{-5.0f, 0.0f}, {-3.0f, 0.0f}}, vec{0.0f, 0.0f}, 1.0f)
                            .has_value());
        }
        SUBCASE("graze an endpoint cap") {
            auto r = swept_intersection(circle{{5.0f, 2.0f}, 1.0f}, vec{0.0f, -10.0f}, hx, vec{0.0f, 0.0f}, 1.0f);
            REQUIRE(r.has_value());
            CHECK(r->entry_time == doctest::Approx(0.1f));
            CHECK(is_unit(r->entry_normal));
        }
        SUBCASE("already overlapping -> entry clamped to 0") {
            auto r = swept_intersection(circle{{0.0f, 0.5f}, 1.0f}, vec{0.0f, -10.0f}, hx, vec{0.0f, 0.0f}, 1.0f);
            REQUIRE(r.has_value());
            CHECK(r->entry_time == doctest::Approx(0.0f));
        }
        SUBCASE("only relative velocity matters") {
            auto r = swept_intersection(circle{{0.0f, 5.0f}, 1.0f}, vec{0.0f, -5.0f}, hx, vec{0.0f, 5.0f}, 1.0f);
            REQUIRE(r.has_value());
            CHECK(r->entry_time == doctest::Approx(0.4f)); // rel velocity (0,-10), same as static wall
        }
        SUBCASE("degenerate segment behaves like a circle") {
            segment pt{{0.0f, 0.0f}, {0.0f, 0.0f}};
            auto r = swept_intersection(circle{{0.0f, 5.0f}, 1.0f}, vec{0.0f, -10.0f}, pt, vec{0.0f, 0.0f}, 1.0f);
            REQUIRE(r.has_value());
            CHECK(r->entry_time == doctest::Approx(0.4f));
        }
    }

    TEST_CASE("swept_intersection(aabb, segment)") {
        aabb bx{{0.0f, 0.0f}, {2.0f, 2.0f}};
        segment wall{{5.0f, 0.0f}, {5.0f, 2.0f}}; // vertical wall at x=5

        SUBCASE("box drives into a wall") {
            auto r = swept_intersection(bx, vec{10.0f, 0.0f}, wall, vec{0.0f, 0.0f}, 1.0f);
            REQUIRE(r.has_value());
            CHECK(r->entry_time == doctest::Approx(0.3f)); // right edge x=2 reaches x=5
            CHECK(r->exit_time == doctest::Approx(0.5f));  // left edge x=0 reaches x=5
            CHECK(vapprox(r->entry_normal, vec{-1.0f, 0.0f})); // push the box back -x
        }
        SUBCASE("miss (wall below the box's path)") {
            CHECK_FALSE(swept_intersection(bx, vec{10.0f, 0.0f}, segment{{5.0f, -5.0f}, {5.0f, -3.0f}},
                                           vec{0.0f, 0.0f}, 1.0f)
                            .has_value());
        }
        SUBCASE("already overlapping -> entry clamped to 0") {
            auto r = swept_intersection(bx, vec{10.0f, 0.0f}, segment{{1.0f, 0.0f}, {1.0f, 2.0f}}, vec{0.0f, 0.0f}, 1.0f);
            REQUIRE(r.has_value());
            CHECK(r->entry_time == doctest::Approx(0.0f));
        }
        SUBCASE("diagonal approach") {
            auto r = swept_intersection(bx, vec{10.0f, 10.0f}, segment{{5.0f, 0.0f}, {5.0f, 8.0f}}, vec{0.0f, 0.0f}, 1.0f);
            REQUIRE(r.has_value());
            CHECK(r->entry_time == doctest::Approx(0.3f)); // right edge reaches x=5 at t=0.3
        }
        SUBCASE("segment crosses the box interior at t=0 (no vertex event)") {
            // Both endpoints outside, no corner on the segment: overlap exists at t=0.
            segment thru{{-1.0f, 1.0f}, {3.0f, 1.0f}};
            // Zero velocity: overlap holds for the whole window.
            auto stat = swept_intersection(bx, vec{0.0f, 0.0f}, thru, vec{0.0f, 0.0f}, 1.0f);
            REQUIRE(stat.has_value());
            CHECK(stat->entry_time == doctest::Approx(0.0f));
            CHECK(stat->exit_time == doctest::Approx(1.0f));
            // Moving up: already overlapping at t=0, separates when the bottom edge passes y=1.
            auto up = swept_intersection(bx, vec{0.0f, 10.0f}, thru, vec{0.0f, 0.0f}, 1.0f);
            REQUIRE(up.has_value());
            CHECK(up->entry_time == doctest::Approx(0.0f)); // NOT 0.1 — overlapping at t=0
            CHECK(up->exit_time == doctest::Approx(0.1f));
        }
    }

    // ------------------------------------------------------------------
    // Static circle/AABB: exact corner tangent (3-4-5)
    // ------------------------------------------------------------------
    TEST_CASE("intersects(circle, aabb) exact corner tangent") {
        // Circle at origin radius 5; AABB nearest corner at (3,4) is exactly 5 away.
        CHECK(intersects(circle{{0.0f, 0.0f}, 5.0f}, aabb{{3.0f, 4.0f}, {10.0f, 10.0f}}));
        // Nudge the corner out to (4,4): distance sqrt(32) > 5 => no touch.
        CHECK_FALSE(intersects(circle{{0.0f, 0.0f}, 5.0f}, aabb{{4.0f, 4.0f}, {10.0f, 10.0f}}));
    }

    // ------------------------------------------------------------------
    // Remaining static-overlap matrix entries (point/segment, segment/circle, segment/aabb)
    // ------------------------------------------------------------------
    TEST_CASE("contains(segment, point)") {
        segment s{{0.0f, 0.0f}, {10.0f, 0.0f}};
        CHECK(contains(s, vec{5.0f, 0.0f}));       // on the segment
        CHECK(contains(s, vec{0.0f, 0.0f}));       // endpoint
        CHECK_FALSE(contains(s, vec{5.0f, 0.5f})); // off the line
        CHECK_FALSE(contains(s, vec{15.0f, 0.0f})); // on the line but past the end
    }

    TEST_CASE("intersects(segment, circle)") {
        circle c{{0.0f, 0.0f}, 1.0f};
        CHECK(intersects(segment{{-5.0f, 0.0f}, {5.0f, 0.0f}}, c));      // through centre
        CHECK(intersects(c, segment{{-5.0f, 0.0f}, {5.0f, 0.0f}}));      // symmetric
        CHECK(intersects(segment{{-5.0f, 0.9f}, {5.0f, 0.9f}}, c));      // chord
        CHECK(intersects(segment{{0.0f, 1.0f}, {2.0f, 1.0f}}, c));       // tangent (touch)
        CHECK_FALSE(intersects(segment{{-5.0f, 5.0f}, {5.0f, 5.0f}}, c));// far away
        CHECK_FALSE(intersects(segment{{3.0f, 0.0f}, {5.0f, 0.0f}}, c)); // collinear but past the rim
    }

    TEST_CASE("intersects(segment, aabb)") {
        aabb b{{0.0f, 0.0f}, {2.0f, 2.0f}};
        CHECK(intersects(segment{{-1.0f, 1.0f}, {3.0f, 1.0f}}, b));      // crossing
        CHECK(intersects(b, segment{{-1.0f, 1.0f}, {3.0f, 1.0f}}));      // symmetric
        CHECK(intersects(segment{{0.5f, 0.5f}, {1.5f, 1.5f}}, b));       // wholly inside
        CHECK_FALSE(intersects(segment{{-1.0f, 5.0f}, {3.0f, 5.0f}}, b));// passes above
        CHECK_FALSE(intersects(segment{{3.0f, 0.0f}, {5.0f, 2.0f}}, b)); // to the right
    }

    // ------------------------------------------------------------------
    // Penetration / MTV (overlap)
    // ------------------------------------------------------------------
    TEST_CASE("overlap(aabb, aabb) MTV") {
        SUBCASE("least-overlap axis and depth") {
            // a overlaps b on x by 1, on y fully (2); MTV is the smaller (x), pushing a in -x.
            auto p = overlap(aabb{{0.0f, 0.0f}, {2.0f, 2.0f}}, aabb{{1.0f, 0.0f}, {3.0f, 2.0f}});
            REQUIRE(p.has_value());
            CHECK(vapprox(p->normal, vec{-1.0f, 0.0f}));
            CHECK(p->depth == doctest::Approx(1.0f));
        }
        SUBCASE("vertical least-overlap") {
            auto p = overlap(aabb{{0.0f, 0.0f}, {4.0f, 4.0f}}, aabb{{0.0f, 3.0f}, {4.0f, 7.0f}});
            REQUIRE(p.has_value());
            CHECK(vapprox(p->normal, vec{0.0f, -1.0f}));
            CHECK(p->depth == doctest::Approx(1.0f));
        }
        SUBCASE("touching is not overlap") {
            CHECK_FALSE(overlap(aabb{{0.0f, 0.0f}, {2.0f, 2.0f}}, aabb{{2.0f, 0.0f}, {4.0f, 2.0f}}).has_value());
        }
        SUBCASE("disjoint") {
            CHECK_FALSE(overlap(aabb{{0.0f, 0.0f}, {2.0f, 2.0f}}, aabb{{5.0f, 5.0f}, {7.0f, 7.0f}}).has_value());
        }
        SUBCASE("MTV separates the boxes") {
            aabb a{{0.0f, 0.0f}, {2.0f, 2.0f}};
            aabb b{{1.0f, 0.0f}, {3.0f, 2.0f}};
            auto p = overlap(a, b);
            REQUIRE(p.has_value());
            aabb moved{a.min + p->normal * p->depth, a.max + p->normal * p->depth};
            CHECK_FALSE(overlap(moved, b).has_value()); // now separated
        }
    }

    TEST_CASE("overlap(circle, circle) MTV") {
        SUBCASE("along the centre line") {
            auto p = overlap(circle{{0.0f, 0.0f}, 1.0f}, circle{{1.0f, 0.0f}, 1.0f});
            REQUIRE(p.has_value());
            CHECK(vapprox(p->normal, vec{-1.0f, 0.0f}));   // push a away from b (+x)
            CHECK(p->depth == doctest::Approx(1.0f));      // (1+1) - 1
        }
        SUBCASE("touching / disjoint give no MTV") {
            CHECK_FALSE(overlap(circle{{0.0f, 0.0f}, 1.0f}, circle{{2.0f, 0.0f}, 1.0f}).has_value());
            CHECK_FALSE(overlap(circle{{0.0f, 0.0f}, 1.0f}, circle{{5.0f, 0.0f}, 1.0f}).has_value());
        }
        SUBCASE("concentric gets a finite arbitrary axis") {
            auto p = overlap(circle{{0.0f, 0.0f}, 1.0f}, circle{{0.0f, 0.0f}, 2.0f});
            REQUIRE(p.has_value());
            CHECK(is_unit(p->normal));
            CHECK(p->depth == doctest::Approx(3.0f));
        }
    }

    TEST_CASE("overlap(circle, aabb) MTV") {
        aabb b{{0.0f, 0.0f}, {2.0f, 2.0f}};
        SUBCASE("centre outside: push along surface normal") {
            auto p = overlap(circle{{2.5f, 1.0f}, 1.0f}, b);
            REQUIRE(p.has_value());
            CHECK(vapprox(p->normal, vec{1.0f, 0.0f}));
            CHECK(p->depth == doctest::Approx(0.5f));      // r - dist = 1 - 0.5
        }
        SUBCASE("centre inside: exit nearest face") {
            auto p = overlap(circle{{1.5f, 1.0f}, 1.0f}, b);
            REQUIRE(p.has_value());
            CHECK(vapprox(p->normal, vec{1.0f, 0.0f}));    // nearest face is +x (0.5 away)
            CHECK(p->depth == doctest::Approx(1.5f));      // r + dist-to-face = 1 + 0.5
        }
        SUBCASE("symmetric overload negates the normal") {
            auto p = overlap(b, circle{{2.5f, 1.0f}, 1.0f});
            REQUIRE(p.has_value());
            CHECK(vapprox(p->normal, vec{-1.0f, 0.0f}));
            CHECK(p->depth == doctest::Approx(0.5f));
        }
        SUBCASE("disjoint") {
            CHECK_FALSE(overlap(circle{{5.0f, 5.0f}, 1.0f}, b).has_value());
        }
        SUBCASE("centre just outside within normalize-epsilon does not falsely overlap") {
            // Regression: a zero-radius centre 5e-7 outside the left face must NOT report a
            // hit (previously fell into the nearest-face branch with a negative depth).
            CHECK_FALSE(overlap(circle{{-5e-7f, 1.0f}, 0.0f}, b).has_value());
            // Tiny radius that still doesn't reach the surface: also no overlap.
            CHECK_FALSE(overlap(circle{{-1e-3f, 1.0f}, 1e-4f}, b).has_value());
        }
        SUBCASE("returned depth is never negative") {
            const circle cs[] = {{{2.5f, 1.0f}, 1.0f}, {{1.5f, 1.0f}, 1.0f}, {{0.0f, 1.0f}, 1.0f},
                                  {{-0.5f, 1.0f}, 1.0f}, {{1.0f, 1.0f}, 3.0f}};
            for (const auto& cc : cs) {
                if (auto p = overlap(cc, b)) {
                    CHECK(p->depth >= 0.0f);
                    CHECK(is_unit(p->normal));
                }
            }
        }
    }

    // ------------------------------------------------------------------
    // Shape-to-shape squared_distance (filled regions; 0 when overlapping)
    // ------------------------------------------------------------------
    TEST_CASE("squared_distance(aabb, aabb)") {
        aabb a{{0.0f, 0.0f}, {2.0f, 2.0f}};
        CHECK(squared_distance(a, aabb{{5.0f, 0.0f}, {7.0f, 2.0f}}) == doctest::Approx(9.0f));  // x-gap 3
        CHECK(squared_distance(a, aabb{{5.0f, 5.0f}, {7.0f, 7.0f}}) == doctest::Approx(18.0f)); // diagonal
        CHECK(squared_distance(a, aabb{{1.0f, 1.0f}, {3.0f, 3.0f}}) == doctest::Approx(0.0f));  // overlap
        CHECK(squared_distance(a, aabb{{2.0f, 0.0f}, {4.0f, 2.0f}}) == doctest::Approx(0.0f));  // touch
    }

    TEST_CASE("squared_distance(circle, circle)") {
        circle a{{0.0f, 0.0f}, 1.0f};
        CHECK(squared_distance(a, circle{{5.0f, 0.0f}, 1.0f}) == doctest::Approx(9.0f)); // gap 5-1-1=3
        CHECK(squared_distance(a, circle{{1.0f, 0.0f}, 1.0f}) == doctest::Approx(0.0f)); // overlap
        CHECK(squared_distance(a, circle{{2.0f, 0.0f}, 1.0f}) == doctest::Approx(0.0f)); // touch
    }

    TEST_CASE("squared_distance(circle, aabb) and (circle, segment)") {
        aabb b{{0.0f, 0.0f}, {2.0f, 2.0f}};
        CHECK(squared_distance(circle{{5.0f, 1.0f}, 1.0f}, b) == doctest::Approx(4.0f)); // 3-1=2 -> 4
        CHECK(squared_distance(b, circle{{5.0f, 1.0f}, 1.0f}) == doctest::Approx(4.0f)); // symmetric
        CHECK(squared_distance(circle{{2.5f, 1.0f}, 1.0f}, b) == doctest::Approx(0.0f)); // overlap

        segment s{{-5.0f, 0.0f}, {5.0f, 0.0f}};
        CHECK(squared_distance(circle{{0.0f, 5.0f}, 1.0f}, s) == doctest::Approx(16.0f)); // 5-1=4 -> 16
        CHECK(squared_distance(s, circle{{0.0f, 5.0f}, 1.0f}) == doctest::Approx(16.0f)); // symmetric
        CHECK(squared_distance(circle{{0.0f, 0.5f}, 1.0f}, s) == doctest::Approx(0.0f));  // overlap
    }

    TEST_CASE("squared_distance(segment, segment)") {
        CHECK(squared_distance(segment{{0.0f, 0.0f}, {2.0f, 2.0f}},
                               segment{{0.0f, 2.0f}, {2.0f, 0.0f}}) == doctest::Approx(0.0f)); // cross
        CHECK(squared_distance(segment{{0.0f, 0.0f}, {4.0f, 0.0f}},
                               segment{{0.0f, 2.0f}, {4.0f, 2.0f}}) == doctest::Approx(4.0f)); // parallel gap 2
        CHECK(squared_distance(segment{{0.0f, 0.0f}, {2.0f, 0.0f}},
                               segment{{5.0f, 0.0f}, {7.0f, 0.0f}}) == doctest::Approx(9.0f)); // collinear gap 3
    }

    TEST_CASE("squared_distance(segment, aabb)") {
        aabb b{{0.0f, 0.0f}, {2.0f, 2.0f}};
        CHECK(squared_distance(segment{{5.0f, 0.0f}, {5.0f, 2.0f}}, b) == doctest::Approx(9.0f)); // right of box
        CHECK(squared_distance(b, segment{{5.0f, 0.0f}, {5.0f, 2.0f}}) == doctest::Approx(9.0f)); // symmetric
        CHECK(squared_distance(segment{{-1.0f, 1.0f}, {3.0f, 1.0f}}, b) == doctest::Approx(0.0f)); // crosses
        // Witness is a box corner vs the segment interior (not a segment endpoint) -> needs the
        // corner candidates, not just endpoint-vs-box.
        CHECK(squared_distance(segment{{3.0f, -1.0f}, {3.0f, 5.0f}}, b) == doctest::Approx(1.0f));
    }

    // ------------------------------------------------------------------
    // circle/aabb queries are argument-order independent
    // ------------------------------------------------------------------
    TEST_CASE("circle/aabb overloads are symmetric in argument order") {
        SUBCASE("static intersects(aabb, circle) == intersects(circle, aabb)") {
            circle c{{0.0f, 0.0f}, 1.0f};
            aabb hit{{0.5f, 0.0f}, {3.0f, 3.0f}};
            aabb miss{{2.0f, 2.0f}, {3.0f, 3.0f}};
            CHECK(intersects(hit, c) == intersects(c, hit));
            CHECK(intersects(miss, c) == intersects(c, miss));
            CHECK(intersects(hit, c));
            CHECK_FALSE(intersects(miss, c));
        }

        SUBCASE("swept_intersection(aabb, circle) matches the canonical (circle, aabb) form") {
            circle c{{0.0f, 0.0f}, 1.0f};
            aabb b{{5.0f, -1.0f}, {7.0f, 1.0f}};
            // Same physical scenario, arguments swapped (and velocities with them).
            auto canon = swept_intersection(c, vec{20.0f, 0.0f}, b, vec{0.0f, 0.0f}, 1.0f);
            auto swapped = swept_intersection(b, vec{0.0f, 0.0f}, c, vec{20.0f, 0.0f}, 1.0f);
            REQUIRE(canon.has_value());
            REQUIRE(swapped.has_value());
            CHECK(swapped->entry_time == doctest::Approx(canon->entry_time));
            CHECK(swapped->exit_time == doctest::Approx(canon->exit_time));
            CHECK(vapprox(swapped->entry_normal, canon->entry_normal));   // outward from the AABB either way
            CHECK(vapprox(swapped->exit_normal, canon->exit_normal));
        }
    }

    // ------------------------------------------------------------------
    // Additional Extended Tests
    // ------------------------------------------------------------------
    TEST_CASE("intersect_param(segment, segment) diagonal collinear and parallel extensions") {
        SUBCASE("diagonal collinear overlap") {
            segment a{{0.0f, 0.0f}, {4.0f, 4.0f}};
            segment b{{2.0f, 2.0f}, {6.0f, 6.0f}};
            auto r = intersect_param(a, b);
            REQUIRE(r.has_value());
            CHECK(r->entry_param == doctest::Approx(0.5f));
            CHECK(r->exit_param == doctest::Approx(1.0f));
            CHECK(is_unit(r->entry_normal));
            CHECK(vapprox(r->entry_normal, r->exit_normal));
            CHECK(intersects(a, b));
        }

        SUBCASE("diagonal collinear overlap reversed") {
            segment a{{0.0f, 0.0f}, {4.0f, 4.0f}};
            segment b{{6.0f, 6.0f}, {2.0f, 2.0f}};
            auto r = intersect_param(a, b);
            REQUIRE(r.has_value());
            CHECK(r->entry_param == doctest::Approx(0.5f));
            CHECK(r->exit_param == doctest::Approx(1.0f));
            CHECK(is_unit(r->entry_normal));
            CHECK(intersects(a, b));
        }

        SUBCASE("diagonal collinear fully inside") {
            segment a{{0.0f, 0.0f}, {4.0f, 4.0f}};
            segment b{{1.0f, 1.0f}, {3.0f, 3.0f}};
            auto r = intersect_param(a, b);
            REQUIRE(r.has_value());
            CHECK(r->entry_param == doctest::Approx(0.25f));
            CHECK(r->exit_param == doctest::Approx(0.75f));
            CHECK(is_unit(r->entry_normal));
            CHECK(intersects(a, b));
        }

        SUBCASE("diagonal collinear endpoint touch") {
            segment a{{0.0f, 0.0f}, {2.0f, 2.0f}};
            segment b{{2.0f, 2.0f}, {4.0f, 4.0f}};
            auto r = intersect_param(a, b);
            REQUIRE(r.has_value());
            CHECK(r->entry_param == doctest::Approx(1.0f));
            CHECK(r->exit_param == doctest::Approx(1.0f));
            CHECK(intersects(a, b));
        }

        SUBCASE("diagonal collinear disjoint") {
            segment a{{0.0f, 0.0f}, {2.0f, 2.0f}};
            segment b{{3.0f, 3.0f}, {5.0f, 5.0f}};
            CHECK_FALSE(intersect_param(a, b).has_value());
            CHECK_FALSE(intersects(a, b));
        }

        SUBCASE("diagonal parallel non-collinear miss") {
            segment a{{0.0f, 0.0f}, {2.0f, 2.0f}};
            segment b{{1.0f, 0.0f}, {3.0f, 2.0f}}; // parallel but shifted by (1, 0)
            CHECK_FALSE(intersect_param(a, b).has_value());
            CHECK_FALSE(intersects(a, b));
        }

        SUBCASE("point b on endpoint of segment a") {
            segment a{{0.0f, 0.0f}, {4.0f, 4.0f}};
            segment b{{4.0f, 4.0f}, {4.0f, 4.0f}};
            auto r = intersect_param(a, b);
            REQUIRE(r.has_value());
            CHECK(r->entry_param == doctest::Approx(1.0f));
            CHECK(r->exit_param == doctest::Approx(1.0f));
            CHECK(intersects(a, b));
        }
    }

    TEST_CASE("zero-radius circle normals and CCD") {
        circle z{{0.0f, 0.0f}, 0.0f};

        SUBCASE("normals are exactly zero on collision") {
            auto r = intersect_param(z, segment{{-2.0f, 0.0f}, {2.0f, 0.0f}});
            REQUIRE(r.has_value());
            CHECK(vapprox(r->entry_normal, vec{0.0f, 0.0f}));
            CHECK(vapprox(r->exit_normal, vec{0.0f, 0.0f}));
        }

        SUBCASE("CCD of two moving points (zero-radius circles) that cross") {
            circle a{{0.0f, 0.0f}, 0.0f};
            circle b{{5.0f, 0.0f}, 0.0f};
            auto r = swept_intersection(a, vec{10.0f, 0.0f}, b, vec{0.0f, 0.0f}, 1.0f);
            REQUIRE(r.has_value());
            CHECK(r->entry_time == doctest::Approx(0.5f));
            CHECK(r->exit_time == doctest::Approx(0.5f));
            CHECK(vapprox(r->entry_normal, vec{0.0f, 0.0f}));
            CHECK(vapprox(r->exit_normal, vec{0.0f, 0.0f}));
        }

        SUBCASE("CCD of point (zero-radius circle) vs AABB") {
            circle c{{0.0f, 0.0f}, 0.0f};
            aabb b{{4.0f, -1.0f}, {6.0f, 1.0f}};
            auto r = swept_intersection(c, vec{10.0f, 0.0f}, b, vec{0.0f, 0.0f}, 1.0f);
            REQUIRE(r.has_value());
            CHECK(r->entry_time == doctest::Approx(0.4f)); // reaches x = 4 at t = 0.4
            CHECK(r->exit_time == doctest::Approx(0.6f));  // leaves x = 6 at t = 0.6
            CHECK(vapprox(r->entry_normal, vec{-1.0f, 0.0f}));
            CHECK(vapprox(r->exit_normal, vec{1.0f, 0.0f}));
        }
    }

    TEST_CASE("intersect_param(aabb, segment) additional boundary and interior cases") {
        aabb box{{0.0f, 0.0f}, {2.0f, 2.0f}};

        SUBCASE("vertical segment on x == min checks normals") {
            auto e = intersect_param(box, segment{{0.0f, -1.0f}, {0.0f, 3.0f}});
            REQUIRE(e.has_value());
            REQUIRE(e->segment_overlaps());
            CHECK(vapprox(e->entry_normal, vec{0.0f, -1.0f}));
            CHECK(vapprox(e->exit_normal, vec{0.0f, 1.0f}));
        }

        SUBCASE("vertical segment on x == max checks normals") {
            auto e = intersect_param(box, segment{{2.0f, -1.0f}, {2.0f, 3.0f}});
            REQUIRE(e.has_value());
            REQUIRE(e->segment_overlaps());
            CHECK(vapprox(e->entry_normal, vec{0.0f, -1.0f}));
            CHECK(vapprox(e->exit_normal, vec{0.0f, 1.0f}));
        }

        SUBCASE("horizontal segment on y == min checks normals") {
            auto e = intersect_param(box, segment{{-1.0f, 0.0f}, {3.0f, 0.0f}});
            REQUIRE(e.has_value());
            REQUIRE(e->segment_overlaps());
            CHECK(vapprox(e->entry_normal, vec{-1.0f, 0.0f}));
            CHECK(vapprox(e->exit_normal, vec{1.0f, 0.0f}));
        }

        SUBCASE("degenerate point segment inside the box checks normals") {
            auto e = intersect_param(box, segment{{1.0f, 1.0f}, {1.0f, 1.0f}});
            REQUIRE(e.has_value());
            REQUIRE(e->segment_overlaps());
            CHECK(vapprox(e->entry_normal, vec{-1.0f, 0.0f}));
            CHECK(vapprox(e->exit_normal, vec{1.0f, 0.0f}));
        }

        SUBCASE("segment completely inside the box yields entry < 0 and exit > 1") {
            segment s{{0.5f, 0.5f}, {1.5f, 1.5f}};
            auto e = intersect_param(box, s);
            REQUIRE(e.has_value());
            CHECK(e->entry_param == doctest::Approx(-0.5f));
            CHECK(e->exit_param == doctest::Approx(1.5f));
            CHECK(e->segment_overlaps());
            CHECK(vapprox(e->entry_normal, vec{-1.0f, 0.0f}));
            CHECK(vapprox(e->exit_normal, vec{1.0f, 0.0f}));
        }
    }
}
