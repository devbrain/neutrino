//
// Created by igor on 25/07/2021.
//

#include <neutrino/utils/strings/number_formatter.hh>
#include <iomanip>

#if !defined(POCO_NO_LOCALE)

#include <locale>

#endif

#include <cstdio>
#include <cinttypes>


#if defined(_MSC_VER) || defined(__MINGW32__)
#define I64_FMT "I64"
#elif defined(__APPLE__)
#define I64_FMT "q"
#else
#define I64_FMT "ll"
#endif

namespace neutrino::utils
{
    std::string number_formatter::format(bool value, BoolFormat format)
    {
        switch (format)
        {
            default:
            case FMT_TRUE_FALSE:
                if (value == true)
                {
                    return "true";
                }
                return "false";
            case FMT_YES_NO:
                if (value == true)
                {
                    return "yes";
                }
                return "no";
            case FMT_ON_OFF:
                if (value == true)
                {
                    return "on";
                }
                return "off";
        }
    }

    void number_formatter::append(std::string& str, int value)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        int_to_str(value, 10, result, sz);
        str.append(result, sz);
    }

    void number_formatter::append(std::string& str, int value, int width)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        int_to_str(value, 10, result, sz, false, width);
        str.append(result, sz);
    }

    void number_formatter::append0(std::string& str, int value, int width)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        int_to_str(value, 10, result, sz, false, width, '0');
        str.append(result, sz);
    }

    void number_formatter::appendHex(std::string& str, int value)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        uint_to_str(static_cast<unsigned int>(value), 0x10, result, sz);
        str.append(result, sz);
    }

    void number_formatter::appendHex(std::string& str, int value, int width)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        uint_to_str(static_cast<unsigned int>(value), 0x10, result, sz, false, width, '0');
        str.append(result, sz);
    }

    void number_formatter::append(std::string& str, unsigned value)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        uint_to_str(value, 10, result, sz);
        str.append(result, sz);
    }

    void number_formatter::append(std::string& str, unsigned value, int width)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        uint_to_str(value, 10, result, sz, false, width);
        str.append(result, sz);
    }

    void number_formatter::append0(std::string& str, unsigned int value, int width)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        uint_to_str(value, 10, result, sz, false, width, '0');
        str.append(result, sz);
    }

    void number_formatter::appendHex(std::string& str, unsigned value)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        uint_to_str(value, 0x10, result, sz);
        str.append(result, sz);
    }

    void number_formatter::appendHex(std::string& str, unsigned value, int width)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        uint_to_str(value, 0x10, result, sz, false, width, '0');
        str.append(result, sz);
    }

    void number_formatter::append(std::string& str, long value)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        int_to_str(value, 10, result, sz);
        str.append(result, sz);
    }

    void number_formatter::append(std::string& str, long value, int width)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        int_to_str(value, 10, result, sz, false, width);
        str.append(result, sz);
    }

    void number_formatter::append0(std::string& str, long value, int width)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        int_to_str(value, 10, result, sz, false, width, '0');
        str.append(result, sz);
    }

    void number_formatter::append_hex(std::string& str, long value)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        uint_to_str(static_cast<unsigned long>(value), 0x10, result, sz);
        str.append(result, sz);
    }

    void number_formatter::append_hex(std::string& str, long value, int width)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        uint_to_str(static_cast<unsigned long>(value), 0x10, result, sz, false, width, '0');
        str.append(result, sz);
    }

    void number_formatter::append(std::string& str, unsigned long value)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        uint_to_str(value, 10, result, sz);
        str.append(result, sz);
    }

    void number_formatter::append(std::string& str, unsigned long value, int width)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        uint_to_str(value, 10, result, sz, false, width, '0');
        str.append(result, sz);
    }

    void number_formatter::append0(std::string& str, unsigned long value, int width)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        uint_to_str(value, 10, result, sz, false, width, '0');
        str.append(result, sz);
    }

    void number_formatter::append_hex(std::string& str, unsigned long value)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        uint_to_str(value, 0x10, result, sz);
        str.append(result, sz);
    }

    void number_formatter::append_hex(std::string& str, unsigned long value, int width)
    {
        char result[NF_MAX_INT_STRING_LEN];
        std::size_t sz = NF_MAX_INT_STRING_LEN;
        uint_to_str(value, 0x10, result, sz, false, width, '0');
        str.append(result, sz);
    }

    void number_formatter::append(std::string& str, float value)
    {
        char buffer[NF_MAX_FLT_STRING_LEN];
        float_to_str(buffer, NEUTRINO_MAX_FLT_STRING_LEN, value);
        str.append(buffer);
    }

    void number_formatter::append(std::string& str, float value, int precision)
    {
        char buffer[NF_MAX_FLT_STRING_LEN];
        float_to_fixed_str(buffer, NEUTRINO_MAX_FLT_STRING_LEN, value, precision);
        str.append(buffer);
    }

    void number_formatter::append(std::string& str, float value, int width, int precision)
    {
        std::string result;
        str.append(float_to_fixed_str(result, value, precision, width));
    }

    void number_formatter::append(std::string& str, double value)
    {
        char buffer[NF_MAX_FLT_STRING_LEN];
        double_to_str(buffer, NEUTRINO_MAX_FLT_STRING_LEN, value);
        str.append(buffer);
    }

    void number_formatter::append(std::string& str, double value, int precision)
    {
        char buffer[NF_MAX_FLT_STRING_LEN];
        double_to_fixed_str(buffer, NEUTRINO_MAX_FLT_STRING_LEN, value, precision);
        str.append(buffer);
    }

    void number_formatter::append(std::string& str, double value, int width, int precision)
    {
        std::string result;
        str.append(double_to_fixed_str(result, value, precision, width));
    }

    void number_formatter::append(std::string& str, const void* ptr)
    {
        char buffer[24];
        if constexpr(sizeof(void*) == 8)
        {
            std::sprintf(buffer, "%016" PRIXPTR, (uintptr_t) ptr);
        } else
        {
            std::sprintf(buffer, "%08" PRIXPTR, (uintptr_t) ptr);
        }

        str.append(buffer);
    }
}