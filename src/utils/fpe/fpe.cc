//
// Created by igor on 25/07/2021.
//
#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <neutrino/utils/fpe/fpe.hh>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)

namespace neutrino::utils
{

fpe_impl::fpe_impl()
{
    _env = _controlfp(0, 0);
}


fpe_impl::fpe_impl(const fpe_impl& env)
{
    _env = env._env;
}


fpe_impl::~fpe_impl()
{
    _controlfp(_env, _MCW_RC);
}


fpe_impl& fpe_impl::operator = (const fpe_impl& env)
{
    _env = env._env;
    return *this;
}


void fpe_impl::keepCurrentImpl()
{
    _env = _controlfp(0, 0);
}


void fpe_impl::clearFlagsImpl()
{
    _clearfp();
}


bool fpe_impl::isFlagImpl(FlagImpl flag)
{
    return (_statusfp() & flag) != 0;
}


void fpe_impl::setRoundingModeImpl(RoundingModeImpl mode)
{
    _controlfp(mode, _MCW_RC);
}


fpe_impl::RoundingModeImpl fpe_impl::getRoundingModeImpl()
{
    return RoundingModeImpl(_controlfp(0, 0) & _MCW_RC);
}

}
#else
namespace neutrino::utils {
  fpe_impl::fpe_impl () {
    fegetenv (&_env);
  }

  fpe_impl::fpe_impl (const fpe_impl &env) {
    _env = env._env;
  }

  fpe_impl::~fpe_impl () {
    fesetenv (&_env);
  }

  fpe_impl &fpe_impl::operator= (const fpe_impl &env) {
    _env = env._env;
    return *this;
  }

  void fpe_impl::keepCurrentImpl () {
    fegetenv (&_env);
  }

  void fpe_impl::clearFlagsImpl () {
    feclearexcept (FE_ALL_EXCEPT);
  }

  bool fpe_impl::isFlagImpl (FlagImpl flag) {
    return fetestexcept (flag) != 0;
  }

  void fpe_impl::setRoundingModeImpl (RoundingModeImpl mode) {
    fesetround (mode);
  }

  fpe_impl::RoundingModeImpl fpe_impl::getRoundingModeImpl () {
    return (RoundingModeImpl) fegetround ();
  }

  long double fpe_impl::copySignImpl (long double target, long double source) {
    return (source >= 0 && target >= 0) || (source < 0 && target < 0) ? target : -target;
  }

}
#endif

namespace neutrino::utils {
  fpe::fpe () {
  }

  fpe::fpe (rounding_mode_t rm) {
    set_rounding_mode (rm);
  }

  fpe::fpe (const fpe &env)
      : fpe_impl (env) {
  }

  fpe::~fpe () {
  }

  fpe &fpe::operator= (const fpe &env) {
    if (&env != this) {
      fpe_impl::operator= (env);
    }
    return *this;
  }

  void fpe::keep_current () {
    keepCurrentImpl ();
  }

  void fpe::clear_flags () {
    clearFlagsImpl ();
  }
}
