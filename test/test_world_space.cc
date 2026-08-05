//
// Tests for the strong gameplay-space vocabulary (roadmap Tier 3).
//
// These pin the ALGEBRA -- which combinations exist and what they produce. The combinations that
// must NOT compile (world_pos + world_pos, world_velocity used as world_delta) cannot be asserted
// at runtime; they are guarded by static_assert on the type relationships below, which fail to
// compile if an operator is ever added that collapses two of these types together.
//
#include <doctest/doctest.h>

#include <neutrino/world_space.hh>
#include <neutrino/video/geometry_types.hh>

#include <type_traits>

using namespace neutrino;

namespace {
    constexpr world_seconds half{0.5f};
    constexpr world_seconds two{2.0f};
} // namespace

// The whole value of the exercise is that these are DISTINCT types, not aliases for one another.
// If any of these ever becomes a typedef of another, the bug class comes straight back.
static_assert(!std::is_same_v<world_pos, world_delta>);
static_assert(!std::is_same_v<world_pos, world_velocity>);
static_assert(!std::is_same_v<world_delta, world_velocity>);
static_assert(!std::is_same_v<render_pos, window_pos>);
// ...and that the algebra lands in the right type.
static_assert(std::is_same_v<decltype(world_pos{} - world_pos{}), world_delta>);
static_assert(std::is_same_v<decltype(world_pos{} + world_delta{}), world_pos>);
static_assert(std::is_same_v<decltype(world_velocity{} * world_seconds{}), world_delta>);
static_assert(std::is_same_v<decltype(world_delta{} / world_seconds{}), world_velocity>);

TEST_SUITE("neutrino world_space") {
    TEST_CASE("positions and offsets compose the way places and gaps do") {
        constexpr world_pos a{10.0f, 4.0f};
        constexpr world_pos b{13.0f, 8.0f};

        SUBCASE("difference of two places is the gap between them") {
            constexpr world_delta d = b - a;
            CHECK(d.x == doctest::Approx(3.0f));
            CHECK(d.y == doctest::Approx(4.0f));
            CHECK(d.length() == doctest::Approx(5.0f));   // 3-4-5
        }

        SUBCASE("a place displaced by that gap is the other place") {
            CHECK(a + (b - a) == b);
            CHECK(b - (b - a) == a);
        }

        SUBCASE("offsets add, scale and negate") {
            constexpr world_delta d{3.0f, 4.0f};
            CHECK((d + d) == world_delta{6.0f, 8.0f});
            CHECK((d * 2.0f) == world_delta{6.0f, 8.0f});
            CHECK((2.0f * d) == world_delta{6.0f, 8.0f});
            CHECK((-d) == world_delta{-3.0f, -4.0f});
            CHECK((d - d) == world_delta{});
        }

        SUBCASE("compound assignment keeps the types straight") {
            world_pos p{1.0f, 1.0f};
            p += world_delta{2.0f, 3.0f};
            CHECK(p == world_pos{3.0f, 4.0f});
            p -= world_delta{2.0f, 3.0f};
            CHECK(p == world_pos{1.0f, 1.0f});
        }

        SUBCASE("normalizing a zero offset yields zero, not NaN") {
            CHECK(world_delta{}.normalized() == world_delta{});
            const world_delta n = world_delta{3.0f, 4.0f}.normalized();
            CHECK(n.length() == doctest::Approx(1.0f));
        }
    }

    TEST_CASE("velocity integrates to an offset and differentiates back") {
        constexpr world_velocity v{10.0f, -4.0f};

        SUBCASE("integrating a rate over a span gives the distance travelled") {
            constexpr world_delta d = v * half;
            CHECK(d.x == doctest::Approx(5.0f));
            CHECK(d.y == doctest::Approx(-2.0f));
            CHECK((half * v) == d);                       // commutes
        }

        SUBCASE("differentiating a move gives the rate it happened at") {
            // This is the EFFECTIVE-velocity computation that was got wrong twice by hand this
            // cycle: "how fast did the body actually go", not "how fast was it asked to go".
            constexpr world_delta travelled{5.0f, -2.0f};
            constexpr world_velocity effective = travelled / half;
            CHECK(effective.x == doctest::Approx(10.0f));
            CHECK(effective.y == doctest::Approx(-4.0f));
        }

        SUBCASE("integrate then differentiate round-trips") {
            CHECK(((v * two) / two).x == doctest::Approx(v.x));
            CHECK(((v * two) / two).y == doctest::Approx(v.y));
        }

        SUBCASE("a zero span has no rate, and does not divide by zero") {
            CHECK((world_delta{5.0f, 5.0f} / world_seconds{0.0f}) == world_velocity{});
        }

        SUBCASE("speed is the magnitude of the rate") {
            CHECK(world_velocity{3.0f, 4.0f}.speed() == doctest::Approx(5.0f));
            CHECK(world_velocity{3.0f, 4.0f}.speed_sq() == doctest::Approx(25.0f));
        }
    }

    TEST_CASE("world_bounds answers containment against positions") {
        constexpr world_bounds box{world_pos{0.0f, 0.0f}, world_pos{10.0f, 4.0f}};

        CHECK(box.size() == world_delta{10.0f, 4.0f});
        CHECK(box.centre() == world_pos{5.0f, 2.0f});
        CHECK(box.contains(world_pos{5.0f, 2.0f}));
        CHECK(box.contains(world_pos{0.0f, 0.0f}));        // inclusive edges
        CHECK_FALSE(box.contains(world_pos{-0.1f, 2.0f}));
        CHECK_FALSE(box.contains(world_pos{5.0f, 4.1f}));

        CHECK(box.intersects(world_bounds{world_pos{9.0f, 3.0f}, world_pos{20.0f, 20.0f}}));
        CHECK_FALSE(box.intersects(world_bounds{world_pos{11.0f, 0.0f}, world_pos{20.0f, 4.0f}}));
    }

    // The untyped world_point survives at the DRAWING edge, where the renderer wants a bare pair.
    // The crossing is by named function only -- neither direction is implicit, so a gameplay
    // position cannot slip into a draw call (or back out of one) without saying so.
    TEST_CASE("crossing to the untyped world_point is explicit and round-trips") {
        static_assert(!std::is_convertible_v<world_pos, world_point>);
        static_assert(!std::is_convertible_v<world_point, world_pos>);
        static_assert(!std::is_convertible_v<world_delta, world_point>);

        constexpr world_pos p{12.5f, -3.25f};
        const world_point wp = to_world_point(p);
        CHECK(wp.x == doctest::Approx(12.5f));
        CHECK(wp.y == doctest::Approx(-3.25f));
        CHECK(to_world_pos(wp) == p);

        // Offsets cross too -- a sprite pivot shift is a delta, and stays one until it is applied.
        CHECK(to_world_point(world_delta{-4.0f, -4.0f}) == world_point{-4.0f, -4.0f});
        CHECK(to_world_point(p + world_delta{-4.0f, -4.0f}) == world_point{8.5f, -7.25f});
    }

    // render and window space are related by a NON-identity mapping (presentation scale +
    // letterbox, plus HiDPI), so they are separate types and cannot be swapped by accident.
    TEST_CASE("render and window positions are not interchangeable") {
        static_assert(!std::is_convertible_v<window_pos, render_pos>);
        static_assert(!std::is_convertible_v<render_pos, window_pos>);
        constexpr render_pos r{160.0f, 100.0f};
        CHECK(r == render_pos{160.0f, 100.0f});
    }
}
