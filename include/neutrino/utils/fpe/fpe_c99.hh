//
// Created by igor on 25/07/2021.
//

#ifndef NEUTRINO_FPE_C99_HH
#define NEUTRINO_FPE_C99_HH
#include <fenv.h>
#include <cmath>

namespace neutrino::utils {

  class fpe_impl {
    protected:
      enum RoundingModeImpl {
        FP_ROUND_DOWNWARD_IMPL = FE_DOWNWARD,
        FP_ROUND_UPWARD_IMPL = FE_UPWARD,
        FP_ROUND_TONEAREST_IMPL = FE_TONEAREST,
        FP_ROUND_TOWARDZERO_IMPL = FE_TOWARDZERO
      };
      enum FlagImpl {
        FP_DIVIDE_BY_ZERO_IMPL = FE_DIVBYZERO,
        FP_INEXACT_IMPL = FE_INEXACT,
        FP_OVERFLOW_IMPL = FE_OVERFLOW,
        FP_UNDERFLOW_IMPL = FE_UNDERFLOW,
        FP_INVALID_IMPL = FE_INVALID
      };
      fpe_impl ();
      fpe_impl (const fpe_impl& env);
      ~fpe_impl ();
      fpe_impl& operator = (const fpe_impl& env);
      void keepCurrentImpl ();
      static void clearFlagsImpl ();
      static bool isFlagImpl (FlagImpl flag);
      static void setRoundingModeImpl (RoundingModeImpl mode);
      static RoundingModeImpl getRoundingModeImpl ();
      static bool isInfiniteImpl (float value);
      static bool isInfiniteImpl (double value);
      static bool isInfiniteImpl (long double value);
      static bool isNaNImpl (float value);
      static bool isNaNImpl (double value);
      static bool isNaNImpl (long double value);
      static float copySignImpl (float target, float source);
      static double copySignImpl (double target, double source);
      static long double copySignImpl (long double target, long double source);

    private:
      fenv_t _env;
  };

//
// inlines
//
  inline bool fpe_impl::isInfiniteImpl (float value) {
    return std::isinf (value) != 0;
  }

  inline bool fpe_impl::isInfiniteImpl (double value) {
    return std::isinf (value) != 0;
  }

  inline bool fpe_impl::isInfiniteImpl (long double value) {
    return std::isinf ((double) value) != 0;
  }

  inline bool fpe_impl::isNaNImpl (float value) {
    return std::isnan (value) != 0;
  }

  inline bool fpe_impl::isNaNImpl (double value) {
    return std::isnan (value) != 0;
  }

  inline bool fpe_impl::isNaNImpl (long double value) {
    return std::isnan ((double) value) != 0;
  }

  inline float fpe_impl::copySignImpl (float target, float source) {
    return copysignf (target, source);
  }

  inline double fpe_impl::copySignImpl (double target, double source) {
    return copysign (target, source);
  }

} // namespace Poco

#endif //NEUTRINO_FPE_C99_HH
