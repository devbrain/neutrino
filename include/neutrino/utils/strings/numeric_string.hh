//
// Created by igor on 25/07/2021.
//

#ifndef NEUTRINO_UTILS_NUMERIC_STRING_HH
#define NEUTRINO_UTILS_NUMERIC_STRING_HH

#include <limits>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <neutrino/utils/exception.hh>
#include <neutrino/utils/fpe/fpe.hh>

#if !defined(POCO_NO_LOCALE)

#include <locale>

#endif

namespace neutrino::utils {
  typedef uint64_t uintmax_t;
  typedef int64_t intmax_t;
  inline constexpr auto intmax_max = std::numeric_limits<intmax_t>::max ();

// binary numbers are supported, thus 64 (bits) + 1 (string terminating zero)
  inline constexpr auto NEUTRINO_MAX_INT_STRING_LEN = 65;
// value from strtod.cc (double_conversion::kMaxSignificantDecimalDigits)
  inline constexpr auto NEUTRINO_MAX_FLT_STRING_LEN = 780;

  inline constexpr auto NEUTRINO_FLT_INF = "inf";
  inline constexpr auto NEUTRINO_FLT_NAN = "nan";
  inline constexpr auto NEUTRINO_FLT_EXP = 'e';

  namespace impl {

    template <bool SIGNED, typename T>
    class IsNegativeImpl;

    template <typename T>
    class IsNegativeImpl<true, T> {
      public:
        bool operator() (T x) {
          return x < 0;
        }
    };

    template <typename T>
    class IsNegativeImpl<false, T> {
      public:
        bool operator() (T) {
          return false;
        }
    };

  }

  template <typename T>
  inline bool isNegative (T x) {
    using namespace impl;
    return IsNegativeImpl<std::numeric_limits<T>::is_signed, T> () (x);
  }

  template <typename To, typename From>
  inline bool isIntOverflow (From val) {
    ENFORCE (std::numeric_limits<From>::is_integer);
    ENFORCE (std::numeric_limits<To>::is_integer);
    bool ret;
    if (std::numeric_limits<To>::is_signed) {
      ret = (!std::numeric_limits<From>::is_signed &&
             (uintmax_t) val > (uintmax_t) intmax_max) ||
            (intmax_t) val < (intmax_t) std::numeric_limits<To>::min () ||
            (intmax_t) val > (intmax_t) std::numeric_limits<To>::max ();
    }
    else {
      ret = isNegative (val) ||
            (uintmax_t) val > (uintmax_t) std::numeric_limits<To>::max ();
    }
    return ret;
  }

  template <typename F, typename T>
  inline T &isSafeIntCast (F from)
  /// Returns true if it is safe to cast
  /// integer from F to T.
  {
    if (!isIntOverflow<T, F> (from)) {
      return true;
    }
    return false;
  }

  template <typename F, typename T>
  inline T &safeIntCast (F from, T &to)
  /// Returns csted value if it is safe
  /// to cast integer from F to T,
  /// otherwise throws BadCastException.
  {
    if (!isIntOverflow<T, F> (from)) {
      to = static_cast<T>(from);
      return to;
    }
    RAISE_EX("safeIntCast: Integer overflow");
  }

  inline char decimalSeparator ()
  /// Returns decimal separator from global locale or
  /// default '.' for platforms where locale is unavailable.
  {
#if !defined(POCO_NO_LOCALE)
    return std::use_facet<std::numpunct<char>> (std::locale ()).decimal_point ();
#else
    return '.';
#endif
  }

  inline char thousandSeparator ()
  /// Returns thousand separator from global locale or
  /// default ',' for platforms where locale is unavailable.
  {
#if !defined(POCO_NO_LOCALE)
    return std::use_facet<std::numpunct<char>> (std::locale ()).thousands_sep ();
#else
    return ',';
#endif
  }


//
// String to Number Conversions
//

  template <typename I>
  bool str_to_int (const char *pStr, I &outResult, short base, char thSep = ',')
  /// Converts zero-terminated character array to integer number;
  /// Thousand separators are recognized for base10 and current locale;
  /// they are silently skipped and not verified for correct positioning.
  /// It is not allowed to convert a negative number to unsigned integer.
  ///
  /// Function returns true if successful. If parsing was unsuccessful,
  /// the return value is false with the result value undetermined.
  {
    ENFORCE (base == 2 || base == 8 || base == 10 || base == 16);

    if (!pStr) {
      return false;
    }
    while (std::isspace (*pStr)) {
      ++pStr;
    }
    if (*pStr == '\0') {
      return false;
    }
    bool negative = false;
    if ((base == 10) && (*pStr == '-')) {
      if (!std::numeric_limits<I>::is_signed) {
        return false;
      }
      negative = true;
      ++pStr;
    }
    else {
      if (*pStr == '+') {
        ++pStr;
      }
    }

    // all numbers are parsed as positive; the sign
    // for negative numbers is adjusted after parsing
    uintmax_t limitCheck = std::numeric_limits<I>::max ();
    if (negative) {
      ENFORCE(std::numeric_limits<I>::is_signed);
      // to cover the entire range, (-min > max) has to be
      // taken into account;
      // to avoid overflow for the largest int size,
      // we resort to FPEnvironment::copySign() (ie. floating-point)
      if constexpr (sizeof (I) == sizeof (intmax_t)) {
        limitCheck = static_cast<uintmax_t>(fpe::copy_sign (static_cast<double>(std::numeric_limits<I>::min ()),
                                                            1));
      }
      else {
        intmax_t i = std::numeric_limits<I>::min ();
        limitCheck = -i;
      }
    }
#if defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough="
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif
    uintmax_t result = 0;
    for (; *pStr != '\0'; ++pStr) {
      if (result > (limitCheck / base)) {
        return false;
      }
      switch (*pStr) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7': {
          char add = (*pStr - '0');
          if ((limitCheck - result) < add) {
            return false;
          }
          result = result * base + add;
        }
          break;

        case '8':
        case '9':
          if ((base == 10) || (base == 0x10)) {
            char add = (*pStr - '0');
            if ((limitCheck - result) < add) {
              return false;
            }
            result = result * base + add;
          }
          else {
            return false;
          }

          break;

        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f': {
          if (base != 0x10) {
            return false;
          }
          char add = (*pStr - 'a');
          if ((limitCheck - result) < add) {
            return false;
          }
          result = result * base + (10 + add);
        }
          break;

        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F': {
          if (base != 0x10) {
            return false;
          }
          char add = (*pStr - 'A');
          if ((limitCheck - result) < add) {
            return false;
          }
          result = result * base + (10 + add);
        }
          break;

        case '.':
          if ((base == 10) && (thSep == '.')) {
            break;
          }
          else {
            return false;
          }

        case ',':
          if ((base == 10) && (thSep == ',')) {
            break;
          }
          else {
            return false;
          }

        case ' ':
          if ((base == 10) && (thSep == ' ')) {
            break;
          }

        default:
          return false;
      }
    }
#if defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#endif

    if (negative && (base == 10)) {
      ENFORCE(std::numeric_limits<I>::is_signed);
      intmax_t i;
      if constexpr (sizeof (I) == sizeof (intmax_t)) {
        i = static_cast<intmax_t>(fpe::copy_sign (static_cast<double>(result), -1));
      }
      else {
#if defined(_MSC_VER)
#pragma warning ( push )
#pragma warning ( disable: 4146 )
#endif
        i = static_cast<intmax_t>(-result);
#if defined(_MSC_VER)
#pragma warning ( pop )
#endif
      }
      if (isIntOverflow<I> (i)) {
        return false;
      }
      outResult = static_cast<I>(i);
    }
    else {
      if (isIntOverflow<I> (result)) {
        return false;
      }
      outResult = static_cast<I>(result);
    }

    return true;
  }

  template <typename I>
  bool str_to_int (const std::string &str, I &result, short base, char thSep = ',')
  /// Converts string to integer number;
  /// This is a wrapper function, for details see see the
  /// bool str_to_int(const char*, I&, short, char) implementation.
  {
    return str_to_int (str.c_str (), result, base, thSep);
  }


//
// Number to String Conversions
//

  namespace impl {

    class ptr
        /// Utility char pointer wrapper class.
        /// Class ensures increment/decrement remain within boundaries.
    {
      public:
        ptr (char *ptr, std::size_t offset)
            : _beg (ptr), _cur (ptr), _end (ptr + offset) {
        }

        char *&operator++ () // prefix
        {
          checkBounds (_cur + 1);
          return ++_cur;
        }

        char *operator++ (int) // postfix
        {
          checkBounds (_cur + 1);
          char *tmp = _cur++;
          return tmp;
        }

        char *&operator-- () // prefix
        {
          checkBounds (_cur - 1);
          return --_cur;
        }

        char *operator-- (int) // postfix
        {
          checkBounds (_cur - 1);
          char *tmp = _cur--;
          return tmp;
        }

        char *&operator+= (int incr) {
          checkBounds (_cur + incr);
          return _cur += incr;
        }

        char *&operator-= (int decr) {
          checkBounds (_cur - decr);
          return _cur -= decr;
        }

        operator char * () const {
          return _cur;
        }

        [[nodiscard]] std::size_t span () const {
          return _end - _beg;
        }

      private:
        void checkBounds (const char *ptr) {
          if (ptr > _end)
            RAISE_EX("RangeException");
        }

        const char *_beg;
        char *_cur;
        const char *_end;
    };

  } // namespace Impl


  template <typename T>
  bool int_to_str (T value,
                   unsigned short base,
                   char *result,
                   std::size_t &size,
                   bool prefix = false,
                   int width = -1,
                   char fill = ' ',
                   char thSep = 0)
  /// Converts integer to string. Numeric bases from binary to hexadecimal are supported.
  /// If width is non-zero, it pads the return value with fill character to the specified width.
  /// When padding is zero character ('0'), it is prepended to the number itself; all other
  /// paddings are prepended to the formatted result with minus sign or base prefix included
  /// If prefix is true and base is octal or hexadecimal, respective prefix ('0' for octal,
  /// "0x" for hexadecimal) is prepended. For all other bases, prefix argument is ignored.
  /// Formatted string has at least [width] total length.
  {
    if (base < 2 || base > 0x10) {
      *result = '\0';
      return false;
    }

    impl::ptr ptr (result, size);
    int thCount = 0;
    T tmpVal;
    do {
      tmpVal = value;
      value /= base;
      *ptr++ = "FEDCBA9876543210123456789ABCDEF"[15 + (tmpVal - value * base)];
      if (thSep && (base == 10) && (++thCount == 3)) {
        *ptr++ = thSep;
        thCount = 0;
      }
    }
    while (value);

    if ('0' == fill) {
      if (tmpVal < 0) {
        --width;
      }
      if (prefix && base == 010) {
        --width;
      }
      if (prefix && base == 0x10) {
        width -= 2;
      }
      while ((ptr - result) < width) {
        *ptr++ = fill;
      }
    }

    if (prefix && base == 010) {
      *ptr++ = '0';
    }
    else {
      if (prefix && base == 0x10) {
        *ptr++ = 'x';
        *ptr++ = '0';
      }
    }

    if (tmpVal < 0) {
      *ptr++ = '-';
    }

    if ('0' != fill) {
      while ((ptr - result) < width) {
        *ptr++ = fill;
      }
    }

    size = ptr - result;
    ENFORCE (size <= ptr.span ());
    ENFORCE ((-1 == width) || (size >= size_t (width)));
    *ptr-- = '\0';

    char *ptrr = result;
    char tmp;
    while (ptrr < ptr) {
      tmp = *ptr;
      *ptr-- = *ptrr;
      *ptrr++ = tmp;
    }

    return true;
  }

  template <typename T>
  bool uint_to_str (T value,
                    unsigned short base,
                    char *result,
                    std::size_t &size,
                    bool prefix = false,
                    int width = -1,
                    char fill = ' ',
                    char thSep = 0)
  /// Converts unsigned integer to string. Numeric bases from binary to hexadecimal are supported.
  /// If width is non-zero, it pads the return value with fill character to the specified width.
  /// When padding is zero character ('0'), it is prepended to the number itself; all other
  /// paddings are prepended to the formatted result with minus sign or base prefix included
  /// If prefix is true and base is octal or hexadecimal, respective prefix ('0' for octal,
  /// "0x" for hexadecimal) is prepended. For all other bases, prefix argument is ignored.
  /// Formatted string has at least [width] total length.
  {
    if (base < 2 || base > 0x10) {
      *result = '\0';
      return false;
    }

    impl::ptr ptr (result, size);
    int thCount = 0;
    T tmpVal;
    do {
      tmpVal = value;
      value /= base;
      *ptr++ = "FEDCBA9876543210123456789ABCDEF"[15 + (tmpVal - value * base)];
      if (thSep && (base == 10) && (++thCount == 3)) {
        *ptr++ = thSep;
        thCount = 0;
      }
    }
    while (value);

    if ('0' == fill) {
      if (prefix && base == 010) {
        --width;
      }
      if (prefix && base == 0x10) {
        width -= 2;
      }
      while ((ptr - result) < width) {
        *ptr++ = fill;
      }
    }

    if (prefix && base == 010) {
      *ptr++ = '0';
    }
    else {
      if (prefix && base == 0x10) {
        *ptr++ = 'x';
        *ptr++ = '0';
      }
    }

    if ('0' != fill) {
      while ((ptr - result) < width) {
        *ptr++ = fill;
      }
    }

    size = ptr - result;
    ENFORCE (size <= ptr.span ());
    ENFORCE ((-1 == width) || (size >= size_t (width)));
    *ptr-- = '\0';

    char *ptrr = result;
    char tmp;
    while (ptrr < ptr) {
      tmp = *ptr;
      *ptr-- = *ptrr;
      *ptrr++ = tmp;
    }

    return true;
  }

  template <typename T>
  bool
  int_to_str (T number, unsigned short base, std::string &result, bool prefix = false, int width = -1, char fill = ' ',
              char thSep = 0)
  /// Converts integer to string; This is a wrapper function, for details see see the
  /// bool int_to_str(T, unsigned short, char*, int, int, char, char) implementation.
  {
    char res[NEUTRINO_MAX_INT_STRING_LEN] = {0};
    std::size_t size = NEUTRINO_MAX_INT_STRING_LEN;
    bool ret = int_to_str (number, base, res, size, prefix, width, fill, thSep);
    result.assign (res, size);
    return ret;
  }

  template <typename T>
  bool uint_to_str (T number, unsigned short base, std::string &result, bool prefix = false, int width = -1,
                    char fill = ' ', char thSep = 0)
  /// Converts unsigned integer to string; This is a wrapper function, for details see see the
  /// bool uint_to_str(T, unsigned short, char*, int, int, char, char) implementation.
  {
    char res[NEUTRINO_MAX_INT_STRING_LEN] = {0};
    std::size_t size = NEUTRINO_MAX_INT_STRING_LEN;
    bool ret = uint_to_str (number, base, res, size, prefix, width, fill, thSep);
    result.assign (res, size);
    return ret;
  }


//
// Wrappers for double-conversion library (http://code.google.com/p/double-conversion/).
//
// Library is the implementation of the algorithm described in Florian Loitsch's paper:
// http://florian.loitsch.com/publications/dtoa-pldi2010.pdf
//


  void float_to_str (char *buffer,
                     int bufferSize,
                     float value,
                     int lowDec = -std::numeric_limits<float>::digits10,
                     int highDec = std::numeric_limits<float>::digits10);
  /// Converts a float value to string. Converted string must be shorter than bufferSize.
  /// Conversion is done by computing the shortest string of digits that correctly represents
  /// the input number. Depending on lowDec and highDec values, the function returns
  /// decimal or exponential representation.

  void float_to_fixed_str (char *buffer,
                           int bufferSize,
                           float value,
                           int precision);
  /// Converts a float value to string. Converted string must be shorter than bufferSize.
  /// Computes a decimal representation with a fixed number of digits after the
  /// decimal point.


  std::string &float_to_str (std::string &str,
                             float value,
                             int precision = -1,
                             int width = 0,
                             char thSep = 0,
                             char decSep = 0);
  /// Converts a float value, assigns it to the supplied string and returns the reference.
  /// This function calls float_to_str(char*, int, float, int, int) and formats the result according to
  /// precision (total number of digits after the decimal point, -1 means ignore precision argument)
  /// and width (total length of formatted string).


  std::string &float_to_fixed_str (std::string &str,
                                   float value,
                                   int precision,
                                   int width = 0,
                                   char thSep = 0,
                                   char decSep = 0);
  /// Converts a float value, assigns it to the supplied string and returns the reference.
  /// This function calls float_to_fixed_str(char*, int, float, int) and formats the result according to
  /// precision (total number of digits after the decimal point) and width (total length of formatted string).


  void double_to_str (char *buffer,
                      int bufferSize,
                      double value,
                      int lowDec = -std::numeric_limits<double>::digits10,
                      int highDec = std::numeric_limits<double>::digits10);
  /// Converts a double value to string. Converted string must be shorter than bufferSize.
  /// Conversion is done by computing the shortest string of digits that correctly represents
  /// the input number. Depending on lowDec and highDec values, the function returns
  /// decimal or exponential representation.


  void double_to_fixed_str (char *buffer,
                            int bufferSize,
                            double value,
                            int precision);
  /// Converts a double value to string. Converted string must be shorter than bufferSize.
  /// Computes a decimal representation with a fixed number of digits after the
  /// decimal point.


  std::string &double_to_str (std::string &str,
                              double value,
                              int precision = -1,
                              int width = 0,
                              char thSep = 0,
                              char decSep = 0);
  /// Converts a double value, assigns it to the supplied string and returns the reference.
  /// This function calls double_to_str(char*, int, double, int, int) and formats the result according to
  /// precision (total number of digits after the decimal point, -1 means ignore precision argument)
  /// and width (total length of formatted string).


  std::string &double_to_fixed_str (std::string &str,
                                    double value,
                                    int precision = -1,
                                    int width = 0,
                                    char thSep = 0,
                                    char decSep = 0);
  /// Converts a double value, assigns it to the supplied string and returns the reference.
  /// This function calls double_to_fixed_str(char*, int, double, int) and formats the result according to
  /// precision (total number of digits after the decimal point) and width (total length of formatted string).


  float str_to_float (const char *str,
                      const char *inf = NEUTRINO_FLT_INF, const char *nan = NEUTRINO_FLT_NAN);
  /// Converts the string of characters into single-precision floating point number.
  /// Function uses double_conversion::DoubleToStringConverter to do the conversion.


  bool str_to_float (const std::string &, float &result,
                     char decSep = '.', char thSep = ',',
                     const char *inf = NEUTRINO_FLT_INF, const char *nan = NEUTRINO_FLT_NAN);
  /// Converts the string of characters into single-precision floating point number.
  /// The conversion result is assigned to the result parameter.
  /// If decimal separator and/or thousand separator are different from defaults, they should be
  /// supplied to ensure proper conversion.
  ///
  /// Returns true if successful, false otherwise.


  double str_to_double (const char *str,
                        const char *inf = NEUTRINO_FLT_INF, const char *nan = NEUTRINO_FLT_NAN);
  /// Converts the string of characters into double-precision floating point number.


  bool str_to_double (const std::string &str, double &result,
                      char decSep = '.', char thSep = ',',
                      const char *inf = NEUTRINO_FLT_INF, const char *nan = NEUTRINO_FLT_NAN);
  /// Converts the string of characters into double-precision floating point number.
  /// The conversion result is assigned to the result parameter.
  /// If decimal separator and/or thousand separator are different from defaults, they should be
  /// supplied to ensure proper conversion.
  ///
  /// Returns true if successful, false otherwise.

}

#endif

