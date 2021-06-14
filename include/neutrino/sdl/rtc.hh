#ifndef NEUTRINO_SDL_RTC_HH
#define NEUTRINO_SDL_RTC_HH

#include <neutrino/utils/exception.hh>

#define RTC_PALETTE

#if !defined(RTC_NO_OP)
#define RTC_NO_OP ((void)0)
#endif
#if !defined(RTC_OP)
#define RTC_OP(value, ...) if (!(value)) RAISE_EX(__VA_ARGS__)
#endif

#if defined(DEBUG) || defined(_DEBUG)
#define RTC_PALETTE
#define RTC_SURFACE
#endif

// RTC for palette
#if defined(RTC_PALETTE)
#define RTC_IN_PALETTE(value, ...) RTC_OP(value, ##__VA_ARGS__)
#else
#define RTC_IN_PALETTE(value, ...) RTC_NO_OP
#endif

// RTC for surface
#if defined(RTC_SURFACE)
#define RTC_IN_SURFACE(value, ...) RTC_OP(value, ##__VA_ARGS__)
#else
#define RTC_IN_SURFACE(value, ...) RTC_NO_OP
#endif

#endif
