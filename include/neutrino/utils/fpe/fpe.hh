//
// Created by igor on 25/07/2021.
//

#ifndef NEUTRINO_FPE_HH
#define NEUTRINO_FPE_HH

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#include <neutrino/utils/fpe/fpe_windows.hh>
#else

#include <neutrino/utils/fpe/fpe_c99.hh>

#endif

namespace neutrino::utils {

  class fpe : private fpe_impl
      /// Instances of this class can be used to save
      /// and later restore the current floating
      /// point environment (consisting of rounding
      /// mode and floating-point flags).
      /// The class also provides various static
      /// methods to query certain properties
      /// of a floating-point number.
  {
    public:
      enum rounding_mode_t {
        FP_ROUND_DOWNWARD = FP_ROUND_DOWNWARD_IMPL,
        FP_ROUND_UPWARD = FP_ROUND_UPWARD_IMPL,
        FP_ROUND_TONEAREST = FP_ROUND_TONEAREST_IMPL,
        FP_ROUND_TOWARDZERO = FP_ROUND_TOWARDZERO_IMPL
      };

      enum flag_t {
        FP_DIVIDE_BY_ZERO = FP_DIVIDE_BY_ZERO_IMPL,
        FP_INEXACT = FP_INEXACT_IMPL,
        FP_OVERFLOW = FP_OVERFLOW_IMPL,
        FP_UNDERFLOW = FP_UNDERFLOW_IMPL,
        FP_INVALID = FP_INVALID_IMPL
      };

      fpe ();
      /// Standard constructor.
      /// Remembers the current environment.

      fpe (rounding_mode_t mode);
      /// Remembers the current environment and
      /// sets the given rounding mode.

      fpe (const fpe& env);
      /// Copy constructor.

      ~fpe ();
      /// Restores the previous environment (unless
      /// keep_current() has been called previously)

      fpe& operator = (const fpe& env);
      /// Assignment operator

      void keep_current ();
      /// Keep the current environment even after
      /// destroying the FPEnvironment object.

      static void clear_flags ();
      /// Resets all flags.

      static bool is_flag (flag_t flag);
      /// Returns true iff the given flag is set.

      static void set_rounding_mode (rounding_mode_t mode);
      /// Sets the rounding mode.

      static rounding_mode_t get_rounding_mode ();
      /// Returns the current rounding mode.

      static bool is_infinite (float value);
      static bool is_infinite (double value);
      static bool is_infinite (long double value);
      /// Returns true iff the given number is infinite.

      static bool is_NaN (float value);
      static bool is_NaN (double value);
      static bool is_NaN (long double value);
      /// Returns true iff the given number is NaN.

      static float copy_sign (float target, float source);
      static double copy_sign (double target, double source);
      static long double copy_sign (long double target, long double source);
      /// Copies the sign from source to target.
  };

//
// For convenience, we provide a shorter name for
// the FPEnvironment class.
//
  typedef fpe FPE;

//
// inline's
//
  inline bool fpe::is_flag (flag_t flag) {
    return isFlagImpl (FlagImpl (flag));
  }

  inline void fpe::set_rounding_mode (rounding_mode_t mode) {
    setRoundingModeImpl (RoundingModeImpl (mode));
  }

  inline fpe::rounding_mode_t fpe::get_rounding_mode () {
    return rounding_mode_t (getRoundingModeImpl ());
  }

  inline bool fpe::is_infinite (float value) {
    return isInfiniteImpl (value);
  }

  inline bool fpe::is_infinite (double value) {
    return isInfiniteImpl (value);
  }

  inline bool fpe::is_infinite (long double value) {
    return isInfiniteImpl (value);
  }

  inline bool fpe::is_NaN (float value) {
    return isNaNImpl (value);
  }

  inline bool fpe::is_NaN (double value) {
    return isNaNImpl (value);
  }

  inline bool fpe::is_NaN (long double value) {
    return isNaNImpl (value);
  }

  inline float fpe::copy_sign (float target, float source) {
    return copySignImpl (target, source);
  }

  inline double fpe::copy_sign (double target, double source) {
    return copySignImpl (target, source);
  }

  inline long double fpe::copy_sign (long double target, long double source) {
    return copySignImpl (target, source);
  }

}

#endif //NEUTRINO_FPE_HH
