//
// Created by igor on 25/07/2021.
//

#ifndef NEUTRINO_FPE_WINDOWS_HH
#define NEUTRINO_FPE_WINDOWS_HH
#include <float.h>
#include <math.h>

#ifndef _SW_INEXACT
#	define _SW_INEXACT 0x00000001 // inexact (precision)
#endif
#ifndef _SW_UNDERFLOW
#	define _SW_UNDERFLOW 0x00000002 // underflow
#endif
#ifndef _SW_OVERFLOW
#	define _SW_OVERFLOW 0x00000004 // overflow
#endif
#ifndef _SW_ZERODIVIDE
#	define _SW_ZERODIVIDE 0x00000008 // zero divide
#endif
#ifndef _SW_INVALID
#	define _SW_INVALID 0x00000010 // invalid
#endif
#ifndef _SW_DENORMAL
#	define _SW_DENORMAL 0x00080000 // denormal status bit
#endif


namespace neutrino::utils {


    class fpe_impl
            {
                    protected:
                    enum RoundingModeImpl
                    {
                        FP_ROUND_DOWNWARD_IMPL   = _RC_DOWN,
                        FP_ROUND_UPWARD_IMPL     = _RC_UP,
                        FP_ROUND_TONEAREST_IMPL  = _RC_NEAR,
                        FP_ROUND_TOWARDZERO_IMPL = _RC_CHOP
                    };
                    enum FlagImpl
                    {
                        FP_DIVIDE_BY_ZERO_IMPL = _SW_ZERODIVIDE,
                                FP_INEXACT_IMPL        = _SW_INEXACT,
                                FP_OVERFLOW_IMPL       = _SW_OVERFLOW,
                                FP_UNDERFLOW_IMPL      = _SW_UNDERFLOW,
                                FP_INVALID_IMPL        = _SW_INVALID
                    };
                    fpe_impl();
                    fpe_impl(const fpe_impl& env);
                    ~fpe_impl();
                    fpe_impl& operator = (const fpe_impl& env);
                    void keepCurrentImpl();
                    static void clearFlagsImpl();
                    static bool isFlagImpl(FlagImpl flag);
                    static void setRoundingModeImpl(RoundingModeImpl mode);
                    static RoundingModeImpl getRoundingModeImpl();
                    static bool isInfiniteImpl(float value);
                    static bool isInfiniteImpl(double value);
                    static bool isInfiniteImpl(long double value);
                    static bool isNaNImpl(float value);
                    static bool isNaNImpl(double value);
                    static bool isNaNImpl(long double value);
                    static float copySignImpl(float target, float source);
                    static double copySignImpl(double target, double source);
                    static long double copySignImpl(long double target, long double source);

                    private:
                    unsigned _env;
            };


//
// inlines
//
    inline bool fpe_impl::isInfiniteImpl(float value)
    {
        if (_isnan(value) != 0) return false;
        return _finite(value) == 0;
    }


    inline bool fpe_impl::isInfiniteImpl(double value)
    {
        if (_isnan(value) != 0) return false;
        return _finite(value) == 0;
    }


    inline bool fpe_impl::isInfiniteImpl(long double value)
    {
        if (_isnan(value) != 0) return false;
        return _finite(value) == 0;
    }


    inline bool fpe_impl::isNaNImpl(float value)
    {
        return _isnan(value) != 0;
    }


    inline bool fpe_impl::isNaNImpl(double value)
    {
        return _isnan(value) != 0;
    }


    inline bool fpe_impl::isNaNImpl(long double value)
    {
        return _isnan(value) != 0;
    }


    inline float fpe_impl::copySignImpl(float target, float source)
    {
        return float(_copysign(target, source));
    }


    inline double fpe_impl::copySignImpl(double target, double source)
    {
        return _copysign(target, source);
    }


    inline long double fpe_impl::copySignImpl(long double target, long double source)
    {
        return (source > 0 && target > 0) || (source < 0 && target < 0) ? target : -target;
    }


}

#endif //NEUTRINO_FPE_WINDOWS_HH
