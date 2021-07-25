//
// Created by igor on 25/07/2021.
//

#include <doctest/doctest.h>
#include <neutrino/utils/strings/numeric_string.hh>
#include <climits>
using namespace neutrino::utils;
TEST_SUITE("numeric string conversions") {
    TEST_CASE("int2string")
    {
        // decimal
        std::string result;
        REQUIRE (int_to_str(0, 10, result));
        REQUIRE (result == "0");
        REQUIRE (int_to_str(0, 10, result, false, 10, '0'));
        REQUIRE (result == "0000000000");
        REQUIRE (int_to_str(1234567890, 10, result));
        REQUIRE (result == "1234567890");
        REQUIRE (int_to_str(-1234567890, 10, result));
        REQUIRE (result == "-1234567890");
        REQUIRE (int_to_str(-1234567890, 10, result, false, 15, '0'));
        REQUIRE (result == "-00001234567890");
        REQUIRE (int_to_str(-1234567890, 10, result, false, 15));
        REQUIRE (result == "    -1234567890");
        REQUIRE (int_to_str(-1234567890, 10, result, false, 0, 0, ','));
        REQUIRE (result == "-1,234,567,890");

        // binary
        REQUIRE (int_to_str(1234567890, 2, result));
        REQUIRE (result == "1001001100101100000001011010010");
        REQUIRE (int_to_str(1234567890, 2, result, true));
        REQUIRE (result == "1001001100101100000001011010010");
        REQUIRE (int_to_str(1234567890, 2, result, true, 35, '0'));
        REQUIRE (result == "00001001001100101100000001011010010");
        REQUIRE (uint_to_str(0xFF, 2, result));
        REQUIRE (result == "11111111");
        REQUIRE (uint_to_str(0x0F, 2, result, false, 8, '0'));
        REQUIRE (result == "00001111");
        REQUIRE (uint_to_str(0x0F, 2, result));
        REQUIRE (result == "1111");
        REQUIRE (uint_to_str(0xF0, 2, result));
        REQUIRE (result == "11110000");
        REQUIRE (uint_to_str(0xFFFF, 2, result));
        REQUIRE (result == "1111111111111111");
        REQUIRE (uint_to_str(0xFF00, 2, result));
        REQUIRE (result == "1111111100000000");
        REQUIRE (uint_to_str(0xFFFFFFFF, 2, result));
        REQUIRE (result == "11111111111111111111111111111111");
        REQUIRE (uint_to_str(0xFF00FF00, 2, result));
        REQUIRE (result == "11111111000000001111111100000000");
        REQUIRE (uint_to_str(0xF0F0F0F0, 2, result));
        REQUIRE (result == "11110000111100001111000011110000");

        REQUIRE (uint_to_str(0xFFFFFFFFFFFFFFFF, 2, result));
	    REQUIRE (result == "1111111111111111111111111111111111111111111111111111111111111111");
	    REQUIRE (uint_to_str(0xFF00000FF00000FF, 2, result));
	    REQUIRE (result == "1111111100000000000000000000111111110000000000000000000011111111");


        // octal
        REQUIRE (uint_to_str(1234567890, 010, result));
        REQUIRE (result == "11145401322");
        REQUIRE (uint_to_str(1234567890, 010, result, true));
        REQUIRE (result == "011145401322");
        REQUIRE (uint_to_str(1234567890, 010, result, true, 15, '0'));
        REQUIRE (result == "000011145401322");
        REQUIRE (uint_to_str(012345670, 010, result, true));
        REQUIRE (result == "012345670");
        REQUIRE (uint_to_str(012345670, 010, result));
        REQUIRE (result == "12345670");

        // hexadecimal
        REQUIRE (uint_to_str(0, 0x10, result, true));
        REQUIRE (result == "0x0");
        REQUIRE (uint_to_str(0, 0x10, result, true, 4, '0'));
        REQUIRE (result == "0x00");
        REQUIRE (uint_to_str(0, 0x10, result, false, 4, '0'));
        REQUIRE (result == "0000");
        REQUIRE (uint_to_str(1234567890, 0x10, result));
        REQUIRE (result == "499602D2");
        REQUIRE (uint_to_str(1234567890, 0x10, result, true));
        REQUIRE (result == "0x499602D2");
        REQUIRE (uint_to_str(1234567890, 0x10, result, true, 15, '0'));
        REQUIRE (result == "0x00000499602D2");
        REQUIRE (uint_to_str(0x1234567890ABCDEF, 0x10, result, true));
        REQUIRE (result == "0x1234567890ABCDEF");
        REQUIRE (uint_to_str(0xDEADBEEF, 0x10, result));
        REQUIRE (result == "DEADBEEF");

        REQUIRE (uint_to_str(0xFFFFFFFFFFFFFFFF, 0x10, result));
    	REQUIRE (result == "FFFFFFFFFFFFFFFF");
	    REQUIRE (uint_to_str(0xFFFFFFFFFFFFFFFF, 0x10, result, true));
	    REQUIRE (result == "0xFFFFFFFFFFFFFFFF");

        char pResult[NEUTRINO_MAX_INT_STRING_LEN];
        std::size_t sz = NEUTRINO_MAX_INT_STRING_LEN;
        REQUIRE_THROWS(int_to_str(0, 10, pResult, sz, false, (int) sz + 1, ' '));
    }

    TEST_CASE("float2string")
    {
        double val = 1.03721575516329e-112;
        std::string str;

        REQUIRE (double_to_str(str, val, 14, 21) == "1.03721575516329e-112");
        REQUIRE (double_to_str(str, val, 14, 22) == " 1.03721575516329e-112");
        val = -val;
        REQUIRE (double_to_str(str, val, 14, 22) == "-1.03721575516329e-112");
        REQUIRE (double_to_str(str, val, 14, 23) == " -1.03721575516329e-112");

        val = -10372157551632.9;
        REQUIRE (double_to_str(str, val, 1, 21, ',') == "-10,372,157,551,632.9");
        REQUIRE (double_to_str(str, val, 1, 22, ',') == " -10,372,157,551,632.9");
        REQUIRE (double_to_str(str, val, 2, 22, ',') == "-10,372,157,551,632.90");
        REQUIRE (double_to_str(str, val, 2, 22, '.', ',') == "-10.372.157.551.632,90");
        REQUIRE (double_to_str(str, val, 2, 22, ' ', ',') == "-10 372 157 551 632,90");

        int ival = 1234567890;
        REQUIRE (double_to_str(str, ival, 1, 15, ',') == "1,234,567,890.0");
        ival = -123456789;
        REQUIRE (double_to_str(str, ival, 1, 14, ',') == "-123,456,789.0");
    }


    TEST_CASE("testNumericStringLimit")
    {
        char c = 0, t = -1;
        REQUIRE(!isIntOverflow<char>(c));
        REQUIRE(safeIntCast<char>(c, t) == c);
        REQUIRE(t == c);

        short s = SHRT_MAX;
        REQUIRE(isIntOverflow<char>(s));
        REQUIRE_THROWS(safeIntCast(s, t));


        s = SHRT_MIN;
        REQUIRE(isIntOverflow<char>(s));
        REQUIRE_THROWS(safeIntCast(s, t));

        signed char sc = 0, st = -1;
        REQUIRE(!isIntOverflow<signed char>(sc));
        REQUIRE(safeIntCast<char>(sc, st) == sc);
        REQUIRE(st == sc);

        short ss = SHRT_MAX;
        REQUIRE(isIntOverflow<signed char>(ss));
        REQUIRE(isIntOverflow<char>(ss));
        REQUIRE_THROWS(safeIntCast(ss, st));

        ss = SHRT_MIN;
        REQUIRE(isIntOverflow<signed char>(ss));
        REQUIRE(isIntOverflow<char>(ss));
        REQUIRE_THROWS(safeIntCast(ss, st));

        REQUIRE(safeIntCast<signed char>(sc, st) == c);
        REQUIRE(st == sc);

        unsigned char uc = 0, ut = -1;
        REQUIRE(!isIntOverflow<unsigned char>(uc));
        REQUIRE(safeIntCast<char>(uc, ut) == uc);
        REQUIRE(ut == uc);

        ss = SHRT_MAX;
        REQUIRE(isIntOverflow<unsigned char>(ss));
        REQUIRE_THROWS(safeIntCast(ss, st));

        ss = -1;
        REQUIRE(isIntOverflow<unsigned char>(ss));
        REQUIRE_THROWS(safeIntCast(ss, uc));

        int i = 0;
        REQUIRE(!isIntOverflow<int>(i));
        REQUIRE(!isIntOverflow<unsigned>(i));
        i = -1;
        unsigned int ti = -1;
        REQUIRE(isIntOverflow<unsigned>(i));
        REQUIRE_THROWS(safeIntCast(i, ti));

        if (sizeof(long) > sizeof(int))
        {
            long l = LONG_MAX;
            REQUIRE(isIntOverflow<int>(l));
            l = -1L;
            REQUIRE(isIntOverflow<unsigned>(l));
            i = -1;
            REQUIRE(!isIntOverflow<long>(i));
            long tl = 0;
            REQUIRE(safeIntCast(i, tl) == i);
            unsigned long ul = ULONG_MAX, tul = 0;
            REQUIRE(isIntOverflow<long>(ul));
            REQUIRE_THROWS(safeIntCast(ul, tl));

            REQUIRE(!isIntOverflow<unsigned long>(ul));
            tl = 0;
            REQUIRE(safeIntCast(ul, tul) == ul);
            l = LONG_MIN;
            REQUIRE(isIntOverflow<unsigned long>(l));
            REQUIRE_THROWS(safeIntCast(l, ul));

            ul = LONG_MAX;
            REQUIRE(!isIntOverflow<long>(ul));
            REQUIRE(safeIntCast(ul, l) == ul);
        }
/*
        numericStringLimitSameSign<unsigned short, unsigned char>();
        numericStringLimitSameSign<short, char>();
        numericStringLimitSameSign<unsigned int, unsigned short>();
        numericStringLimitSameSign<int, short>();

        if (sizeof(long) > sizeof(int))
        {
            numericStringLimitSameSign<unsigned long, unsigned int>();
            numericStringLimitSameSign<long, int>();
        }

        numericStringLowerLimit<short, char>();
        numericStringLowerLimit<int, short>();

        if (sizeof(long) > sizeof(int))
        {
            numericStringLowerLimit<Poco::Int64, Poco::Int32>();
        }
*/
        REQUIRE(!isIntOverflow<int8_t>(0));
        REQUIRE(isIntOverflow<int8_t>(std::numeric_limits<int16_t>::max()));
        REQUIRE(isIntOverflow<int8_t>(std::numeric_limits<int16_t>::min()));
        REQUIRE(!isIntOverflow<uint8_t>(0));
        REQUIRE(isIntOverflow<uint8_t>(std::numeric_limits<int16_t>::max()));
        REQUIRE(isIntOverflow<uint8_t>(-1));
        REQUIRE(!isIntOverflow<int32_t>(0));
        REQUIRE(isIntOverflow<int32_t>(std::numeric_limits<int64_t>::max()));
        REQUIRE(!isIntOverflow<uint32_t>(0));
        REQUIRE(isIntOverflow<uint32_t>(-1));
        REQUIRE(isIntOverflow<uint32_t>(-1L));
        REQUIRE(isIntOverflow<uint32_t>(-1LL));
        REQUIRE(!isIntOverflow<int64_t>(-1));
        REQUIRE(isIntOverflow<int64_t>(std::numeric_limits<uint64_t>::max()));
        REQUIRE(!isIntOverflow<uint64_t>(std::numeric_limits<uint64_t>::max()));
        REQUIRE(isIntOverflow<uint64_t>(std::numeric_limits<int64_t>::min()));
        REQUIRE(!isIntOverflow<uint64_t>(std::numeric_limits<uint64_t>::min()));
        REQUIRE(!isIntOverflow<int64_t>(std::numeric_limits<int64_t>::max()));
/*
        numericStringLimitSameSign<uint16_t, uint8_t>();
        numericStringLimitSameSign<int16_t, int8_t>();
        numericStringLimitSameSign<uint32_t, uint16_t>();
        numericStringLimitSameSign<int32_t, int16_t>();
        numericStringLimitSameSign<uint64_t, uint32_t>();
        numericStringLimitSameSign<int64_t, int32_t>();

        numericStringLowerLimit<int16_t, int8_t>();
        numericStringLowerLimit<int32_t, int16_t>();
        numericStringLowerLimit<int64_t, int32_t>();
        */
    }

    TEST_CASE("testNumericStringPadding")
    {
        std::string str;
        REQUIRE (float_to_str(str, 0.999f, 2, 4) == "1.00");
        REQUIRE (float_to_str(str, 0.945f, 2, 4) == "0.95");
        REQUIRE (float_to_str(str, 0.944f, 2, 4) == "0.94");
        REQUIRE (float_to_str(str, 12.45f, 2, 5) == "12.45");
        REQUIRE (float_to_str(str, 12.45f, 1, 4) == "12.5");
        REQUIRE (float_to_str(str, 12.45f, 2, 6) == " 12.45");
        REQUIRE (float_to_str(str, 12.455f, 3, 7) == " 12.455");
        REQUIRE (float_to_str(str, 12.455f, 2, 6) == " 12.46");
        REQUIRE (float_to_str(str, 1.23556E-16f, 2, 6) == "1.24e-16");

        REQUIRE (double_to_str(str, 0.999, 2, 4) == "1.00");
        REQUIRE (double_to_str(str, 0.945, 2, 4) == "0.95");
        REQUIRE (double_to_str(str, 0.944, 2, 4) == "0.94");
        REQUIRE (double_to_str(str, 12.45, 2, 5) == "12.45");
        REQUIRE (double_to_str(str, 12.45, 1, 4) == "12.5");
        REQUIRE (double_to_str(str, 12.45, 2, 6) == " 12.45");
        REQUIRE (double_to_str(str, 12.455, 3, 7) == " 12.455");
        REQUIRE (double_to_str(str, 12.455, 2, 6) == " 12.46");
        REQUIRE (double_to_str(str, 1.23556E-16, 2, 6) == "1.24e-16");
    }

}