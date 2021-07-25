//
// Created by igor on 25/07/2021.
//

#include <doctest/doctest.h>
#include <neutrino/utils/strings/number_formatter.hh>

using namespace neutrino::utils;

TEST_SUITE("NumberFormatter")
{
    TEST_CASE("format")
    {
        REQUIRE (number_formatter::format(123) == "123");
        REQUIRE (number_formatter::format(-123) == "-123");
        REQUIRE (number_formatter::format(-123, 5) == " -123");

        REQUIRE (number_formatter::format((unsigned) 123) == "123");
        REQUIRE (number_formatter::format((unsigned) 123, 5) == "  123");
        REQUIRE (number_formatter::format0((unsigned) 123, 5) == "00123");

        REQUIRE (number_formatter::format((long) 123) == "123");
        REQUIRE (number_formatter::format((long) -123) == "-123");
        REQUIRE (number_formatter::format((long) -123, 5) == " -123");

        REQUIRE (number_formatter::format((unsigned long) 123) == "123");
        REQUIRE (number_formatter::format((unsigned long) 123, 5) == "  123");

        REQUIRE (number_formatter::format(123) == "123");
        REQUIRE (number_formatter::format(-123) == "-123");
        REQUIRE (number_formatter::format(-123, 5) == " -123");


        REQUIRE (number_formatter::format((int64_t) 123) == "123");
        REQUIRE (number_formatter::format((int64_t) -123) == "-123");
        REQUIRE (number_formatter::format((int64_t) -123, 5) == " -123");

        REQUIRE (number_formatter::format((uint64_t) 123) == "123");
        REQUIRE (number_formatter::format((uint64_t) 123, 5) == "  123");


        if (sizeof(void*) == 4)
        {
            REQUIRE (number_formatter::format((void*) 0x12345678) == "12345678");
        } else
        {
            REQUIRE (number_formatter::format((void*) 0x12345678) == "0000000012345678");
        }
    }

    TEST_CASE("format0")
    {
        REQUIRE (number_formatter::format0(123, 5) == "00123");
        REQUIRE (number_formatter::format0(-123, 5) == "-0123");
        REQUIRE (number_formatter::format0((long) 123, 5) == "00123");
        REQUIRE (number_formatter::format0((long) -123, 5) == "-0123");
        REQUIRE (number_formatter::format0((unsigned long) 123, 5) == "00123");


        REQUIRE (number_formatter::format0((int64_t) 123, 5) == "00123");
        REQUIRE (number_formatter::format0((int64_t) -123, 5) == "-0123");
        REQUIRE (number_formatter::format0((uint64_t) 123, 5) == "00123");

    }

    TEST_CASE("format bool")
    {
        REQUIRE (number_formatter::format(true, number_formatter::FMT_TRUE_FALSE) == "true");
        REQUIRE (number_formatter::format(false, number_formatter::FMT_TRUE_FALSE) == "false");
        REQUIRE (number_formatter::format(true, number_formatter::FMT_YES_NO) == "yes");
        REQUIRE (number_formatter::format(false, number_formatter::FMT_YES_NO) == "no");
        REQUIRE (number_formatter::format(true, number_formatter::FMT_ON_OFF) == "on");
        REQUIRE (number_formatter::format(false, number_formatter::FMT_ON_OFF) == "off");
    }

    TEST_CASE("format hex")
    {
        REQUIRE (number_formatter::format_hex(0x12) == "12");
        REQUIRE (number_formatter::format_hex(0xab) == "AB");
        REQUIRE (number_formatter::format_hex(0x12, 4) == "0012");
        REQUIRE (number_formatter::format_hex(0xab, 4) == "00AB");

        REQUIRE (number_formatter::format_hex((unsigned) 0x12) == "12");
        REQUIRE (number_formatter::format_hex((unsigned) 0xab) == "AB");
        REQUIRE (number_formatter::format_hex((unsigned) 0x12, 4) == "0012");
        REQUIRE (number_formatter::format_hex((unsigned) 0xab, 4) == "00AB");

        REQUIRE (number_formatter::format_hex((long) 0x12) == "12");
        REQUIRE (number_formatter::format_hex((long) 0xab) == "AB");
        REQUIRE (number_formatter::format_hex((long) 0x12, 4) == "0012");
        REQUIRE (number_formatter::format_hex((long) 0xab, 4) == "00AB");

        REQUIRE (number_formatter::format_hex((unsigned long) 0x12) == "12");
        REQUIRE (number_formatter::format_hex((unsigned long) 0xab) == "AB");
        REQUIRE (number_formatter::format_hex((unsigned long) 0x12, 4) == "0012");
        REQUIRE (number_formatter::format_hex((unsigned long) 0xab, 4) == "00AB");


        REQUIRE (number_formatter::format_hex((int64_t) 0x12) == "12");
        REQUIRE (number_formatter::format_hex((int64_t) 0xab) == "AB");
        REQUIRE (number_formatter::format_hex((int64_t) 0x12, 4) == "0012");
        REQUIRE (number_formatter::format_hex((int64_t) 0xab, 4) == "00AB");

        REQUIRE (number_formatter::format_hex((uint64_t) 0x12) == "12");
        REQUIRE (number_formatter::format_hex((uint64_t) 0xab) == "AB");
        REQUIRE (number_formatter::format_hex((uint64_t) 0x12, 4) == "0012");
        REQUIRE (number_formatter::format_hex((uint64_t) 0xab, 4) == "00AB");

        REQUIRE (number_formatter::format_hex(0x12, true) == "0x12");
        REQUIRE (number_formatter::format_hex(0xab, true) == "0xAB");
        REQUIRE (number_formatter::format_hex(0x12, 4, true) == "0x12");
        REQUIRE (number_formatter::format_hex(0xab, 4, true) == "0xAB");
        REQUIRE (number_formatter::format_hex(0x12, 6, true) == "0x0012");
        REQUIRE (number_formatter::format_hex(0xab, 6, true) == "0x00AB");

        REQUIRE (number_formatter::format_hex((unsigned) 0x12, true) == "0x12");
        REQUIRE (number_formatter::format_hex((unsigned) 0xab, true) == "0xAB");
        REQUIRE (number_formatter::format_hex((unsigned) 0x12, 4, true) == "0x12");
        REQUIRE (number_formatter::format_hex((unsigned) 0xab, 4, true) == "0xAB");
        REQUIRE (number_formatter::format_hex((unsigned) 0x12, 6, true) == "0x0012");
        REQUIRE (number_formatter::format_hex((unsigned) 0xab, 6, true) == "0x00AB");

        REQUIRE (number_formatter::format_hex((long) 0x12, true) == "0x12");
        REQUIRE (number_formatter::format_hex((long) 0xab, true) == "0xAB");
        REQUIRE (number_formatter::format_hex((long) 0x12, 4, true) == "0x12");
        REQUIRE (number_formatter::format_hex((long) 0xab, 4, true) == "0xAB");
        REQUIRE (number_formatter::format_hex((long) 0x12, 6, true) == "0x0012");
        REQUIRE (number_formatter::format_hex((long) 0xab, 6, true) == "0x00AB");

        REQUIRE (number_formatter::format_hex((unsigned long) 0x12, true) == "0x12");
        REQUIRE (number_formatter::format_hex((unsigned long) 0xab, true) == "0xAB");
        REQUIRE (number_formatter::format_hex((unsigned long) 0x12, 4, true) == "0x12");
        REQUIRE (number_formatter::format_hex((unsigned long) 0xab, 4, true) == "0xAB");
        REQUIRE (number_formatter::format_hex((unsigned long) 0x12, 6, true) == "0x0012");
        REQUIRE (number_formatter::format_hex((unsigned long) 0xab, 6, true) == "0x00AB");


        REQUIRE (number_formatter::format_hex((int64_t) 0x12, true) == "0x12");
        REQUIRE (number_formatter::format_hex((int64_t) 0xab, true) == "0xAB");
        REQUIRE (number_formatter::format_hex((int64_t) 0x12, 4, true) == "0x12");
        REQUIRE (number_formatter::format_hex((int64_t) 0xab, 4, true) == "0xAB");
        REQUIRE (number_formatter::format_hex((int64_t) 0x12, 6, true) == "0x0012");
        REQUIRE (number_formatter::format_hex((int64_t) 0xab, 6, true) == "0x00AB");

        REQUIRE (number_formatter::format_hex((uint64_t) 0x12, true) == "0x12");
        REQUIRE (number_formatter::format_hex((uint64_t) 0xab, true) == "0xAB");
        REQUIRE (number_formatter::format_hex((uint64_t) 0x12, 4, true) == "0x12");
        REQUIRE (number_formatter::format_hex((uint64_t) 0xab, 4, true) == "0xAB");
        REQUIRE (number_formatter::format_hex((uint64_t) 0x12, 6, true) == "0x0012");
        REQUIRE (number_formatter::format_hex((uint64_t) 0xab, 6, true) == "0x00AB");
    }

    TEST_CASE("format float")
    {
        REQUIRE (number_formatter::format(1.0f) == "1");
        REQUIRE (number_formatter::format(1.23f) == "1.23");
        REQUIRE (number_formatter::format(-1.23f) == "-1.23");
        REQUIRE (number_formatter::format(0.1f) == "0.1");
        REQUIRE (number_formatter::format(-0.1f) == "-0.1");
        REQUIRE (number_formatter::format(1.23) == "1.23");
        REQUIRE (number_formatter::format(-1.23) == "-1.23");
        REQUIRE (number_formatter::format(1.0) == "1");
        REQUIRE (number_formatter::format(-1.0) == "-1");
        REQUIRE (number_formatter::format(0.1) == "0.1");
        REQUIRE (number_formatter::format(-0.1) == "-0.1");

        int decDigits = std::numeric_limits<double>::digits10;
        std::ostringstream ostr;
        ostr << "0." << std::string(decDigits - 1, '0') << '1';
        REQUIRE (number_formatter::format(1 / std::pow(10., decDigits)) == ostr.str());

        ostr.str("");
        ostr << "1e-" << decDigits + 1;
        std::string str(ostr.str());
        std::string str1(number_formatter::format(1 / std::pow(10., decDigits + 1)));
        REQUIRE (number_formatter::format(1 / std::pow(10., decDigits + 1)) == ostr.str());

        REQUIRE (number_formatter::format(12.25) == "12.25");
        REQUIRE (number_formatter::format(12.25, 4) == "12.2500");
        REQUIRE (number_formatter::format(12.25, 8, 4) == " 12.2500");

        REQUIRE (number_formatter::format(12.45f, 2) == "12.45");

        REQUIRE (number_formatter::format(-12.25) == "-12.25");
        REQUIRE (number_formatter::format(-12.25, 4) == "-12.2500");
        REQUIRE (number_formatter::format(-12.25, 10, 4) == "  -12.2500");
        REQUIRE (number_formatter::format(-12.25, 10, 2) == "    -12.25");
        REQUIRE (number_formatter::format(-12.25, 10, 1) == "     -12.3");

        REQUIRE (number_formatter::format(50.0, 3) == "50.000");
        REQUIRE (number_formatter::format(50.0f, 3) == "50.000");
        REQUIRE (number_formatter::format(50.123, 3) == "50.123");
        REQUIRE (number_formatter::format(50.123f, 3) == "50.123");
        REQUIRE (number_formatter::format(50.123, 0) == "50");
        REQUIRE (number_formatter::format(50.123f, 0) == "50");
        REQUIRE (number_formatter::format(50.546, 0) == "51");
        REQUIRE (number_formatter::format(50.546f, 0) == "51");
        REQUIRE (number_formatter::format(50.546f, 2) == "50.55");
    }

    TEST_CASE("append")
    {
        std::string s;
        number_formatter::append(s, 123);
        REQUIRE (s == "123");
        s.erase();
        number_formatter::append(s, 123, 4);
        REQUIRE (s == " 123");
        s.erase();
        number_formatter::append0(s, 123, 5);
        REQUIRE (s == "00123");
        s.erase();
        number_formatter::appendHex(s, 0xDEAD);
        REQUIRE (s == "DEAD");
        s.erase();
        number_formatter::appendHex(s, 0xDEAD, 6);
        REQUIRE (s == "00DEAD");
        s.erase();
        number_formatter::append(s, 123u);
        REQUIRE (s == "123");
        s.erase();
        number_formatter::append(s, 123u, 4);
        REQUIRE (s == " 123");
        s.erase();
        number_formatter::append0(s, 123u, 5);
        REQUIRE (s == "00123");


        s.erase();
        number_formatter::append(s, 123.4);
        REQUIRE (s == "123.4");
        s.erase();
        number_formatter::append(s, 123.4567, 2);
        REQUIRE (s == "123.46");
        s.erase();
        number_formatter::append(s, 123.4567, 10, 5);
        REQUIRE (s == " 123.45670");
        s.erase();
        number_formatter::append(s, 123., 2);
        REQUIRE (s == "123.00");
        s.erase();
        number_formatter::append(s, static_cast<double>(1234567), 2);
        REQUIRE (s == "1234567.00");
        s.erase();
        number_formatter::append(s, 1234567.0, 10, 1);
        REQUIRE (s == " 1234567.0");
    }

}
