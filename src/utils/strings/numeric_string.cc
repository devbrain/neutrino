//
// Created by igor on 25/07/2021.
//


#include <neutrino/utils/strings/numeric_string.hh>
#include <neutrino/utils/exception.hh>
#include <neutrino/utils/strings/string_utils.hh>
#include <utils/fpe/double-conversion/double-conversion.h>
#include <utils/fpe/double-conversion/ieee.h>

#define UNIMPLEMENTED RAISE_EX("Unimplemented")

#if defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#endif

namespace
{
    void pad(std::string& str, int precision, int width, char prefix = ' ', char decSep = '.')
    /// Pads the string with prefix space and postfix 0.
    /// Alternative prefix (e.g. zero instead of space) can be supplied by caller.
    /// Used only internally.
    {
        // these cases should never happen, if they do, it's a library bug
        ENFORCE (precision > 0);
        ENFORCE (str.length());

        std::string::size_type decSepPos = str.find(decSep);
        if (decSepPos == std::string::npos)
        {
            str.append(1, '.');
            decSepPos = str.size() - 1;
        }

        std::string::size_type frac = str.length() - decSepPos - 1;

        std::string::size_type ePos = str.find_first_of("eE");
        std::unique_ptr<std::string> eStr;
        if (ePos != std::string::npos)
        {
            eStr.reset(new std::string(str.substr(ePos, std::string::npos)));
            frac -= eStr->length();
            str = str.substr(0, str.length() - eStr->length());
        }

        if (frac != precision)
        {
            if (frac < precision)
            {
                str.append(precision - frac, '0');
            } else
            {
                if ((frac > precision) && (decSepPos != std::string::npos))
                {
                    int pos = static_cast<int>(decSepPos) + 1 + precision;
                    if (str[pos] >= '5') // we must round up
                    {
                        char carry = 0;
                        if (str[--pos] == '9')
                        {
                            str[pos] = '0';
                            carry = 1;
                        } else
                        {
                            ++str[pos];
                            carry = 0;
                        }
                        while (--pos >= 0)
                        {
                            if (str[pos] == decSep)
                            { continue; }
                            if (carry)
                            {
                                if ((str[pos] + carry) <= '9')
                                {
                                    ++str[pos];
                                    carry = 0;
                                } else
                                {
                                    str[pos] = '0';
                                    carry = 1;
                                }
                            }
                        }
                        if (carry)
                        { str.insert(str.begin(), 1, '1'); }
                    }
                    str = str.substr(0, decSepPos + 1 + precision);
                }
            }
        }

        if (eStr)
        { str += *eStr; }

        if (width && (str.length() < width))
        { str.insert(str.begin(), width - str.length(), prefix); }
    }

    void insertThousandSep(std::string& str, char thSep, char decSep = '.')
    /// Inserts thousand separators.
    /// Used only internally.
    {
        ENFORCE (decSep != thSep);
        if (str.empty())
        { return; }

        std::string::size_type exPos = str.find('e');
        if (exPos == std::string::npos)
        { exPos = str.find('E'); }
        std::string::size_type decPos = str.find(decSep);
        // there's no rinsert, using forward iterator to go backwards
        std::string::iterator it = str.end();
        if (exPos != std::string::npos)
        { it -= str.size() - exPos; }

        if (decPos != std::string::npos)
        {
            while (it != str.begin())
            {
                --it;
                if (*it == decSep)
                { break; }
            }
        }
        int thCount = 0;
        if (it == str.end())
        { --it; }
        for (; it != str.begin();)
        {
            std::string::iterator pos = it;
            std::string::value_type chr = *it;
            std::string::value_type prevChr = *--it;

            if (!std::isdigit(chr))
            { continue; }

            if (++thCount == 3 && std::isdigit(prevChr))
            {
                it = str.insert(pos, thSep);
            }

            if (thCount == 3)
            { thCount = 0; }
        }
    }


} // namespace


namespace neutrino::utils
{


    void float_to_str(char* buffer, int bufferSize, float value, int lowDec, int highDec)
    {
        using namespace double_conversion;

        StringBuilder builder(buffer, bufferSize);
        int flags = DoubleToStringConverter::UNIQUE_ZERO |
                    DoubleToStringConverter::EMIT_POSITIVE_EXPONENT_SIGN;
        DoubleToStringConverter dc(flags, NEUTRINO_FLT_INF, NEUTRINO_FLT_NAN, NEUTRINO_FLT_EXP, lowDec, highDec, 0, 0);
        dc.ToShortestSingle(value, &builder);
        builder.Finalize();
    }

    void float_to_fixed_str(char* buffer, int bufferSize, float value, int precision)
    {
        using namespace double_conversion;

        StringBuilder builder(buffer, bufferSize);
        int flags = DoubleToStringConverter::UNIQUE_ZERO |
                    DoubleToStringConverter::EMIT_POSITIVE_EXPONENT_SIGN;
        DoubleToStringConverter dc(flags, NEUTRINO_FLT_INF, NEUTRINO_FLT_NAN, NEUTRINO_FLT_EXP,
                                   -std::numeric_limits<float>::digits10, std::numeric_limits<float>::digits10, 0, 0);
        dc.ToFixed(value, precision, &builder);
        builder.Finalize();
    }

    std::string& float_to_str(std::string& str, float value, int precision, int width, char thSep, char decSep)
    {
        if (!decSep)
        { decSep = '.'; }
        if (precision == 0)
        { value = std::floor(value); }

        char buffer[NEUTRINO_MAX_FLT_STRING_LEN];
        float_to_str(buffer, NEUTRINO_MAX_FLT_STRING_LEN, value);
        str = buffer;

        if (decSep && (decSep != '.') && (str.find('.') != std::string::npos))
        {
            replace_in_place(str, '.', decSep);
        }

        if (thSep)
        { insertThousandSep(str, thSep, decSep); }
        if (precision > 0 || width)
        { pad(str, precision, width, ' ', decSep ? decSep : '.'); }
        return str;
    }

    std::string& float_to_fixed_str(std::string& str, float value, int precision, int width, char thSep, char decSep)
    {
        if (!decSep)
        { decSep = '.'; }
        if (precision == 0)
        { value = std::floor(value); }

        char buffer[NEUTRINO_MAX_FLT_STRING_LEN];
        float_to_fixed_str(buffer, NEUTRINO_MAX_FLT_STRING_LEN, value, precision);
        str = buffer;

        if (decSep && (decSep != '.') && (str.find('.') != std::string::npos))
        {
            replace_in_place(str, '.', decSep);
        }

        if (thSep)
        { insertThousandSep(str, thSep, decSep); }
        if (precision > 0 || width)
        { pad(str, precision, width, ' ', decSep ? decSep : '.'); }
        return str;
    }

    void double_to_str(char* buffer, int bufferSize, double value, int lowDec, int highDec)
    {
        using namespace double_conversion;

        StringBuilder builder(buffer, bufferSize);
        int flags = DoubleToStringConverter::UNIQUE_ZERO |
                    DoubleToStringConverter::EMIT_POSITIVE_EXPONENT_SIGN;
        DoubleToStringConverter dc(flags, NEUTRINO_FLT_INF, NEUTRINO_FLT_NAN, NEUTRINO_FLT_EXP, lowDec, highDec, 0, 0);
        dc.ToShortest(value, &builder);
        builder.Finalize();
    }

    void double_to_fixed_str(char* buffer, int bufferSize, double value, int precision)
    {
        using namespace double_conversion;

        StringBuilder builder(buffer, bufferSize);
        int flags = DoubleToStringConverter::UNIQUE_ZERO |
                    DoubleToStringConverter::EMIT_POSITIVE_EXPONENT_SIGN;
        DoubleToStringConverter dc(flags, NEUTRINO_FLT_INF, NEUTRINO_FLT_NAN, NEUTRINO_FLT_EXP,
                                   -std::numeric_limits<double>::digits10, std::numeric_limits<double>::digits10, 0, 0);
        dc.ToFixed(value, precision, &builder);
        builder.Finalize();
    }

    std::string& double_to_str(std::string& str, double value, int precision, int width, char thSep, char decSep)
    {
        if (!decSep)
        { decSep = '.'; }
        if (precision == 0)
        { value = std::floor(value); }

        char buffer[NEUTRINO_MAX_FLT_STRING_LEN];
        double_to_str(buffer, NEUTRINO_MAX_FLT_STRING_LEN, value);

        str = buffer;

        if (decSep && (decSep != '.') && (str.find('.') != std::string::npos))
        {
            replace_in_place(str, '.', decSep);
        }

        if (thSep)
        { insertThousandSep(str, thSep, decSep); }
        if (precision > 0 || width)
        { pad(str, precision, width, ' ', decSep ? decSep : '.'); }
        return str;
    }

    std::string& double_to_fixed_str(std::string& str, double value, int precision, int width, char thSep, char decSep)
    {
        if (!decSep)
        { decSep = '.'; }
        if (precision == 0)
        { value = std::floor(value); }

        char buffer[NEUTRINO_MAX_FLT_STRING_LEN];
        double_to_fixed_str(buffer, NEUTRINO_MAX_FLT_STRING_LEN, value, precision);

        str = buffer;

        if (decSep && (decSep != '.') && (str.find('.') != std::string::npos))
        {
            replace_in_place(str, '.', decSep);
        }

        if (thSep)
        { insertThousandSep(str, thSep, decSep); }
        if (precision > 0 || width)
        { pad(str, precision, width, ' ', decSep ? decSep : '.'); }
        return str;
    }

    float str_to_float(const char* str, const char* inf, const char* nan)
    {
        using namespace double_conversion;

        int processed;
        int flags = StringToDoubleConverter::ALLOW_LEADING_SPACES |
                    StringToDoubleConverter::ALLOW_TRAILING_SPACES;
        StringToDoubleConverter converter(flags, 0.0, Single::NaN(), inf, nan);
        float result = converter.StringToFloat(str, static_cast<int>(strlen(str)), &processed);
        return result;
    }

    double str_to_double(const char* str, const char* inf, const char* nan)
    {
        using namespace double_conversion;
        int processed;
        int flags = StringToDoubleConverter::ALLOW_LEADING_SPACES |
                    StringToDoubleConverter::ALLOW_TRAILING_SPACES;
        StringToDoubleConverter converter(flags, 0.0, Double::NaN(), inf, nan);
        double result = converter.StringToDouble(str, static_cast<int>(strlen(str)), &processed);
        return result;
    }

    bool str_to_float(const std::string& str, float& result, char decSep, char thSep, const char* inf, const char* nan)
    {
        using namespace double_conversion;

        std::string tmp(str);
        trim_in_place(tmp);
        remove_in_place(tmp, thSep);
        remove_in_place(tmp, 'f');
        replace_in_place(tmp, decSep, '.');
        result = str_to_float(tmp.c_str(), inf, nan);
        return !fpe::is_infinite(result) &&
               !fpe::is_NaN(result);
    }

    bool
    str_to_double(const std::string& str, double& result, char decSep, char thSep, const char* inf, const char* nan)
    {
        if (str.empty())
        { return false; }

        using namespace double_conversion;

        std::string tmp(str);
        trim_in_place(tmp);
        remove_in_place(tmp, thSep);
        replace_in_place(tmp, decSep, '.');
        remove_in_place(tmp, 'f');
        result = str_to_double(tmp.c_str(), inf, nan);
        return !fpe::is_infinite(result) &&
               !fpe::is_NaN(result);
    }
}
