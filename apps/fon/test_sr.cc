//
// Created by igor on 26/06/2022.
//
#include <doctest/doctest.h>

#include "bm.hh"
#include "test_utils.hh"

TEST_SUITE("bitmap shift register") {
  TEST_CASE_FIXTURE(TestFixture1, "sr #1") {

    shift_register<8> sr(a.data() + 0, 7, 0, 8);
    auto r0 = sr.shift();
    REQUIRE(r0 == 0b01001000);

    sr = shift_register<8>(a.data() + 0, 6, 1, 8);
    auto r1 = sr.shift();
    REQUIRE(r1 == 0b10010000);

    sr = shift_register<8>(a.data() + 0, 5, 2, 7);
    auto r2 = sr.shift();
    REQUIRE(r2 == 0b00100000);

    sr = shift_register<8>(a.data() + 0, 4, 3, 6);
    auto r3 = sr.shift();
    REQUIRE(r3 == 0b01000000);

    sr = shift_register<8>(a.data() + 0, 3, 4, 5);
    auto r4 = sr.shift();
    REQUIRE(r4 == 0b10000000);

    sr = shift_register<8>(a.data() + 0, 2, 5, 4);
    auto r5 = sr.shift();
    REQUIRE(r5 == 0b00000000);

    sr = shift_register<8>(a.data() + 0, 1, 6, 3);
    auto r6 = sr.shift();
    REQUIRE(r6 == 0b00000000);

    sr = shift_register<8>(a.data() + 1, 6, 0, 8);
    auto r7 = sr.shift();
    REQUIRE(r7 == 0b01000010);

    sr = shift_register<8>(a.data() + 1, 5, 1, 8);
    auto r8 = sr.shift();
    REQUIRE(r8 == 0b10000101);

    sr = shift_register<8>(a.data() + 1, 4, 2, 7);
    auto r9 = sr.shift();
    REQUIRE(r9 == 0b00001010);

    sr = shift_register<8>(a.data() + 1, 3, 3, 6);
    auto r10 = sr.shift();
    REQUIRE(r10 == 0b00010100);

    sr = shift_register<8>(a.data() + 1, 2, 4, 5);
    auto r11 = sr.shift();
    REQUIRE(r11 == 0b00101000);

    sr = shift_register<8>(a.data() + 1, 1, 5, 4);
    auto r12 = sr.shift();
    REQUIRE(r12 == 0b01010000);

    sr = shift_register<8>(a.data() + 1, 0, 6, 3);
    auto r13 = sr.shift();
    REQUIRE(r13 == 0b10100000);

    sr = shift_register<8>(a.data() + 2, 5, 0, 8);
    auto r14 = sr.shift();
    REQUIRE(r14 == 0b01100000);

    sr = shift_register<8>(a.data() + 2, 4, 1, 8);
    auto r15 = sr.shift();
    REQUIRE(r15 == 0b11000000);

    sr = shift_register<8>(a.data() + 2, 3, 2, 7);
    auto r16 = sr.shift();
    REQUIRE(r16 == 0b10000000);

    sr = shift_register<8>(a.data() + 2, 2, 3, 6);
    auto r17 = sr.shift();
    REQUIRE(r17 == 0b00000000);

    sr = shift_register<8>(a.data() + 2, 1, 4, 5);
    auto r18 = sr.shift();
    REQUIRE(r18 == 0b00000000);

    sr = shift_register<8>(a.data() + 2, 0, 5, 4);
    auto r19 = sr.shift();
    REQUIRE(r19 == 0b00000000);

    sr = shift_register<8>(a.data() + 3, 7, 6, 3);
    auto r20 = sr.shift();
    REQUIRE(r20 == 0b00000000);

    sr = shift_register<8>(a.data() + 3, 4, 0, 8);
    auto r21 = sr.shift();
    REQUIRE(r21 == 0b00111001);

    sr = shift_register<8>(a.data() + 3, 3, 1, 8);
    auto r22 = sr.shift();
    REQUIRE(r22 == 0b01110010);

    sr = shift_register<8>(a.data() + 3, 2, 2, 7);
    auto r23 = sr.shift();
    REQUIRE(r23 == 0b11100100);

    sr = shift_register<8>(a.data() + 3, 1, 3, 6);
    auto r24 = sr.shift();
    REQUIRE(r24 == 0b11001000);

    sr = shift_register<8>(a.data() + 3, 0, 4, 5);
    auto r25 = sr.shift();
    REQUIRE(r25 == 0b10010000);

    sr = shift_register<8>(a.data() + 4, 7, 5, 4);
    auto r26 = sr.shift();
    REQUIRE(r26 == 0b00100000);

    sr = shift_register<8>(a.data() + 4, 6, 6, 3);
    auto r27 = sr.shift();
    REQUIRE(r27 == 0b01000000);

    sr = shift_register<8>(a.data() + 4, 3, 0, 8);
    auto r28 = sr.shift();
    REQUIRE(r28 == 0b01010010);

    sr = shift_register<8>(a.data() + 4, 2, 1, 8);
    auto r29 = sr.shift();
    REQUIRE(r29 == 0b10100100);

    sr = shift_register<8>(a.data() + 4, 1, 2, 7);
    auto r30 = sr.shift();
    REQUIRE(r30 == 0b01001000);

    sr = shift_register<8>(a.data() + 4, 0, 3, 6);
    auto r31 = sr.shift();
    REQUIRE(r31 == 0b10010000);

    sr = shift_register<8>(a.data() + 5, 7, 4, 5);
    auto r32 = sr.shift();
    REQUIRE(r32 == 0b00100000);

    sr = shift_register<8>(a.data() + 5, 6, 5, 4);
    auto r33 = sr.shift();
    REQUIRE(r33 == 0b01000000);

    sr = shift_register<8>(a.data() + 5, 5, 6, 3);
    auto r34 = sr.shift();
    REQUIRE(r34 == 0b10000000);

    sr = shift_register<8>(a.data() + 5, 2, 0, 8);
    auto r35 = sr.shift();
    REQUIRE(r35 == 0b10100011);

    sr = shift_register<8>(a.data() + 5, 1, 1, 8);
    auto r36 = sr.shift();
    REQUIRE(r36 == 0b01000111);

    sr = shift_register<8>(a.data() + 5, 0, 2, 7);
    auto r37 = sr.shift();
    REQUIRE(r37 == 0b10001110);

    sr = shift_register<8>(a.data() + 6, 7, 3, 6);
    auto r38 = sr.shift();
    REQUIRE(r38 == 0b00011100);

    sr = shift_register<8>(a.data() + 6, 6, 4, 5);
    auto r39 = sr.shift();
    REQUIRE(r39 == 0b00111000);

    sr = shift_register<8>(a.data() + 6, 5, 5, 4);
    auto r40 = sr.shift();
    REQUIRE(r40 == 0b01110000);

    sr = shift_register<8>(a.data() + 6, 4, 6, 3);
    auto r41 = sr.shift();
    REQUIRE(r41 == 0b11100000);

    sr = shift_register<8>(a.data() + 6, 1, 0, 8);
    auto r42 = sr.shift();
    REQUIRE(r42 == 0b01001000);

    sr = shift_register<8>(a.data() + 6, 0, 1, 8);
    auto r43 = sr.shift();
    REQUIRE(r43 == 0b10010001);

    sr = shift_register<8>(a.data() + 7, 7, 2, 7);
    auto r44 = sr.shift();
    REQUIRE(r44 == 0b00100010);

    sr = shift_register<8>(a.data() + 7, 6, 3, 6);
    auto r45 = sr.shift();
    REQUIRE(r45 == 0b01000100);

    sr = shift_register<8>(a.data() + 7, 5, 4, 5);
    auto r46 = sr.shift();
    REQUIRE(r46 == 0b10001000);

    sr = shift_register<8>(a.data() + 7, 4, 5, 4);
    auto r47 = sr.shift();
    REQUIRE(r47 == 0b00010000);

    sr = shift_register<8>(a.data() + 7, 3, 6, 3);
    auto r48 = sr.shift();
    REQUIRE(r48 == 0b00100000);

    sr = shift_register<8>(a.data() + 7, 0, 0, 8);
    auto r49 = sr.shift();
    REQUIRE(r49 == 0b00010111);

    sr = shift_register<8>(a.data() + 8, 7, 1, 8);
    auto r50 = sr.shift();
    REQUIRE(r50 == 0b00101110);

    sr = shift_register<8>(a.data() + 8, 6, 2, 7);
    auto r51 = sr.shift();
    REQUIRE(r51 == 0b01011100);

    sr = shift_register<8>(a.data() + 8, 5, 3, 6);
    auto r52 = sr.shift();
    REQUIRE(r52 == 0b10111000);

    sr = shift_register<8>(a.data() + 8, 4, 4, 5);
    auto r53 = sr.shift();
    REQUIRE(r53 == 0b01110000);

    sr = shift_register<8>(a.data() + 8, 3, 5, 4);
    auto r54 = sr.shift();
    REQUIRE(r54 == 0b11100000);

    sr = shift_register<8>(a.data() + 8, 2, 6, 3);
    auto r55 = sr.shift();
    REQUIRE(r55 == 0b11000000);

    sr = shift_register<8>(a.data() + 9, 7, 0, 8);
    auto r56 = sr.shift();
    REQUIRE(r56 == 0b01101001);

    sr = shift_register<8>(a.data() + 9, 6, 1, 8);
    auto r57 = sr.shift();
    REQUIRE(r57 == 0b11010010);

    sr = shift_register<8>(a.data() + 9, 5, 2, 7);
    auto r58 = sr.shift();
    REQUIRE(r58 == 0b10100100);

    sr = shift_register<8>(a.data() + 9, 4, 3, 6);
    auto r59 = sr.shift();
    REQUIRE(r59 == 0b01001000);

    sr = shift_register<8>(a.data() + 9, 3, 4, 5);
    auto r60 = sr.shift();
    REQUIRE(r60 == 0b10010000);

    sr = shift_register<8>(a.data() + 9, 2, 5, 4);
    auto r61 = sr.shift();
    REQUIRE(r61 == 0b00100000);

    sr = shift_register<8>(a.data() + 9, 1, 6, 3);
    auto r62 = sr.shift();
    REQUIRE(r62 == 0b01000000);
  }
}
