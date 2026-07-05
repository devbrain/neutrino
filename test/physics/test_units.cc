//
// Created by igor on 23/06/2026.
//
// Tests for the strong physical-unit types in <neutrino/physics/collide/units.hh>. Two things to
// pin down: the legal arithmetic produces the right value, and the ILLEGAL combinations do
// not compile (the whole point -- they are what catch the dt/toi confusion bugs).
//
#include <doctest/doctest.h>

#include <cmath>

#include <neutrino/physics/geometry/units.hh>

using namespace neutrino::physics;
using units::duration;
using units::fraction;
using units::velocity;
using units::displacement;

namespace {
    bool eq(const vec& a, float x, float y) {
        return std::fabs(a.x() - x) < 1e-6f && std::fabs(a.y() - y) < 1e-6f;
    }

    // `mul<A,B>` is satisfied iff `a * b` is a valid expression. Used below to assert that the
    // ILLEGAL unit products are rejected (a named concept keeps the check in a clean SFINAE
    // context, unlike an inline requires in a static_assert).
    template <class A, class B>
    concept mul = requires(A a, B b) { a * b; };
} // namespace

TEST_SUITE("units: legal algebra") {
    TEST_CASE("velocity * duration = displacement") {
        const displacement d = velocity{{3.0f, -4.0f}} * duration{0.5f};
        CHECK(eq(d.value, 1.5f, -2.0f));
        // symmetric
        const displacement d2 = duration{0.5f} * velocity{{3.0f, -4.0f}};
        CHECK(eq(d2.value, 1.5f, -2.0f));
    }

    TEST_CASE("displacement * fraction = displacement") {
        const displacement full{{10.0f, 0.0f}};
        const displacement part = full * fraction{0.375f};
        CHECK(eq(part.value, 3.75f, 0.0f));
        CHECK(eq((fraction{0.375f} * full).value, 3.75f, 0.0f)); // symmetric
    }

    TEST_CASE("displacement +/- displacement = displacement") {
        const displacement a{{1.0f, 2.0f}};
        const displacement b{{3.0f, -1.0f}};
        CHECK(eq((a + b).value, 4.0f, 1.0f));
        CHECK(eq((a - b).value, -2.0f, 3.0f));
    }

    TEST_CASE("the two real-world chains type-check and compute correctly") {
        // delta = v * dt ; then take the part up to the TOI -- the exact shape of the bullet/
        // move-and-slide math that previously had the dt bug.
        const duration dt{0.5f};
        const displacement delta = velocity{{20.0f, 0.0f}} * dt; // -> (10, 0)
        CHECK(eq(delta.value, 10.0f, 0.0f));
        const displacement to_hit = delta * fraction{0.375f};    // -> (3.75, 0)
        CHECK(eq(to_hit.value, 3.75f, 0.0f));
    }
}

// ---- illegal combinations must NOT compile ---------------------------------------
// These are the guards that turn the dt/toi confusion into a build error. If any of these
// `requires` becomes true, the algebra has been loosened and a whole class of unit bug is
// no longer caught.
TEST_SUITE("units: illegal combinations are rejected") {
    TEST_CASE("ill-typed products do not compile") {
        static_assert(!mul<velocity, fraction>);     // missing-dt bug shape (v * toi)
        static_assert(!mul<displacement, duration>); // extra-dt bug shape (delta * dt)
        static_assert(!mul<duration, fraction>);     // meaningless
        static_assert(!mul<velocity, velocity>);
        static_assert(!mul<displacement, displacement>);
        static_assert(!mul<velocity, float>);        // raw float where a typed scalar belongs
        static_assert(!mul<displacement, float>);
        // sanity: the LEGAL ones are satisfied
        static_assert(mul<velocity, duration>);
        static_assert(mul<displacement, fraction>);
        CHECK(true); // the real assertions are the static_asserts above
    }
}
