//
// Created by igor on 25/07/2021.
//

#include <neutrino/utils/strings/number_parser.hh>
#include <neutrino/utils/strings/string_utils.hh>
#include <neutrino/utils/strings/numeric_string.hh>
#include <neutrino/utils/exception.hh>
#include <cstdio>
#include <cctype>

#if !defined(POCO_NO_LOCALE)
#include <locale>
#endif


#if defined(POCO_LONG_IS_64_BIT)
#define I64_FMT "l"
#elif defined(_MSC_VER) || defined(__MINGW32__)
#define I64_FMT "I64"
#elif defined(__APPLE__)
#define I64_FMT "q"
#else
#define I64_FMT "ll"
#endif
namespace neutrino::utils
{
    int number_parser::parse(const std::string& s, char thSep)
    {
        int result;
        if (try_parse(s, result, thSep))
        {
            return result;
        } else
            RAISE_EX("Not a valid integer", s);
    }

    bool number_parser::try_parse(const std::string& s, int& value, char thousandSeparator)
    {
        return str_to_int(s.c_str(), value, NUM_BASE_DEC, thousandSeparator);
    }

    unsigned number_parser::parse_unsigned(const std::string& s, char thousandSeparator)
    {
        unsigned result;
        if (try_parse_unsigned(s, result, thousandSeparator))
        {
            return result;
        } else
            RAISE_EX("Not a valid unsigned integer", s);
    }

    bool number_parser::try_parse_unsigned(const std::string& s, unsigned& value, char thousandSeparator)
    {
        return str_to_int(s.c_str(), value, NUM_BASE_DEC, thousandSeparator);
    }

    unsigned number_parser::parse_hex(const std::string& s)
    {
        unsigned result;
        if (try_parse_hex(s, result))
        {
            return result;
        } else
            RAISE_EX("Not a valid hexadecimal integer", s);
    }

    bool number_parser::try_parse_hex(const std::string& s, unsigned& value)
    {
        int offset = 0;
        if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        { offset = 2; }
        return str_to_int(s.c_str() + offset, value, NUM_BASE_HEX);
    }

    unsigned number_parser::parseOct(const std::string& s)
    {
        unsigned result;
        if (try_parse_oct(s, result))
        {
            return result;
        } else
            RAISE_EX("Not a valid hexadecimal integer", s);
    }

    bool number_parser::try_parse_oct(const std::string& s, unsigned& value)
    {
        return str_to_int(s.c_str(), value, NUM_BASE_OCT);
    }

    int64_t number_parser::parse64(const std::string& s, char thSep)
    {
        int64_t result;
        if (try_parse_64(s, result, thSep))
        {
            return result;
        } else {
            RAISE_EX("Not a valid integer", s);
        }
    }

    bool number_parser::try_parse_64(const std::string& s, int64_t& value, char thousandSeparator)
    {
        return str_to_int(s.c_str(), value, NUM_BASE_DEC, thousandSeparator);
    }

    uint64_t number_parser::parse_unsigned_64(const std::string& s, char thousandSeparator)
    {
        uint64_t result;
        if (try_parse_unsigned_64(s, result, thousandSeparator))
        {
            return result;
        } else
            RAISE_EX("Not a valid unsigned integer", s);
    }

    bool number_parser::try_parse_unsigned_64(const std::string& s, uint64_t& value, char thousandSeparator)
    {
        return str_to_int(s.c_str(), value, NUM_BASE_DEC, thousandSeparator);
    }

    uint64_t number_parser::parse_hex_64(const std::string& s)
    {
        uint64_t result;
        if (try_parse_hex_64(s, result))
        {
            return result;
        } else
            RAISE_EX("Not a valid hexadecimal integer", s);
    }

    bool number_parser::try_parse_hex_64(const std::string& s, uint64_t& value)
    {
        int offset = 0;
        if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        { offset = 2; }
        return str_to_int(s.c_str() + offset, value, NUM_BASE_HEX);
    }

    uint64_t number_parser::parse_oct_64(const std::string& s)
    {
        uint64_t result;
        if (try_parse_oct_64(s, result))
        {
            return result;
        } else
            RAISE_EX("Not a valid hexadecimal integer", s);
    }

    bool number_parser::try_parse_oct_64(const std::string& s, uint64_t& value)
    {
        return str_to_int(s.c_str(), value, NUM_BASE_OCT);
    }

    double number_parser::parse_float(const std::string& s, char decimalSeparator, char thousandSeparator)
    {
        double result;
        if (try_parse_float(s, result, decimalSeparator, thousandSeparator))
        {
            return result;
        } else
            RAISE_EX("Not a valid floating-point number", s);
    }

    bool number_parser::try_parse_float(const std::string& s, double& value, char decimalSeparator, char thousandSeparator)
    {
        return str_to_double(s.c_str(), value, decimalSeparator, thousandSeparator);
    }

    bool number_parser::parse_bool(const std::string& s)
    {
        bool result;
        if (try_parse_bool(s, result))
        {
            return result;
        } else
            RAISE_EX("Not a valid bool number", s);
    }

    bool number_parser::try_parse_bool(const std::string& s, bool& value)
    {
        int n;
        if (number_parser::try_parse(s, n))
        {
            value = (n != 0);
            return true;
        }

        if (icompare(s, "true") == 0)
        {
            value = true;
            return true;
        } else
        {
            if (icompare(s, "yes") == 0)
            {
                value = true;
                return true;
            } else
            {
                if (icompare(s, "on") == 0)
                {
                    value = true;
                    return true;
                }
            }
        }

        if (icompare(s, "false") == 0)
        {
            value = false;
            return true;
        } else
        {
            if (icompare(s, "no") == 0)
            {
                value = false;
                return true;
            } else
            {
                if (icompare(s, "off") == 0)
                {
                    value = false;
                    return true;
                }
            }
        }

        return false;
    }

}