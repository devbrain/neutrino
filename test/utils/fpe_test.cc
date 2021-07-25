//
// Created by igor on 25/07/2021.
//

#include <doctest/doctest.h>
#include <neutrino/utils/fpe/fpe.hh>
using FPE = neutrino::utils::fpe;

static double mult(double a, double b)
{
    return a*b;
}

static double div(double a, double b)
{
    return a/b;
}

TEST_SUITE("FPE test") {
    TEST_CASE("Classify")
    {
        {
            float a = 0.0f;
            float b = 0.0f;
            float nan = a/b;
            float inf = 1.0f/b;

            REQUIRE (FPE::is_NaN(nan));
            REQUIRE (!FPE::is_NaN(a));
            REQUIRE (FPE::is_infinite(inf));
            REQUIRE (!FPE::is_infinite(a));
        }
        {
            double a = 0;
            double b = 0;
            double nan = a/b;
            double inf = 1.0/b;

            REQUIRE (FPE::is_NaN(nan));
            REQUIRE (!FPE::is_NaN(a));
            REQUIRE (FPE::is_infinite(inf));
            REQUIRE (!FPE::is_infinite(a));
        }
    }

#if defined(__HP_aCC)
#pragma OPTIMIZE OFF
#elif defined(_MSC_VER)
#pragma optimize("", off)
#elif defined(__APPLE__) && defined(__GNUC__)
#pragma GCC optimization_level 0
#endif

    TEST_CASE("flags")
    {
        FPE::clear_flags();

        // some compilers are intelligent enough to optimize the calculations below away.
        // unfortunately this leads to a failing test, so we have to trick out the
        // compiler's optimizer a little bit by doing something with the results.
        volatile double a = 10;
        volatile double b = 0;
        volatile double c = div(a, b);

#if !defined(POCO_NO_FPENVIRONMENT)
        REQUIRE (FPE::is_flag(FPE::FP_DIVIDE_BY_ZERO));
#endif
        REQUIRE (FPE::is_infinite(c));

        FPE::clear_flags();
        a = 1.23456789e210;
        b = 9.87654321e210;
        c = mult(a, b);

        REQUIRE (FPE::is_flag(FPE::FP_OVERFLOW));

        FPE::clear_flags();
        a = 1.23456789e-99;
        b = 9.87654321e210;
        c = div(a, b);

        REQUIRE (FPE::is_flag(FPE::FP_UNDERFLOW));

    }

#if defined(__HP_aCC)
#pragma OPTIMIZE ON
#elif defined(_MSC_VER)
#pragma optimize("", on)
#elif defined(__APPLE__) && defined(__GNUC__)
#pragma GCC optimization_level reset
#endif

    TEST_CASE("round") {
        FPE::set_rounding_mode(FPE::FP_ROUND_TONEAREST);
        REQUIRE (FPE::get_rounding_mode() == FPE::FP_ROUND_TONEAREST);
        {
            FPE env(FPE::FP_ROUND_TOWARDZERO);
            REQUIRE (FPE::get_rounding_mode() == FPE::FP_ROUND_TOWARDZERO);
        }
        REQUIRE (FPE::get_rounding_mode() == FPE::FP_ROUND_TONEAREST);
    }

}