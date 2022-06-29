//
// Created by igor on 26/06/2022.
//
#include <doctest/doctest.h>

#include "bitmask_test_utils.hh"
#include <neutrino/assets/image/bitmask/bitmask_algo.hh>

using namespace neutrino::assets::detail;
TEST_SUITE("bitmap shift register") {

  TEST_CASE_FIXTURE(test_fixture_16bit, "sr #1") {
    shift_register<16> sr(a.data() + 0, 15, 0, 16);
    auto r0 = sr.shift();
    REQUIRE(r0 == 0b1000111011011101);

    sr = shift_register<16>(a.data() + 0, 14, 1, 16);
    auto r1 = sr.shift();
    REQUIRE(r1 == 0b0001110110111010);

    sr = shift_register<16>(a.data() + 0, 13, 2, 16);
    auto r2 = sr.shift();
    REQUIRE(r2 == 0b0011101101110100);

    sr = shift_register<16>(a.data() + 0, 12, 3, 16);
    auto r3 = sr.shift();
    REQUIRE(r3 == 0b0111011011101001);

    sr = shift_register<16>(a.data() + 0, 11, 4, 16);
    auto r4 = sr.shift();
    REQUIRE(r4 == 0b1110110111010011);

    sr = shift_register<16>(a.data() + 0, 10, 5, 16);
    auto r5 = sr.shift();
    REQUIRE(r5 == 0b1101101110100111);

    sr = shift_register<16>(a.data() + 0, 9, 6, 16);
    auto r6 = sr.shift();
    REQUIRE(r6 == 0b1011011101001110);

    sr = shift_register<16>(a.data() + 0, 8, 7, 16);
    auto r7 = sr.shift();
    REQUIRE(r7 == 0b0110111010011100);

    sr = shift_register<16>(a.data() + 0, 7, 8, 16);
    auto r8 = sr.shift();
    REQUIRE(r8 == 0b1101110100111000);

    sr = shift_register<16>(a.data() + 0, 6, 9, 16);
    auto r9 = sr.shift();
    REQUIRE(r9 == 0b1011101001110000);

    sr = shift_register<16>(a.data() + 0, 5, 10, 16);
    auto r10 = sr.shift();
    REQUIRE(r10 == 0b0111010011100000);

    sr = shift_register<16>(a.data() + 0, 4, 11, 16);
    auto r11 = sr.shift();
    REQUIRE(r11 == 0b1110100111000001);

    sr = shift_register<16>(a.data() + 0, 3, 12, 16);
    auto r12 = sr.shift();
    REQUIRE(r12 == 0b1101001110000011);

    sr = shift_register<16>(a.data() + 0, 2, 13, 16);
    auto r13 = sr.shift();
    REQUIRE(r13 == 0b1010011100000111);

    sr = shift_register<16>(a.data() + 0, 1, 14, 16);
    auto r14 = sr.shift();
    REQUIRE(r14 == 0b0100111000001111);

    sr = shift_register<16>(a.data() + 8, 13, 0, 16);
    auto r15 = sr.shift();
    REQUIRE(r15 == 0b0100010100001101);

    sr = shift_register<16>(a.data() + 8, 12, 1, 16);
    auto r16 = sr.shift();
    REQUIRE(r16 == 0b1000101000011010);

    sr = shift_register<16>(a.data() + 8, 11, 2, 16);
    auto r17 = sr.shift();
    REQUIRE(r17 == 0b0001010000110101);

    sr = shift_register<16>(a.data() + 8, 10, 3, 16);
    auto r18 = sr.shift();
    REQUIRE(r18 == 0b0010100001101011);

    sr = shift_register<16>(a.data() + 8, 9, 4, 16);
    auto r19 = sr.shift();
    REQUIRE(r19 == 0b0101000011010111);

    sr = shift_register<16>(a.data() + 8, 8, 5, 16);
    auto r20 = sr.shift();
    REQUIRE(r20 == 0b1010000110101110);

    sr = shift_register<16>(a.data() + 8, 7, 6, 16);
    auto r21 = sr.shift();
    REQUIRE(r21 == 0b0100001101011101);

    sr = shift_register<16>(a.data() + 8, 6, 7, 16);
    auto r22 = sr.shift();
    REQUIRE(r22 == 0b1000011010111011);

    sr = shift_register<16>(a.data() + 8, 5, 8, 16);
    auto r23 = sr.shift();
    REQUIRE(r23 == 0b0000110101110110);

    sr = shift_register<16>(a.data() + 8, 4, 9, 16);
    auto r24 = sr.shift();
    REQUIRE(r24 == 0b0001101011101101);

    sr = shift_register<16>(a.data() + 8, 3, 10, 16);
    auto r25 = sr.shift();
    REQUIRE(r25 == 0b0011010111011010);

    sr = shift_register<16>(a.data() + 8, 2, 11, 16);
    auto r26 = sr.shift();
    REQUIRE(r26 == 0b0110101110110100);

    sr = shift_register<16>(a.data() + 8, 1, 12, 16);
    auto r27 = sr.shift();
    REQUIRE(r27 == 0b1101011101101001);

    sr = shift_register<16>(a.data() + 8, 0, 13, 16);
    auto r28 = sr.shift();
    REQUIRE(r28 == 0b1010111011010011);

    sr = shift_register<16>(a.data() + 9, 15, 14, 16);
    auto r29 = sr.shift();
    REQUIRE(r29 == 0b0101110110100110);

    sr = shift_register<16>(a.data() + 16, 11, 0, 16);
    auto r30 = sr.shift();
    REQUIRE(r30 == 0b1010010010100000);

    sr = shift_register<16>(a.data() + 16, 10, 1, 16);
    auto r31 = sr.shift();
    REQUIRE(r31 == 0b0100100101000001);

    sr = shift_register<16>(a.data() + 16, 9, 2, 16);
    auto r32 = sr.shift();
    REQUIRE(r32 == 0b1001001010000011);

    sr = shift_register<16>(a.data() + 16, 8, 3, 16);
    auto r33 = sr.shift();
    REQUIRE(r33 == 0b0010010100000111);

    sr = shift_register<16>(a.data() + 16, 7, 4, 16);
    auto r34 = sr.shift();
    REQUIRE(r34 == 0b0100101000001110);

    sr = shift_register<16>(a.data() + 16, 6, 5, 16);
    auto r35 = sr.shift();
    REQUIRE(r35 == 0b1001010000011100);

    sr = shift_register<16>(a.data() + 16, 5, 6, 16);
    auto r36 = sr.shift();
    REQUIRE(r36 == 0b0010100000111001);

    sr = shift_register<16>(a.data() + 16, 4, 7, 16);
    auto r37 = sr.shift();
    REQUIRE(r37 == 0b0101000001110010);

    sr = shift_register<16>(a.data() + 16, 3, 8, 16);
    auto r38 = sr.shift();
    REQUIRE(r38 == 0b1010000011100101);

    sr = shift_register<16>(a.data() + 16, 2, 9, 16);
    auto r39 = sr.shift();
    REQUIRE(r39 == 0b0100000111001010);

    sr = shift_register<16>(a.data() + 16, 1, 10, 16);
    auto r40 = sr.shift();
    REQUIRE(r40 == 0b1000001110010101);

    sr = shift_register<16>(a.data() + 16, 0, 11, 16);
    auto r41 = sr.shift();
    REQUIRE(r41 == 0b0000011100101011);

    sr = shift_register<16>(a.data() + 17, 15, 12, 16);
    auto r42 = sr.shift();
    REQUIRE(r42 == 0b0000111001010111);

    sr = shift_register<16>(a.data() + 17, 14, 13, 16);
    auto r43 = sr.shift();
    REQUIRE(r43 == 0b0001110010101111);

    sr = shift_register<16>(a.data() + 17, 13, 14, 16);
    auto r44 = sr.shift();
    REQUIRE(r44 == 0b0011100101011110);

    sr = shift_register<16>(a.data() + 24, 9, 0, 16);
    auto r45 = sr.shift();
    REQUIRE(r45 == 0b1110011001001011);

    sr = shift_register<16>(a.data() + 24, 8, 1, 16);
    auto r46 = sr.shift();
    REQUIRE(r46 == 0b1100110010010110);

    sr = shift_register<16>(a.data() + 24, 7, 2, 16);
    auto r47 = sr.shift();
    REQUIRE(r47 == 0b1001100100101100);

    sr = shift_register<16>(a.data() + 24, 6, 3, 16);
    auto r48 = sr.shift();
    REQUIRE(r48 == 0b0011001001011000);

    sr = shift_register<16>(a.data() + 24, 5, 4, 16);
    auto r49 = sr.shift();
    REQUIRE(r49 == 0b0110010010110001);

    sr = shift_register<16>(a.data() + 24, 4, 5, 16);
    auto r50 = sr.shift();
    REQUIRE(r50 == 0b1100100101100010);

    sr = shift_register<16>(a.data() + 24, 3, 6, 16);
    auto r51 = sr.shift();
    REQUIRE(r51 == 0b1001001011000100);

    sr = shift_register<16>(a.data() + 24, 2, 7, 16);
    auto r52 = sr.shift();
    REQUIRE(r52 == 0b0010010110001000);

    sr = shift_register<16>(a.data() + 24, 1, 8, 16);
    auto r53 = sr.shift();
    REQUIRE(r53 == 0b0100101100010001);

    sr = shift_register<16>(a.data() + 24, 0, 9, 16);
    auto r54 = sr.shift();
    REQUIRE(r54 == 0b1001011000100011);

    sr = shift_register<16>(a.data() + 25, 15, 10, 16);
    auto r55 = sr.shift();
    REQUIRE(r55 == 0b0010110001000111);

    sr = shift_register<16>(a.data() + 25, 14, 11, 16);
    auto r56 = sr.shift();
    REQUIRE(r56 == 0b0101100010001111);

    sr = shift_register<16>(a.data() + 25, 13, 12, 16);
    auto r57 = sr.shift();
    REQUIRE(r57 == 0b1011000100011111);

    sr = shift_register<16>(a.data() + 25, 12, 13, 16);
    auto r58 = sr.shift();
    REQUIRE(r58 == 0b0110001000111110);

    sr = shift_register<16>(a.data() + 25, 11, 14, 16);
    auto r59 = sr.shift();
    REQUIRE(r59 == 0b1100010001111100);

    sr = shift_register<16>(a.data() + 32, 7, 0, 16);
    auto r60 = sr.shift();
    REQUIRE(r60 == 0b0101010000110011);

    sr = shift_register<16>(a.data() + 32, 6, 1, 16);
    auto r61 = sr.shift();
    REQUIRE(r61 == 0b1010100001100110);

    sr = shift_register<16>(a.data() + 32, 5, 2, 16);
    auto r62 = sr.shift();
    REQUIRE(r62 == 0b0101000011001100);

    sr = shift_register<16>(a.data() + 32, 4, 3, 16);
    auto r63 = sr.shift();
    REQUIRE(r63 == 0b1010000110011000);

    sr = shift_register<16>(a.data() + 32, 3, 4, 16);
    auto r64 = sr.shift();
    REQUIRE(r64 == 0b0100001100110001);

    sr = shift_register<16>(a.data() + 32, 2, 5, 16);
    auto r65 = sr.shift();
    REQUIRE(r65 == 0b1000011001100011);

    sr = shift_register<16>(a.data() + 32, 1, 6, 16);
    auto r66 = sr.shift();
    REQUIRE(r66 == 0b0000110011000111);

    sr = shift_register<16>(a.data() + 32, 0, 7, 16);
    auto r67 = sr.shift();
    REQUIRE(r67 == 0b0001100110001111);

    sr = shift_register<16>(a.data() + 33, 15, 8, 16);
    auto r68 = sr.shift();
    REQUIRE(r68 == 0b0011001100011110);

    sr = shift_register<16>(a.data() + 33, 14, 9, 16);
    auto r69 = sr.shift();
    REQUIRE(r69 == 0b0110011000111101);

    sr = shift_register<16>(a.data() + 33, 13, 10, 16);
    auto r70 = sr.shift();
    REQUIRE(r70 == 0b1100110001111010);

    sr = shift_register<16>(a.data() + 33, 12, 11, 16);
    auto r71 = sr.shift();
    REQUIRE(r71 == 0b1001100011110100);

    sr = shift_register<16>(a.data() + 33, 11, 12, 16);
    auto r72 = sr.shift();
    REQUIRE(r72 == 0b0011000111101000);

    sr = shift_register<16>(a.data() + 33, 10, 13, 16);
    auto r73 = sr.shift();
    REQUIRE(r73 == 0b0110001111010000);

    sr = shift_register<16>(a.data() + 33, 9, 14, 16);
    auto r74 = sr.shift();
    REQUIRE(r74 == 0b1100011110100001);

    sr = shift_register<16>(a.data() + 40, 5, 0, 16);
    auto r75 = sr.shift();
    REQUIRE(r75 == 0b1001010000000010);

    sr = shift_register<16>(a.data() + 40, 4, 1, 16);
    auto r76 = sr.shift();
    REQUIRE(r76 == 0b0010100000000101);

    sr = shift_register<16>(a.data() + 40, 3, 2, 16);
    auto r77 = sr.shift();
    REQUIRE(r77 == 0b0101000000001011);

    sr = shift_register<16>(a.data() + 40, 2, 3, 16);
    auto r78 = sr.shift();
    REQUIRE(r78 == 0b1010000000010111);

    sr = shift_register<16>(a.data() + 40, 1, 4, 16);
    auto r79 = sr.shift();
    REQUIRE(r79 == 0b0100000000101110);

    sr = shift_register<16>(a.data() + 40, 0, 5, 16);
    auto r80 = sr.shift();
    REQUIRE(r80 == 0b1000000001011101);

    sr = shift_register<16>(a.data() + 41, 15, 6, 16);
    auto r81 = sr.shift();
    REQUIRE(r81 == 0b0000000010111010);

    sr = shift_register<16>(a.data() + 41, 14, 7, 16);
    auto r82 = sr.shift();
    REQUIRE(r82 == 0b0000000101110101);

    sr = shift_register<16>(a.data() + 41, 13, 8, 16);
    auto r83 = sr.shift();
    REQUIRE(r83 == 0b0000001011101010);

    sr = shift_register<16>(a.data() + 41, 12, 9, 16);
    auto r84 = sr.shift();
    REQUIRE(r84 == 0b0000010111010101);

    sr = shift_register<16>(a.data() + 41, 11, 10, 16);
    auto r85 = sr.shift();
    REQUIRE(r85 == 0b0000101110101011);

    sr = shift_register<16>(a.data() + 41, 10, 11, 16);
    auto r86 = sr.shift();
    REQUIRE(r86 == 0b0001011101010111);

    sr = shift_register<16>(a.data() + 41, 9, 12, 16);
    auto r87 = sr.shift();
    REQUIRE(r87 == 0b0010111010101111);

    sr = shift_register<16>(a.data() + 41, 8, 13, 16);
    auto r88 = sr.shift();
    REQUIRE(r88 == 0b0101110101011111);

    sr = shift_register<16>(a.data() + 41, 7, 14, 16);
    auto r89 = sr.shift();
    REQUIRE(r89 == 0b1011101010111111);

    sr = shift_register<16>(a.data() + 48, 3, 0, 16);
    auto r90 = sr.shift();
    REQUIRE(r90 == 0b1001111111010101);

    sr = shift_register<16>(a.data() + 48, 2, 1, 16);
    auto r91 = sr.shift();
    REQUIRE(r91 == 0b0011111110101010);

    sr = shift_register<16>(a.data() + 48, 1, 2, 16);
    auto r92 = sr.shift();
    REQUIRE(r92 == 0b0111111101010101);

    sr = shift_register<16>(a.data() + 48, 0, 3, 16);
    auto r93 = sr.shift();
    REQUIRE(r93 == 0b1111111010101011);

    sr = shift_register<16>(a.data() + 49, 15, 4, 16);
    auto r94 = sr.shift();
    REQUIRE(r94 == 0b1111110101010111);

    sr = shift_register<16>(a.data() + 49, 14, 5, 16);
    auto r95 = sr.shift();
    REQUIRE(r95 == 0b1111101010101111);

    sr = shift_register<16>(a.data() + 49, 13, 6, 16);
    auto r96 = sr.shift();
    REQUIRE(r96 == 0b1111010101011111);

    sr = shift_register<16>(a.data() + 49, 12, 7, 16);
    auto r97 = sr.shift();
    REQUIRE(r97 == 0b1110101010111111);

    sr = shift_register<16>(a.data() + 49, 11, 8, 16);
    auto r98 = sr.shift();
    REQUIRE(r98 == 0b1101010101111111);

    sr = shift_register<16>(a.data() + 49, 10, 9, 16);
    auto r99 = sr.shift();
    REQUIRE(r99 == 0b1010101011111110);

    sr = shift_register<16>(a.data() + 49, 9, 10, 16);
    auto r100 = sr.shift();
    REQUIRE(r100 == 0b0101010111111101);

    sr = shift_register<16>(a.data() + 49, 8, 11, 16);
    auto r101 = sr.shift();
    REQUIRE(r101 == 0b1010101111111011);

    sr = shift_register<16>(a.data() + 49, 7, 12, 16);
    auto r102 = sr.shift();
    REQUIRE(r102 == 0b0101011111110111);

    sr = shift_register<16>(a.data() + 49, 6, 13, 16);
    auto r103 = sr.shift();
    REQUIRE(r103 == 0b1010111111101110);

    sr = shift_register<16>(a.data() + 49, 5, 14, 16);
    auto r104 = sr.shift();
    REQUIRE(r104 == 0b0101111111011100);

    sr = shift_register<16>(a.data() + 56, 1, 0, 16);
    auto r105 = sr.shift();
    REQUIRE(r105 == 0b1010011010110100);

    sr = shift_register<16>(a.data() + 56, 0, 1, 16);
    auto r106 = sr.shift();
    REQUIRE(r106 == 0b0100110101101000);

    sr = shift_register<16>(a.data() + 57, 15, 2, 16);
    auto r107 = sr.shift();
    REQUIRE(r107 == 0b1001101011010001);

    sr = shift_register<16>(a.data() + 57, 14, 3, 16);
    auto r108 = sr.shift();
    REQUIRE(r108 == 0b0011010110100011);

    sr = shift_register<16>(a.data() + 57, 13, 4, 16);
    auto r109 = sr.shift();
    REQUIRE(r109 == 0b0110101101000110);

    sr = shift_register<16>(a.data() + 57, 12, 5, 16);
    auto r110 = sr.shift();
    REQUIRE(r110 == 0b1101011010001101);

    sr = shift_register<16>(a.data() + 57, 11, 6, 16);
    auto r111 = sr.shift();
    REQUIRE(r111 == 0b1010110100011011);

    sr = shift_register<16>(a.data() + 57, 10, 7, 16);
    auto r112 = sr.shift();
    REQUIRE(r112 == 0b0101101000110111);

    sr = shift_register<16>(a.data() + 57, 9, 8, 16);
    auto r113 = sr.shift();
    REQUIRE(r113 == 0b1011010001101111);

    sr = shift_register<16>(a.data() + 57, 8, 9, 16);
    auto r114 = sr.shift();
    REQUIRE(r114 == 0b0110100011011111);

    sr = shift_register<16>(a.data() + 57, 7, 10, 16);
    auto r115 = sr.shift();
    REQUIRE(r115 == 0b1101000110111111);

    sr = shift_register<16>(a.data() + 57, 6, 11, 16);
    auto r116 = sr.shift();
    REQUIRE(r116 == 0b1010001101111110);

    sr = shift_register<16>(a.data() + 57, 5, 12, 16);
    auto r117 = sr.shift();
    REQUIRE(r117 == 0b0100011011111101);

    sr = shift_register<16>(a.data() + 57, 4, 13, 16);
    auto r118 = sr.shift();
    REQUIRE(r118 == 0b1000110111111010);

    sr = shift_register<16>(a.data() + 57, 3, 14, 16);
    auto r119 = sr.shift();
    REQUIRE(r119 == 0b0001101111110101);

    sr = shift_register<16>(a.data() + 65, 15, 0, 16);
    auto r120 = sr.shift();
    REQUIRE(r120 == 0b1101111100110101);

    sr = shift_register<16>(a.data() + 65, 14, 1, 16);
    auto r121 = sr.shift();
    REQUIRE(r121 == 0b1011111001101011);

    sr = shift_register<16>(a.data() + 65, 13, 2, 16);
    auto r122 = sr.shift();
    REQUIRE(r122 == 0b0111110011010111);

    sr = shift_register<16>(a.data() + 65, 12, 3, 16);
    auto r123 = sr.shift();
    REQUIRE(r123 == 0b1111100110101111);

    sr = shift_register<16>(a.data() + 65, 11, 4, 16);
    auto r124 = sr.shift();
    REQUIRE(r124 == 0b1111001101011111);

    sr = shift_register<16>(a.data() + 65, 10, 5, 16);
    auto r125 = sr.shift();
    REQUIRE(r125 == 0b1110011010111111);

    sr = shift_register<16>(a.data() + 65, 9, 6, 16);
    auto r126 = sr.shift();
    REQUIRE(r126 == 0b1100110101111111);

    sr = shift_register<16>(a.data() + 65, 8, 7, 16);
    auto r127 = sr.shift();
    REQUIRE(r127 == 0b1001101011111111);

    sr = shift_register<16>(a.data() + 65, 7, 8, 16);
    auto r128 = sr.shift();
    REQUIRE(r128 == 0b0011010111111110);

    sr = shift_register<16>(a.data() + 65, 6, 9, 16);
    auto r129 = sr.shift();
    REQUIRE(r129 == 0b0110101111111100);

    sr = shift_register<16>(a.data() + 65, 5, 10, 16);
    auto r130 = sr.shift();
    REQUIRE(r130 == 0b1101011111111000);

    sr = shift_register<16>(a.data() + 65, 4, 11, 16);
    auto r131 = sr.shift();
    REQUIRE(r131 == 0b1010111111110000);

    sr = shift_register<16>(a.data() + 65, 3, 12, 16);
    auto r132 = sr.shift();
    REQUIRE(r132 == 0b0101111111100000);

    sr = shift_register<16>(a.data() + 65, 2, 13, 16);
    auto r133 = sr.shift();
    REQUIRE(r133 == 0b1011111111000001);

    sr = shift_register<16>(a.data() + 65, 1, 14, 16);
    auto r134 = sr.shift();
    REQUIRE(r134 == 0b0111111110000010);

    sr = shift_register<16>(a.data() + 73, 13, 0, 16);
    auto r135 = sr.shift();
    REQUIRE(r135 == 0b1011110110111010);

    sr = shift_register<16>(a.data() + 73, 12, 1, 16);
    auto r136 = sr.shift();
    REQUIRE(r136 == 0b0111101101110100);

    sr = shift_register<16>(a.data() + 73, 11, 2, 16);
    auto r137 = sr.shift();
    REQUIRE(r137 == 0b1111011011101001);

    sr = shift_register<16>(a.data() + 73, 10, 3, 16);
    auto r138 = sr.shift();
    REQUIRE(r138 == 0b1110110111010010);

    sr = shift_register<16>(a.data() + 73, 9, 4, 16);
    auto r139 = sr.shift();
    REQUIRE(r139 == 0b1101101110100101);

    sr = shift_register<16>(a.data() + 73, 8, 5, 16);
    auto r140 = sr.shift();
    REQUIRE(r140 == 0b1011011101001011);

    sr = shift_register<16>(a.data() + 73, 7, 6, 16);
    auto r141 = sr.shift();
    REQUIRE(r141 == 0b0110111010010111);

    sr = shift_register<16>(a.data() + 73, 6, 7, 16);
    auto r142 = sr.shift();
    REQUIRE(r142 == 0b1101110100101111);

    sr = shift_register<16>(a.data() + 73, 5, 8, 16);
    auto r143 = sr.shift();
    REQUIRE(r143 == 0b1011101001011111);

    sr = shift_register<16>(a.data() + 73, 4, 9, 16);
    auto r144 = sr.shift();
    REQUIRE(r144 == 0b0111010010111110);

    sr = shift_register<16>(a.data() + 73, 3, 10, 16);
    auto r145 = sr.shift();
    REQUIRE(r145 == 0b1110100101111101);

    sr = shift_register<16>(a.data() + 73, 2, 11, 16);
    auto r146 = sr.shift();
    REQUIRE(r146 == 0b1101001011111010);

    sr = shift_register<16>(a.data() + 73, 1, 12, 16);
    auto r147 = sr.shift();
    REQUIRE(r147 == 0b1010010111110100);

    sr = shift_register<16>(a.data() + 73, 0, 13, 16);
    auto r148 = sr.shift();
    REQUIRE(r148 == 0b0100101111101001);

    sr = shift_register<16>(a.data() + 74, 15, 14, 16);
    auto r149 = sr.shift();
    REQUIRE(r149 == 0b1001011111010010);

    sr = shift_register<16>(a.data() + 81, 11, 0, 16);
    auto r150 = sr.shift();
    REQUIRE(r150 == 0b0110101000100100);

    sr = shift_register<16>(a.data() + 81, 10, 1, 16);
    auto r151 = sr.shift();
    REQUIRE(r151 == 0b1101010001001000);

    sr = shift_register<16>(a.data() + 81, 9, 2, 16);
    auto r152 = sr.shift();
    REQUIRE(r152 == 0b1010100010010001);

    sr = shift_register<16>(a.data() + 81, 8, 3, 16);
    auto r153 = sr.shift();
    REQUIRE(r153 == 0b0101000100100011);

    sr = shift_register<16>(a.data() + 81, 7, 4, 16);
    auto r154 = sr.shift();
    REQUIRE(r154 == 0b1010001001000111);

    sr = shift_register<16>(a.data() + 81, 6, 5, 16);
    auto r155 = sr.shift();
    REQUIRE(r155 == 0b0100010010001111);

    sr = shift_register<16>(a.data() + 81, 5, 6, 16);
    auto r156 = sr.shift();
    REQUIRE(r156 == 0b1000100100011111);

    sr = shift_register<16>(a.data() + 81, 4, 7, 16);
    auto r157 = sr.shift();
    REQUIRE(r157 == 0b0001001000111110);

    sr = shift_register<16>(a.data() + 81, 3, 8, 16);
    auto r158 = sr.shift();
    REQUIRE(r158 == 0b0010010001111101);

    sr = shift_register<16>(a.data() + 81, 2, 9, 16);
    auto r159 = sr.shift();
    REQUIRE(r159 == 0b0100100011111010);

    sr = shift_register<16>(a.data() + 81, 1, 10, 16);
    auto r160 = sr.shift();
    REQUIRE(r160 == 0b1001000111110100);

    sr = shift_register<16>(a.data() + 81, 0, 11, 16);
    auto r161 = sr.shift();
    REQUIRE(r161 == 0b0010001111101000);

    sr = shift_register<16>(a.data() + 82, 15, 12, 16);
    auto r162 = sr.shift();
    REQUIRE(r162 == 0b0100011111010000);

    sr = shift_register<16>(a.data() + 82, 14, 13, 16);
    auto r163 = sr.shift();
    REQUIRE(r163 == 0b1000111110100001);

    sr = shift_register<16>(a.data() + 82, 13, 14, 16);
    auto r164 = sr.shift();
    REQUIRE(r164 == 0b0001111101000010);

    sr = shift_register<16>(a.data() + 89, 9, 0, 16);
    auto r165 = sr.shift();
    REQUIRE(r165 == 0b1111111010110101);

    sr = shift_register<16>(a.data() + 89, 8, 1, 16);
    auto r166 = sr.shift();
    REQUIRE(r166 == 0b1111110101101010);

    sr = shift_register<16>(a.data() + 89, 7, 2, 16);
    auto r167 = sr.shift();
    REQUIRE(r167 == 0b1111101011010100);

    sr = shift_register<16>(a.data() + 89, 6, 3, 16);
    auto r168 = sr.shift();
    REQUIRE(r168 == 0b1111010110101001);

    sr = shift_register<16>(a.data() + 89, 5, 4, 16);
    auto r169 = sr.shift();
    REQUIRE(r169 == 0b1110101101010011);

    sr = shift_register<16>(a.data() + 89, 4, 5, 16);
    auto r170 = sr.shift();
    REQUIRE(r170 == 0b1101011010100111);

    sr = shift_register<16>(a.data() + 89, 3, 6, 16);
    auto r171 = sr.shift();
    REQUIRE(r171 == 0b1010110101001111);

    sr = shift_register<16>(a.data() + 89, 2, 7, 16);
    auto r172 = sr.shift();
    REQUIRE(r172 == 0b0101101010011110);

    sr = shift_register<16>(a.data() + 89, 1, 8, 16);
    auto r173 = sr.shift();
    REQUIRE(r173 == 0b1011010100111101);

    sr = shift_register<16>(a.data() + 89, 0, 9, 16);
    auto r174 = sr.shift();
    REQUIRE(r174 == 0b0110101001111011);

    sr = shift_register<16>(a.data() + 90, 15, 10, 16);
    auto r175 = sr.shift();
    REQUIRE(r175 == 0b1101010011110111);

    sr = shift_register<16>(a.data() + 90, 14, 11, 16);
    auto r176 = sr.shift();
    REQUIRE(r176 == 0b1010100111101110);

    sr = shift_register<16>(a.data() + 90, 13, 12, 16);
    auto r177 = sr.shift();
    REQUIRE(r177 == 0b0101001111011101);

    sr = shift_register<16>(a.data() + 90, 12, 13, 16);
    auto r178 = sr.shift();
    REQUIRE(r178 == 0b1010011110111011);

    sr = shift_register<16>(a.data() + 90, 11, 14, 16);
    auto r179 = sr.shift();
    REQUIRE(r179 == 0b0100111101110110);

    sr = shift_register<16>(a.data() + 97, 7, 0, 16);
    auto r180 = sr.shift();
    REQUIRE(r180 == 0b0110101111001101);

    sr = shift_register<16>(a.data() + 97, 6, 1, 16);
    auto r181 = sr.shift();
    REQUIRE(r181 == 0b1101011110011011);

    sr = shift_register<16>(a.data() + 97, 5, 2, 16);
    auto r182 = sr.shift();
    REQUIRE(r182 == 0b1010111100110111);

    sr = shift_register<16>(a.data() + 97, 4, 3, 16);
    auto r183 = sr.shift();
    REQUIRE(r183 == 0b0101111001101111);

    sr = shift_register<16>(a.data() + 97, 3, 4, 16);
    auto r184 = sr.shift();
    REQUIRE(r184 == 0b1011110011011111);

    sr = shift_register<16>(a.data() + 97, 2, 5, 16);
    auto r185 = sr.shift();
    REQUIRE(r185 == 0b0111100110111110);

    sr = shift_register<16>(a.data() + 97, 1, 6, 16);
    auto r186 = sr.shift();
    REQUIRE(r186 == 0b1111001101111100);

    sr = shift_register<16>(a.data() + 97, 0, 7, 16);
    auto r187 = sr.shift();
    REQUIRE(r187 == 0b1110011011111001);

    sr = shift_register<16>(a.data() + 98, 15, 8, 16);
    auto r188 = sr.shift();
    REQUIRE(r188 == 0b1100110111110011);

    sr = shift_register<16>(a.data() + 98, 14, 9, 16);
    auto r189 = sr.shift();
    REQUIRE(r189 == 0b1001101111100111);

    sr = shift_register<16>(a.data() + 98, 13, 10, 16);
    auto r190 = sr.shift();
    REQUIRE(r190 == 0b0011011111001111);

    sr = shift_register<16>(a.data() + 98, 12, 11, 16);
    auto r191 = sr.shift();
    REQUIRE(r191 == 0b0110111110011110);

    sr = shift_register<16>(a.data() + 98, 11, 12, 16);
    auto r192 = sr.shift();
    REQUIRE(r192 == 0b1101111100111101);

    sr = shift_register<16>(a.data() + 98, 10, 13, 16);
    auto r193 = sr.shift();
    REQUIRE(r193 == 0b1011111001111011);

    sr = shift_register<16>(a.data() + 98, 9, 14, 16);
    auto r194 = sr.shift();
    REQUIRE(r194 == 0b0111110011110110);

    sr = shift_register<16>(a.data() + 105, 5, 0, 16);
    auto r195 = sr.shift();
    REQUIRE(r195 == 0b0000000101001111);

    sr = shift_register<16>(a.data() + 105, 4, 1, 16);
    auto r196 = sr.shift();
    REQUIRE(r196 == 0b0000001010011111);

    sr = shift_register<16>(a.data() + 105, 3, 2, 16);
    auto r197 = sr.shift();
    REQUIRE(r197 == 0b0000010100111111);

    sr = shift_register<16>(a.data() + 105, 2, 3, 16);
    auto r198 = sr.shift();
    REQUIRE(r198 == 0b0000101001111110);

    sr = shift_register<16>(a.data() + 105, 1, 4, 16);
    auto r199 = sr.shift();
    REQUIRE(r199 == 0b0001010011111101);

    sr = shift_register<16>(a.data() + 105, 0, 5, 16);
    auto r200 = sr.shift();
    REQUIRE(r200 == 0b0010100111111011);

    sr = shift_register<16>(a.data() + 106, 15, 6, 16);
    auto r201 = sr.shift();
    REQUIRE(r201 == 0b0101001111110111);

    sr = shift_register<16>(a.data() + 106, 14, 7, 16);
    auto r202 = sr.shift();
    REQUIRE(r202 == 0b1010011111101110);

    sr = shift_register<16>(a.data() + 106, 13, 8, 16);
    auto r203 = sr.shift();
    REQUIRE(r203 == 0b0100111111011101);

    sr = shift_register<16>(a.data() + 106, 12, 9, 16);
    auto r204 = sr.shift();
    REQUIRE(r204 == 0b1001111110111010);

    sr = shift_register<16>(a.data() + 106, 11, 10, 16);
    auto r205 = sr.shift();
    REQUIRE(r205 == 0b0011111101110101);

    sr = shift_register<16>(a.data() + 106, 10, 11, 16);
    auto r206 = sr.shift();
    REQUIRE(r206 == 0b0111111011101011);

    sr = shift_register<16>(a.data() + 106, 9, 12, 16);
    auto r207 = sr.shift();
    REQUIRE(r207 == 0b1111110111010110);

    sr = shift_register<16>(a.data() + 106, 8, 13, 16);
    auto r208 = sr.shift();
    REQUIRE(r208 == 0b1111101110101100);

    sr = shift_register<16>(a.data() + 106, 7, 14, 16);
    auto r209 = sr.shift();
    REQUIRE(r209 == 0b1111011101011001);

    sr = shift_register<16>(a.data() + 113, 3, 0, 16);
    auto r210 = sr.shift();
    REQUIRE(r210 == 0b0100100010011001);

    sr = shift_register<16>(a.data() + 113, 2, 1, 16);
    auto r211 = sr.shift();
    REQUIRE(r211 == 0b1001000100110010);

    sr = shift_register<16>(a.data() + 113, 1, 2, 16);
    auto r212 = sr.shift();
    REQUIRE(r212 == 0b0010001001100101);

    sr = shift_register<16>(a.data() + 113, 0, 3, 16);
    auto r213 = sr.shift();
    REQUIRE(r213 == 0b0100010011001011);

    sr = shift_register<16>(a.data() + 114, 15, 4, 16);
    auto r214 = sr.shift();
    REQUIRE(r214 == 0b1000100110010110);

    sr = shift_register<16>(a.data() + 114, 14, 5, 16);
    auto r215 = sr.shift();
    REQUIRE(r215 == 0b0001001100101100);

    sr = shift_register<16>(a.data() + 114, 13, 6, 16);
    auto r216 = sr.shift();
    REQUIRE(r216 == 0b0010011001011000);

    sr = shift_register<16>(a.data() + 114, 12, 7, 16);
    auto r217 = sr.shift();
    REQUIRE(r217 == 0b0100110010110001);

    sr = shift_register<16>(a.data() + 114, 11, 8, 16);
    auto r218 = sr.shift();
    REQUIRE(r218 == 0b1001100101100010);

    sr = shift_register<16>(a.data() + 114, 10, 9, 16);
    auto r219 = sr.shift();
    REQUIRE(r219 == 0b0011001011000100);

    sr = shift_register<16>(a.data() + 114, 9, 10, 16);
    auto r220 = sr.shift();
    REQUIRE(r220 == 0b0110010110001000);

    sr = shift_register<16>(a.data() + 114, 8, 11, 16);
    auto r221 = sr.shift();
    REQUIRE(r221 == 0b1100101100010000);

    sr = shift_register<16>(a.data() + 114, 7, 12, 16);
    auto r222 = sr.shift();
    REQUIRE(r222 == 0b1001011000100001);

    sr = shift_register<16>(a.data() + 114, 6, 13, 16);
    auto r223 = sr.shift();
    REQUIRE(r223 == 0b0010110001000010);

    sr = shift_register<16>(a.data() + 114, 5, 14, 16);
    auto r224 = sr.shift();
    REQUIRE(r224 == 0b0101100010000100);

    sr = shift_register<16>(a.data() + 121, 1, 0, 16);
    auto r225 = sr.shift();
    REQUIRE(r225 == 0b0101001011001111);

    sr = shift_register<16>(a.data() + 121, 0, 1, 16);
    auto r226 = sr.shift();
    REQUIRE(r226 == 0b1010010110011111);

    sr = shift_register<16>(a.data() + 122, 15, 2, 16);
    auto r227 = sr.shift();
    REQUIRE(r227 == 0b0100101100111111);

    sr = shift_register<16>(a.data() + 122, 14, 3, 16);
    auto r228 = sr.shift();
    REQUIRE(r228 == 0b1001011001111111);

    sr = shift_register<16>(a.data() + 122, 13, 4, 16);
    auto r229 = sr.shift();
    REQUIRE(r229 == 0b0010110011111110);

    sr = shift_register<16>(a.data() + 122, 12, 5, 16);
    auto r230 = sr.shift();
    REQUIRE(r230 == 0b0101100111111100);

    sr = shift_register<16>(a.data() + 122, 11, 6, 16);
    auto r231 = sr.shift();
    REQUIRE(r231 == 0b1011001111111000);

    sr = shift_register<16>(a.data() + 122, 10, 7, 16);
    auto r232 = sr.shift();
    REQUIRE(r232 == 0b0110011111110001);

    sr = shift_register<16>(a.data() + 122, 9, 8, 16);
    auto r233 = sr.shift();
    REQUIRE(r233 == 0b1100111111100011);

    sr = shift_register<16>(a.data() + 122, 8, 9, 16);
    auto r234 = sr.shift();
    REQUIRE(r234 == 0b1001111111000111);

    sr = shift_register<16>(a.data() + 122, 7, 10, 16);
    auto r235 = sr.shift();
    REQUIRE(r235 == 0b0011111110001110);

    sr = shift_register<16>(a.data() + 122, 6, 11, 16);
    auto r236 = sr.shift();
    REQUIRE(r236 == 0b0111111100011100);

    sr = shift_register<16>(a.data() + 122, 5, 12, 16);
    auto r237 = sr.shift();
    REQUIRE(r237 == 0b1111111000111001);

    sr = shift_register<16>(a.data() + 122, 4, 13, 16);
    auto r238 = sr.shift();
    REQUIRE(r238 == 0b1111110001110010);

    sr = shift_register<16>(a.data() + 122, 3, 14, 16);
    auto r239 = sr.shift();
    REQUIRE(r239 == 0b1111100011100100);

    sr = shift_register<16>(a.data() + 130, 15, 0, 16);
    auto r240 = sr.shift();
    REQUIRE(r240 == 0b0111111001110110);

    sr = shift_register<16>(a.data() + 130, 14, 1, 16);
    auto r241 = sr.shift();
    REQUIRE(r241 == 0b1111110011101100);

    sr = shift_register<16>(a.data() + 130, 13, 2, 16);
    auto r242 = sr.shift();
    REQUIRE(r242 == 0b1111100111011000);

    sr = shift_register<16>(a.data() + 130, 12, 3, 16);
    auto r243 = sr.shift();
    REQUIRE(r243 == 0b1111001110110001);

    sr = shift_register<16>(a.data() + 130, 11, 4, 16);
    auto r244 = sr.shift();
    REQUIRE(r244 == 0b1110011101100011);

    sr = shift_register<16>(a.data() + 130, 10, 5, 16);
    auto r245 = sr.shift();
    REQUIRE(r245 == 0b1100111011000110);

    sr = shift_register<16>(a.data() + 130, 9, 6, 16);
    auto r246 = sr.shift();
    REQUIRE(r246 == 0b1001110110001101);

    sr = shift_register<16>(a.data() + 130, 8, 7, 16);
    auto r247 = sr.shift();
    REQUIRE(r247 == 0b0011101100011010);

    sr = shift_register<16>(a.data() + 130, 7, 8, 16);
    auto r248 = sr.shift();
    REQUIRE(r248 == 0b0111011000110101);

    sr = shift_register<16>(a.data() + 130, 6, 9, 16);
    auto r249 = sr.shift();
    REQUIRE(r249 == 0b1110110001101010);

    sr = shift_register<16>(a.data() + 130, 5, 10, 16);
    auto r250 = sr.shift();
    REQUIRE(r250 == 0b1101100011010100);

    sr = shift_register<16>(a.data() + 130, 4, 11, 16);
    auto r251 = sr.shift();
    REQUIRE(r251 == 0b1011000110101000);

    sr = shift_register<16>(a.data() + 130, 3, 12, 16);
    auto r252 = sr.shift();
    REQUIRE(r252 == 0b0110001101010001);

    sr = shift_register<16>(a.data() + 130, 2, 13, 16);
    auto r253 = sr.shift();
    REQUIRE(r253 == 0b1100011010100011);

    sr = shift_register<16>(a.data() + 130, 1, 14, 16);
    auto r254 = sr.shift();
    REQUIRE(r254 == 0b1000110101000111);

    sr = shift_register<16>(a.data() + 138, 13, 0, 16);
    auto r255 = sr.shift();
    REQUIRE(r255 == 0b1001100011000111);

    sr = shift_register<16>(a.data() + 138, 12, 1, 16);
    auto r256 = sr.shift();
    REQUIRE(r256 == 0b0011000110001110);

    sr = shift_register<16>(a.data() + 138, 11, 2, 16);
    auto r257 = sr.shift();
    REQUIRE(r257 == 0b0110001100011101);

    sr = shift_register<16>(a.data() + 138, 10, 3, 16);
    auto r258 = sr.shift();
    REQUIRE(r258 == 0b1100011000111011);

    sr = shift_register<16>(a.data() + 138, 9, 4, 16);
    auto r259 = sr.shift();
    REQUIRE(r259 == 0b1000110001110111);

    sr = shift_register<16>(a.data() + 138, 8, 5, 16);
    auto r260 = sr.shift();
    REQUIRE(r260 == 0b0001100011101110);

    sr = shift_register<16>(a.data() + 138, 7, 6, 16);
    auto r261 = sr.shift();
    REQUIRE(r261 == 0b0011000111011100);

    sr = shift_register<16>(a.data() + 138, 6, 7, 16);
    auto r262 = sr.shift();
    REQUIRE(r262 == 0b0110001110111000);

    sr = shift_register<16>(a.data() + 138, 5, 8, 16);
    auto r263 = sr.shift();
    REQUIRE(r263 == 0b1100011101110001);

    sr = shift_register<16>(a.data() + 138, 4, 9, 16);
    auto r264 = sr.shift();
    REQUIRE(r264 == 0b1000111011100010);

    sr = shift_register<16>(a.data() + 138, 3, 10, 16);
    auto r265 = sr.shift();
    REQUIRE(r265 == 0b0001110111000100);

    sr = shift_register<16>(a.data() + 138, 2, 11, 16);
    auto r266 = sr.shift();
    REQUIRE(r266 == 0b0011101110001000);

    sr = shift_register<16>(a.data() + 138, 1, 12, 16);
    auto r267 = sr.shift();
    REQUIRE(r267 == 0b0111011100010000);

    sr = shift_register<16>(a.data() + 138, 0, 13, 16);
    auto r268 = sr.shift();
    REQUIRE(r268 == 0b1110111000100001);

    sr = shift_register<16>(a.data() + 139, 15, 14, 16);
    auto r269 = sr.shift();
    REQUIRE(r269 == 0b1101110001000011);

    sr = shift_register<16>(a.data() + 146, 11, 0, 16);
    auto r270 = sr.shift();
    REQUIRE(r270 == 0b0100010101010100);

    sr = shift_register<16>(a.data() + 146, 10, 1, 16);
    auto r271 = sr.shift();
    REQUIRE(r271 == 0b1000101010101001);

    sr = shift_register<16>(a.data() + 146, 9, 2, 16);
    auto r272 = sr.shift();
    REQUIRE(r272 == 0b0001010101010010);

    sr = shift_register<16>(a.data() + 146, 8, 3, 16);
    auto r273 = sr.shift();
    REQUIRE(r273 == 0b0010101010100101);

    sr = shift_register<16>(a.data() + 146, 7, 4, 16);
    auto r274 = sr.shift();
    REQUIRE(r274 == 0b0101010101001011);

    sr = shift_register<16>(a.data() + 146, 6, 5, 16);
    auto r275 = sr.shift();
    REQUIRE(r275 == 0b1010101010010110);

    sr = shift_register<16>(a.data() + 146, 5, 6, 16);
    auto r276 = sr.shift();
    REQUIRE(r276 == 0b0101010100101100);

    sr = shift_register<16>(a.data() + 146, 4, 7, 16);
    auto r277 = sr.shift();
    REQUIRE(r277 == 0b1010101001011001);

    sr = shift_register<16>(a.data() + 146, 3, 8, 16);
    auto r278 = sr.shift();
    REQUIRE(r278 == 0b0101010010110010);

    sr = shift_register<16>(a.data() + 146, 2, 9, 16);
    auto r279 = sr.shift();
    REQUIRE(r279 == 0b1010100101100101);

    sr = shift_register<16>(a.data() + 146, 1, 10, 16);
    auto r280 = sr.shift();
    REQUIRE(r280 == 0b0101001011001010);

    sr = shift_register<16>(a.data() + 146, 0, 11, 16);
    auto r281 = sr.shift();
    REQUIRE(r281 == 0b1010010110010100);

    sr = shift_register<16>(a.data() + 147, 15, 12, 16);
    auto r282 = sr.shift();
    REQUIRE(r282 == 0b0100101100101000);

    sr = shift_register<16>(a.data() + 147, 14, 13, 16);
    auto r283 = sr.shift();
    REQUIRE(r283 == 0b1001011001010000);

    sr = shift_register<16>(a.data() + 147, 13, 14, 16);
    auto r284 = sr.shift();
    REQUIRE(r284 == 0b0010110010100000);

    sr = shift_register<16>(a.data() + 154, 9, 0, 16);
    auto r285 = sr.shift();
    REQUIRE(r285 == 0b0110010101111100);

    sr = shift_register<16>(a.data() + 154, 8, 1, 16);
    auto r286 = sr.shift();
    REQUIRE(r286 == 0b1100101011111000);

    sr = shift_register<16>(a.data() + 154, 7, 2, 16);
    auto r287 = sr.shift();
    REQUIRE(r287 == 0b1001010111110000);

    sr = shift_register<16>(a.data() + 154, 6, 3, 16);
    auto r288 = sr.shift();
    REQUIRE(r288 == 0b0010101111100001);

    sr = shift_register<16>(a.data() + 154, 5, 4, 16);
    auto r289 = sr.shift();
    REQUIRE(r289 == 0b0101011111000011);

    sr = shift_register<16>(a.data() + 154, 4, 5, 16);
    auto r290 = sr.shift();
    REQUIRE(r290 == 0b1010111110000111);

    sr = shift_register<16>(a.data() + 154, 3, 6, 16);
    auto r291 = sr.shift();
    REQUIRE(r291 == 0b0101111100001110);

    sr = shift_register<16>(a.data() + 154, 2, 7, 16);
    auto r292 = sr.shift();
    REQUIRE(r292 == 0b1011111000011101);

    sr = shift_register<16>(a.data() + 154, 1, 8, 16);
    auto r293 = sr.shift();
    REQUIRE(r293 == 0b0111110000111011);

    sr = shift_register<16>(a.data() + 154, 0, 9, 16);
    auto r294 = sr.shift();
    REQUIRE(r294 == 0b1111100001110111);

    sr = shift_register<16>(a.data() + 155, 15, 10, 16);
    auto r295 = sr.shift();
    REQUIRE(r295 == 0b1111000011101110);

    sr = shift_register<16>(a.data() + 155, 14, 11, 16);
    auto r296 = sr.shift();
    REQUIRE(r296 == 0b1110000111011100);

    sr = shift_register<16>(a.data() + 155, 13, 12, 16);
    auto r297 = sr.shift();
    REQUIRE(r297 == 0b1100001110111000);

    sr = shift_register<16>(a.data() + 155, 12, 13, 16);
    auto r298 = sr.shift();
    REQUIRE(r298 == 0b1000011101110001);

    sr = shift_register<16>(a.data() + 155, 11, 14, 16);
    auto r299 = sr.shift();
    REQUIRE(r299 == 0b0000111011100010);

    sr = shift_register<16>(a.data() + 162, 7, 0, 16);
    auto r300 = sr.shift();
    REQUIRE(r300 == 0b0010101110010110);

    sr = shift_register<16>(a.data() + 162, 6, 1, 16);
    auto r301 = sr.shift();
    REQUIRE(r301 == 0b0101011100101101);

    sr = shift_register<16>(a.data() + 162, 5, 2, 16);
    auto r302 = sr.shift();
    REQUIRE(r302 == 0b1010111001011010);

    sr = shift_register<16>(a.data() + 162, 4, 3, 16);
    auto r303 = sr.shift();
    REQUIRE(r303 == 0b0101110010110100);

    sr = shift_register<16>(a.data() + 162, 3, 4, 16);
    auto r304 = sr.shift();
    REQUIRE(r304 == 0b1011100101101000);

    sr = shift_register<16>(a.data() + 162, 2, 5, 16);
    auto r305 = sr.shift();
    REQUIRE(r305 == 0b0111001011010000);

    sr = shift_register<16>(a.data() + 162, 1, 6, 16);
    auto r306 = sr.shift();
    REQUIRE(r306 == 0b1110010110100000);

    sr = shift_register<16>(a.data() + 162, 0, 7, 16);
    auto r307 = sr.shift();
    REQUIRE(r307 == 0b1100101101000000);

    sr = shift_register<16>(a.data() + 163, 15, 8, 16);
    auto r308 = sr.shift();
    REQUIRE(r308 == 0b1001011010000000);

    sr = shift_register<16>(a.data() + 163, 14, 9, 16);
    auto r309 = sr.shift();
    REQUIRE(r309 == 0b0010110100000000);

    sr = shift_register<16>(a.data() + 163, 13, 10, 16);
    auto r310 = sr.shift();
    REQUIRE(r310 == 0b0101101000000001);

    sr = shift_register<16>(a.data() + 163, 12, 11, 16);
    auto r311 = sr.shift();
    REQUIRE(r311 == 0b1011010000000010);

    sr = shift_register<16>(a.data() + 163, 11, 12, 16);
    auto r312 = sr.shift();
    REQUIRE(r312 == 0b0110100000000101);

    sr = shift_register<16>(a.data() + 163, 10, 13, 16);
    auto r313 = sr.shift();
    REQUIRE(r313 == 0b1101000000001010);

    sr = shift_register<16>(a.data() + 163, 9, 14, 16);
    auto r314 = sr.shift();
    REQUIRE(r314 == 0b1010000000010101);

    sr = shift_register<16>(a.data() + 170, 5, 0, 16);
    auto r315 = sr.shift();
    REQUIRE(r315 == 0b0000111001110101);

    sr = shift_register<16>(a.data() + 170, 4, 1, 16);
    auto r316 = sr.shift();
    REQUIRE(r316 == 0b0001110011101011);

    sr = shift_register<16>(a.data() + 170, 3, 2, 16);
    auto r317 = sr.shift();
    REQUIRE(r317 == 0b0011100111010111);

    sr = shift_register<16>(a.data() + 170, 2, 3, 16);
    auto r318 = sr.shift();
    REQUIRE(r318 == 0b0111001110101110);

    sr = shift_register<16>(a.data() + 170, 1, 4, 16);
    auto r319 = sr.shift();
    REQUIRE(r319 == 0b1110011101011100);

    sr = shift_register<16>(a.data() + 170, 0, 5, 16);
    auto r320 = sr.shift();
    REQUIRE(r320 == 0b1100111010111000);

    sr = shift_register<16>(a.data() + 171, 15, 6, 16);
    auto r321 = sr.shift();
    REQUIRE(r321 == 0b1001110101110000);

    sr = shift_register<16>(a.data() + 171, 14, 7, 16);
    auto r322 = sr.shift();
    REQUIRE(r322 == 0b0011101011100000);

    sr = shift_register<16>(a.data() + 171, 13, 8, 16);
    auto r323 = sr.shift();
    REQUIRE(r323 == 0b0111010111000000);

    sr = shift_register<16>(a.data() + 171, 12, 9, 16);
    auto r324 = sr.shift();
    REQUIRE(r324 == 0b1110101110000001);

    sr = shift_register<16>(a.data() + 171, 11, 10, 16);
    auto r325 = sr.shift();
    REQUIRE(r325 == 0b1101011100000010);

    sr = shift_register<16>(a.data() + 171, 10, 11, 16);
    auto r326 = sr.shift();
    REQUIRE(r326 == 0b1010111000000101);

    sr = shift_register<16>(a.data() + 171, 9, 12, 16);
    auto r327 = sr.shift();
    REQUIRE(r327 == 0b0101110000001010);

    sr = shift_register<16>(a.data() + 171, 8, 13, 16);
    auto r328 = sr.shift();
    REQUIRE(r328 == 0b1011100000010100);

    sr = shift_register<16>(a.data() + 171, 7, 14, 16);
    auto r329 = sr.shift();
    REQUIRE(r329 == 0b0111000000101001);

    sr = shift_register<16>(a.data() + 178, 3, 0, 16);
    auto r330 = sr.shift();
    REQUIRE(r330 == 0b1010000100110010);

    sr = shift_register<16>(a.data() + 178, 2, 1, 16);
    auto r331 = sr.shift();
    REQUIRE(r331 == 0b0100001001100100);

    sr = shift_register<16>(a.data() + 178, 1, 2, 16);
    auto r332 = sr.shift();
    REQUIRE(r332 == 0b1000010011001000);

    sr = shift_register<16>(a.data() + 178, 0, 3, 16);
    auto r333 = sr.shift();
    REQUIRE(r333 == 0b0000100110010000);

    sr = shift_register<16>(a.data() + 179, 15, 4, 16);
    auto r334 = sr.shift();
    REQUIRE(r334 == 0b0001001100100001);

    sr = shift_register<16>(a.data() + 179, 14, 5, 16);
    auto r335 = sr.shift();
    REQUIRE(r335 == 0b0010011001000010);

    sr = shift_register<16>(a.data() + 179, 13, 6, 16);
    auto r336 = sr.shift();
    REQUIRE(r336 == 0b0100110010000101);

    sr = shift_register<16>(a.data() + 179, 12, 7, 16);
    auto r337 = sr.shift();
    REQUIRE(r337 == 0b1001100100001010);

    sr = shift_register<16>(a.data() + 179, 11, 8, 16);
    auto r338 = sr.shift();
    REQUIRE(r338 == 0b0011001000010100);

    sr = shift_register<16>(a.data() + 179, 10, 9, 16);
    auto r339 = sr.shift();
    REQUIRE(r339 == 0b0110010000101000);

    sr = shift_register<16>(a.data() + 179, 9, 10, 16);
    auto r340 = sr.shift();
    REQUIRE(r340 == 0b1100100001010001);

    sr = shift_register<16>(a.data() + 179, 8, 11, 16);
    auto r341 = sr.shift();
    REQUIRE(r341 == 0b1001000010100011);

    sr = shift_register<16>(a.data() + 179, 7, 12, 16);
    auto r342 = sr.shift();
    REQUIRE(r342 == 0b0010000101000111);

    sr = shift_register<16>(a.data() + 179, 6, 13, 16);
    auto r343 = sr.shift();
    REQUIRE(r343 == 0b0100001010001111);

    sr = shift_register<16>(a.data() + 179, 5, 14, 16);
    auto r344 = sr.shift();
    REQUIRE(r344 == 0b1000010100011110);

    sr = shift_register<16>(a.data() + 186, 1, 0, 16);
    auto r345 = sr.shift();
    REQUIRE(r345 == 0b0111100111101010);

    sr = shift_register<16>(a.data() + 186, 0, 1, 16);
    auto r346 = sr.shift();
    REQUIRE(r346 == 0b1111001111010100);

    sr = shift_register<16>(a.data() + 187, 15, 2, 16);
    auto r347 = sr.shift();
    REQUIRE(r347 == 0b1110011110101000);

    sr = shift_register<16>(a.data() + 187, 14, 3, 16);
    auto r348 = sr.shift();
    REQUIRE(r348 == 0b1100111101010001);

    sr = shift_register<16>(a.data() + 187, 13, 4, 16);
    auto r349 = sr.shift();
    REQUIRE(r349 == 0b1001111010100011);

    sr = shift_register<16>(a.data() + 187, 12, 5, 16);
    auto r350 = sr.shift();
    REQUIRE(r350 == 0b0011110101000111);

    sr = shift_register<16>(a.data() + 187, 11, 6, 16);
    auto r351 = sr.shift();
    REQUIRE(r351 == 0b0111101010001111);

    sr = shift_register<16>(a.data() + 187, 10, 7, 16);
    auto r352 = sr.shift();
    REQUIRE(r352 == 0b1111010100011111);

    sr = shift_register<16>(a.data() + 187, 9, 8, 16);
    auto r353 = sr.shift();
    REQUIRE(r353 == 0b1110101000111111);

    sr = shift_register<16>(a.data() + 187, 8, 9, 16);
    auto r354 = sr.shift();
    REQUIRE(r354 == 0b1101010001111110);

    sr = shift_register<16>(a.data() + 187, 7, 10, 16);
    auto r355 = sr.shift();
    REQUIRE(r355 == 0b1010100011111101);

    sr = shift_register<16>(a.data() + 187, 6, 11, 16);
    auto r356 = sr.shift();
    REQUIRE(r356 == 0b0101000111111010);

    sr = shift_register<16>(a.data() + 187, 5, 12, 16);
    auto r357 = sr.shift();
    REQUIRE(r357 == 0b1010001111110101);

    sr = shift_register<16>(a.data() + 187, 4, 13, 16);
    auto r358 = sr.shift();
    REQUIRE(r358 == 0b0100011111101011);

    sr = shift_register<16>(a.data() + 187, 3, 14, 16);
    auto r359 = sr.shift();
    REQUIRE(r359 == 0b1000111111010111);

    sr = shift_register<16>(a.data() + 195, 15, 0, 16);
    auto r360 = sr.shift();
    REQUIRE(r360 == 0b1111011011001010);

    sr = shift_register<16>(a.data() + 195, 14, 1, 16);
    auto r361 = sr.shift();
    REQUIRE(r361 == 0b1110110110010100);

    sr = shift_register<16>(a.data() + 195, 13, 2, 16);
    auto r362 = sr.shift();
    REQUIRE(r362 == 0b1101101100101000);

    sr = shift_register<16>(a.data() + 195, 12, 3, 16);
    auto r363 = sr.shift();
    REQUIRE(r363 == 0b1011011001010001);

    sr = shift_register<16>(a.data() + 195, 11, 4, 16);
    auto r364 = sr.shift();
    REQUIRE(r364 == 0b0110110010100010);

    sr = shift_register<16>(a.data() + 195, 10, 5, 16);
    auto r365 = sr.shift();
    REQUIRE(r365 == 0b1101100101000100);

    sr = shift_register<16>(a.data() + 195, 9, 6, 16);
    auto r366 = sr.shift();
    REQUIRE(r366 == 0b1011001010001000);

    sr = shift_register<16>(a.data() + 195, 8, 7, 16);
    auto r367 = sr.shift();
    REQUIRE(r367 == 0b0110010100010000);

    sr = shift_register<16>(a.data() + 195, 7, 8, 16);
    auto r368 = sr.shift();
    REQUIRE(r368 == 0b1100101000100001);

    sr = shift_register<16>(a.data() + 195, 6, 9, 16);
    auto r369 = sr.shift();
    REQUIRE(r369 == 0b1001010001000011);

    sr = shift_register<16>(a.data() + 195, 5, 10, 16);
    auto r370 = sr.shift();
    REQUIRE(r370 == 0b0010100010000110);

    sr = shift_register<16>(a.data() + 195, 4, 11, 16);
    auto r371 = sr.shift();
    REQUIRE(r371 == 0b0101000100001100);

    sr = shift_register<16>(a.data() + 195, 3, 12, 16);
    auto r372 = sr.shift();
    REQUIRE(r372 == 0b1010001000011001);

    sr = shift_register<16>(a.data() + 195, 2, 13, 16);
    auto r373 = sr.shift();
    REQUIRE(r373 == 0b0100010000110011);

    sr = shift_register<16>(a.data() + 195, 1, 14, 16);
    auto r374 = sr.shift();
    REQUIRE(r374 == 0b1000100001100110);

    sr = shift_register<16>(a.data() + 203, 13, 0, 16);
    auto r375 = sr.shift();
    REQUIRE(r375 == 0b0100010110101100);

    sr = shift_register<16>(a.data() + 203, 12, 1, 16);
    auto r376 = sr.shift();
    REQUIRE(r376 == 0b1000101101011000);

    sr = shift_register<16>(a.data() + 203, 11, 2, 16);
    auto r377 = sr.shift();
    REQUIRE(r377 == 0b0001011010110001);

    sr = shift_register<16>(a.data() + 203, 10, 3, 16);
    auto r378 = sr.shift();
    REQUIRE(r378 == 0b0010110101100010);

    sr = shift_register<16>(a.data() + 203, 9, 4, 16);
    auto r379 = sr.shift();
    REQUIRE(r379 == 0b0101101011000100);

    sr = shift_register<16>(a.data() + 203, 8, 5, 16);
    auto r380 = sr.shift();
    REQUIRE(r380 == 0b1011010110001001);

    sr = shift_register<16>(a.data() + 203, 7, 6, 16);
    auto r381 = sr.shift();
    REQUIRE(r381 == 0b0110101100010011);

    sr = shift_register<16>(a.data() + 203, 6, 7, 16);
    auto r382 = sr.shift();
    REQUIRE(r382 == 0b1101011000100111);

    sr = shift_register<16>(a.data() + 203, 5, 8, 16);
    auto r383 = sr.shift();
    REQUIRE(r383 == 0b1010110001001111);

    sr = shift_register<16>(a.data() + 203, 4, 9, 16);
    auto r384 = sr.shift();
    REQUIRE(r384 == 0b0101100010011110);

    sr = shift_register<16>(a.data() + 203, 3, 10, 16);
    auto r385 = sr.shift();
    REQUIRE(r385 == 0b1011000100111101);

    sr = shift_register<16>(a.data() + 203, 2, 11, 16);
    auto r386 = sr.shift();
    REQUIRE(r386 == 0b0110001001111011);

    sr = shift_register<16>(a.data() + 203, 1, 12, 16);
    auto r387 = sr.shift();
    REQUIRE(r387 == 0b1100010011110111);

    sr = shift_register<16>(a.data() + 203, 0, 13, 16);
    auto r388 = sr.shift();
    REQUIRE(r388 == 0b1000100111101110);

    sr = shift_register<16>(a.data() + 204, 15, 14, 16);
    auto r389 = sr.shift();
    REQUIRE(r389 == 0b0001001111011100);

    sr = shift_register<16>(a.data() + 211, 11, 0, 16);
    auto r390 = sr.shift();
    REQUIRE(r390 == 0b1110001100001010);

    sr = shift_register<16>(a.data() + 211, 10, 1, 16);
    auto r391 = sr.shift();
    REQUIRE(r391 == 0b1100011000010100);

    sr = shift_register<16>(a.data() + 211, 9, 2, 16);
    auto r392 = sr.shift();
    REQUIRE(r392 == 0b1000110000101000);

    sr = shift_register<16>(a.data() + 211, 8, 3, 16);
    auto r393 = sr.shift();
    REQUIRE(r393 == 0b0001100001010000);

    sr = shift_register<16>(a.data() + 211, 7, 4, 16);
    auto r394 = sr.shift();
    REQUIRE(r394 == 0b0011000010100000);

    sr = shift_register<16>(a.data() + 211, 6, 5, 16);
    auto r395 = sr.shift();
    REQUIRE(r395 == 0b0110000101000001);

    sr = shift_register<16>(a.data() + 211, 5, 6, 16);
    auto r396 = sr.shift();
    REQUIRE(r396 == 0b1100001010000011);

    sr = shift_register<16>(a.data() + 211, 4, 7, 16);
    auto r397 = sr.shift();
    REQUIRE(r397 == 0b1000010100000111);

    sr = shift_register<16>(a.data() + 211, 3, 8, 16);
    auto r398 = sr.shift();
    REQUIRE(r398 == 0b0000101000001110);

    sr = shift_register<16>(a.data() + 211, 2, 9, 16);
    auto r399 = sr.shift();
    REQUIRE(r399 == 0b0001010000011101);

    sr = shift_register<16>(a.data() + 211, 1, 10, 16);
    auto r400 = sr.shift();
    REQUIRE(r400 == 0b0010100000111010);

    sr = shift_register<16>(a.data() + 211, 0, 11, 16);
    auto r401 = sr.shift();
    REQUIRE(r401 == 0b0101000001110101);

    sr = shift_register<16>(a.data() + 212, 15, 12, 16);
    auto r402 = sr.shift();
    REQUIRE(r402 == 0b1010000011101010);

    sr = shift_register<16>(a.data() + 212, 14, 13, 16);
    auto r403 = sr.shift();
    REQUIRE(r403 == 0b0100000111010101);

    sr = shift_register<16>(a.data() + 212, 13, 14, 16);
    auto r404 = sr.shift();
    REQUIRE(r404 == 0b1000001110101011);

    sr = shift_register<16>(a.data() + 219, 9, 0, 16);
    auto r405 = sr.shift();
    REQUIRE(r405 == 0b0101000001000101);

    sr = shift_register<16>(a.data() + 219, 8, 1, 16);
    auto r406 = sr.shift();
    REQUIRE(r406 == 0b1010000010001010);

    sr = shift_register<16>(a.data() + 219, 7, 2, 16);
    auto r407 = sr.shift();
    REQUIRE(r407 == 0b0100000100010100);

    sr = shift_register<16>(a.data() + 219, 6, 3, 16);
    auto r408 = sr.shift();
    REQUIRE(r408 == 0b1000001000101000);

    sr = shift_register<16>(a.data() + 219, 5, 4, 16);
    auto r409 = sr.shift();
    REQUIRE(r409 == 0b0000010001010001);

    sr = shift_register<16>(a.data() + 219, 4, 5, 16);
    auto r410 = sr.shift();
    REQUIRE(r410 == 0b0000100010100011);

    sr = shift_register<16>(a.data() + 219, 3, 6, 16);
    auto r411 = sr.shift();
    REQUIRE(r411 == 0b0001000101000111);

    sr = shift_register<16>(a.data() + 219, 2, 7, 16);
    auto r412 = sr.shift();
    REQUIRE(r412 == 0b0010001010001110);

    sr = shift_register<16>(a.data() + 219, 1, 8, 16);
    auto r413 = sr.shift();
    REQUIRE(r413 == 0b0100010100011101);

    sr = shift_register<16>(a.data() + 219, 0, 9, 16);
    auto r414 = sr.shift();
    REQUIRE(r414 == 0b1000101000111010);

    sr = shift_register<16>(a.data() + 220, 15, 10, 16);
    auto r415 = sr.shift();
    REQUIRE(r415 == 0b0001010001110100);

    sr = shift_register<16>(a.data() + 220, 14, 11, 16);
    auto r416 = sr.shift();
    REQUIRE(r416 == 0b0010100011101001);

    sr = shift_register<16>(a.data() + 220, 13, 12, 16);
    auto r417 = sr.shift();
    REQUIRE(r417 == 0b0101000111010010);

    sr = shift_register<16>(a.data() + 220, 12, 13, 16);
    auto r418 = sr.shift();
    REQUIRE(r418 == 0b1010001110100101);

    sr = shift_register<16>(a.data() + 220, 11, 14, 16);
    auto r419 = sr.shift();
    REQUIRE(r419 == 0b0100011101001011);

    sr = shift_register<16>(a.data() + 227, 7, 0, 16);
    auto r420 = sr.shift();
    REQUIRE(r420 == 0b0111100111100011);

    sr = shift_register<16>(a.data() + 227, 6, 1, 16);
    auto r421 = sr.shift();
    REQUIRE(r421 == 0b1111001111000111);

    sr = shift_register<16>(a.data() + 227, 5, 2, 16);
    auto r422 = sr.shift();
    REQUIRE(r422 == 0b1110011110001110);

    sr = shift_register<16>(a.data() + 227, 4, 3, 16);
    auto r423 = sr.shift();
    REQUIRE(r423 == 0b1100111100011100);

    sr = shift_register<16>(a.data() + 227, 3, 4, 16);
    auto r424 = sr.shift();
    REQUIRE(r424 == 0b1001111000111001);

    sr = shift_register<16>(a.data() + 227, 2, 5, 16);
    auto r425 = sr.shift();
    REQUIRE(r425 == 0b0011110001110011);

    sr = shift_register<16>(a.data() + 227, 1, 6, 16);
    auto r426 = sr.shift();
    REQUIRE(r426 == 0b0111100011100111);

    sr = shift_register<16>(a.data() + 227, 0, 7, 16);
    auto r427 = sr.shift();
    REQUIRE(r427 == 0b1111000111001110);

    sr = shift_register<16>(a.data() + 228, 15, 8, 16);
    auto r428 = sr.shift();
    REQUIRE(r428 == 0b1110001110011100);

    sr = shift_register<16>(a.data() + 228, 14, 9, 16);
    auto r429 = sr.shift();
    REQUIRE(r429 == 0b1100011100111001);

    sr = shift_register<16>(a.data() + 228, 13, 10, 16);
    auto r430 = sr.shift();
    REQUIRE(r430 == 0b1000111001110010);

    sr = shift_register<16>(a.data() + 228, 12, 11, 16);
    auto r431 = sr.shift();
    REQUIRE(r431 == 0b0001110011100100);

    sr = shift_register<16>(a.data() + 228, 11, 12, 16);
    auto r432 = sr.shift();
    REQUIRE(r432 == 0b0011100111001001);

    sr = shift_register<16>(a.data() + 228, 10, 13, 16);
    auto r433 = sr.shift();
    REQUIRE(r433 == 0b0111001110010010);

    sr = shift_register<16>(a.data() + 228, 9, 14, 16);
    auto r434 = sr.shift();
    REQUIRE(r434 == 0b1110011100100101);

    sr = shift_register<16>(a.data() + 235, 5, 0, 16);
    auto r435 = sr.shift();
    REQUIRE(r435 == 0b0011110011001001);

    sr = shift_register<16>(a.data() + 235, 4, 1, 16);
    auto r436 = sr.shift();
    REQUIRE(r436 == 0b0111100110010010);

    sr = shift_register<16>(a.data() + 235, 3, 2, 16);
    auto r437 = sr.shift();
    REQUIRE(r437 == 0b1111001100100100);

    sr = shift_register<16>(a.data() + 235, 2, 3, 16);
    auto r438 = sr.shift();
    REQUIRE(r438 == 0b1110011001001000);

    sr = shift_register<16>(a.data() + 235, 1, 4, 16);
    auto r439 = sr.shift();
    REQUIRE(r439 == 0b1100110010010000);

    sr = shift_register<16>(a.data() + 235, 0, 5, 16);
    auto r440 = sr.shift();
    REQUIRE(r440 == 0b1001100100100001);

    sr = shift_register<16>(a.data() + 236, 15, 6, 16);
    auto r441 = sr.shift();
    REQUIRE(r441 == 0b0011001001000010);

    sr = shift_register<16>(a.data() + 236, 14, 7, 16);
    auto r442 = sr.shift();
    REQUIRE(r442 == 0b0110010010000100);

    sr = shift_register<16>(a.data() + 236, 13, 8, 16);
    auto r443 = sr.shift();
    REQUIRE(r443 == 0b1100100100001000);

    sr = shift_register<16>(a.data() + 236, 12, 9, 16);
    auto r444 = sr.shift();
    REQUIRE(r444 == 0b1001001000010000);

    sr = shift_register<16>(a.data() + 236, 11, 10, 16);
    auto r445 = sr.shift();
    REQUIRE(r445 == 0b0010010000100001);

    sr = shift_register<16>(a.data() + 236, 10, 11, 16);
    auto r446 = sr.shift();
    REQUIRE(r446 == 0b0100100001000010);

    sr = shift_register<16>(a.data() + 236, 9, 12, 16);
    auto r447 = sr.shift();
    REQUIRE(r447 == 0b1001000010000100);

    sr = shift_register<16>(a.data() + 236, 8, 13, 16);
    auto r448 = sr.shift();
    REQUIRE(r448 == 0b0010000100001001);

    sr = shift_register<16>(a.data() + 236, 7, 14, 16);
    auto r449 = sr.shift();
    REQUIRE(r449 == 0b0100001000010011);

    sr = shift_register<16>(a.data() + 243, 3, 0, 16);
    auto r450 = sr.shift();
    REQUIRE(r450 == 0b0010100001100000);

    sr = shift_register<16>(a.data() + 243, 2, 1, 16);
    auto r451 = sr.shift();
    REQUIRE(r451 == 0b0101000011000001);

    sr = shift_register<16>(a.data() + 243, 1, 2, 16);
    auto r452 = sr.shift();
    REQUIRE(r452 == 0b1010000110000011);

    sr = shift_register<16>(a.data() + 243, 0, 3, 16);
    auto r453 = sr.shift();
    REQUIRE(r453 == 0b0100001100000111);

    sr = shift_register<16>(a.data() + 244, 15, 4, 16);
    auto r454 = sr.shift();
    REQUIRE(r454 == 0b1000011000001111);

    sr = shift_register<16>(a.data() + 244, 14, 5, 16);
    auto r455 = sr.shift();
    REQUIRE(r455 == 0b0000110000011111);

    sr = shift_register<16>(a.data() + 244, 13, 6, 16);
    auto r456 = sr.shift();
    REQUIRE(r456 == 0b0001100000111111);

    sr = shift_register<16>(a.data() + 244, 12, 7, 16);
    auto r457 = sr.shift();
    REQUIRE(r457 == 0b0011000001111110);

    sr = shift_register<16>(a.data() + 244, 11, 8, 16);
    auto r458 = sr.shift();
    REQUIRE(r458 == 0b0110000011111100);

    sr = shift_register<16>(a.data() + 244, 10, 9, 16);
    auto r459 = sr.shift();
    REQUIRE(r459 == 0b1100000111111000);

    sr = shift_register<16>(a.data() + 244, 9, 10, 16);
    auto r460 = sr.shift();
    REQUIRE(r460 == 0b1000001111110001);

    sr = shift_register<16>(a.data() + 244, 8, 11, 16);
    auto r461 = sr.shift();
    REQUIRE(r461 == 0b0000011111100011);

    sr = shift_register<16>(a.data() + 244, 7, 12, 16);
    auto r462 = sr.shift();
    REQUIRE(r462 == 0b0000111111000110);

    sr = shift_register<16>(a.data() + 244, 6, 13, 16);
    auto r463 = sr.shift();
    REQUIRE(r463 == 0b0001111110001101);

    sr = shift_register<16>(a.data() + 244, 5, 14, 16);
    auto r464 = sr.shift();
    REQUIRE(r464 == 0b0011111100011010);

    sr = shift_register<16>(a.data() + 251, 1, 0, 16);
    auto r465 = sr.shift();
    REQUIRE(r465 == 0b0001001101010100);

    sr = shift_register<16>(a.data() + 251, 0, 1, 16);
    auto r466 = sr.shift();
    REQUIRE(r466 == 0b0010011010101001);

    sr = shift_register<16>(a.data() + 252, 15, 2, 16);
    auto r467 = sr.shift();
    REQUIRE(r467 == 0b0100110101010010);

    sr = shift_register<16>(a.data() + 252, 14, 3, 16);
    auto r468 = sr.shift();
    REQUIRE(r468 == 0b1001101010100100);

    sr = shift_register<16>(a.data() + 252, 13, 4, 16);
    auto r469 = sr.shift();
    REQUIRE(r469 == 0b0011010101001001);

    sr = shift_register<16>(a.data() + 252, 12, 5, 16);
    auto r470 = sr.shift();
    REQUIRE(r470 == 0b0110101010010011);

    sr = shift_register<16>(a.data() + 252, 11, 6, 16);
    auto r471 = sr.shift();
    REQUIRE(r471 == 0b1101010100100110);

    sr = shift_register<16>(a.data() + 252, 10, 7, 16);
    auto r472 = sr.shift();
    REQUIRE(r472 == 0b1010101001001101);

    sr = shift_register<16>(a.data() + 252, 9, 8, 16);
    auto r473 = sr.shift();
    REQUIRE(r473 == 0b0101010010011010);

    sr = shift_register<16>(a.data() + 252, 8, 9, 16);
    auto r474 = sr.shift();
    REQUIRE(r474 == 0b1010100100110101);

    sr = shift_register<16>(a.data() + 252, 7, 10, 16);
    auto r475 = sr.shift();
    REQUIRE(r475 == 0b0101001001101011);

    sr = shift_register<16>(a.data() + 252, 6, 11, 16);
    auto r476 = sr.shift();
    REQUIRE(r476 == 0b1010010011010111);

    sr = shift_register<16>(a.data() + 252, 5, 12, 16);
    auto r477 = sr.shift();
    REQUIRE(r477 == 0b0100100110101111);

    sr = shift_register<16>(a.data() + 252, 4, 13, 16);
    auto r478 = sr.shift();
    REQUIRE(r478 == 0b1001001101011110);

    sr = shift_register<16>(a.data() + 252, 3, 14, 16);
    auto r479 = sr.shift();
    REQUIRE(r479 == 0b0010011010111101);

    sr = shift_register<16>(a.data() + 260, 15, 0, 16);
    auto r480 = sr.shift();
    REQUIRE(r480 == 0b1101010110101111);

    sr = shift_register<16>(a.data() + 260, 14, 1, 16);
    auto r481 = sr.shift();
    REQUIRE(r481 == 0b1010101101011111);

    sr = shift_register<16>(a.data() + 260, 13, 2, 16);
    auto r482 = sr.shift();
    REQUIRE(r482 == 0b0101011010111111);

    sr = shift_register<16>(a.data() + 260, 12, 3, 16);
    auto r483 = sr.shift();
    REQUIRE(r483 == 0b1010110101111111);

    sr = shift_register<16>(a.data() + 260, 11, 4, 16);
    auto r484 = sr.shift();
    REQUIRE(r484 == 0b0101101011111111);

    sr = shift_register<16>(a.data() + 260, 10, 5, 16);
    auto r485 = sr.shift();
    REQUIRE(r485 == 0b1011010111111111);

    sr = shift_register<16>(a.data() + 260, 9, 6, 16);
    auto r486 = sr.shift();
    REQUIRE(r486 == 0b0110101111111110);

    sr = shift_register<16>(a.data() + 260, 8, 7, 16);
    auto r487 = sr.shift();
    REQUIRE(r487 == 0b1101011111111101);

    sr = shift_register<16>(a.data() + 260, 7, 8, 16);
    auto r488 = sr.shift();
    REQUIRE(r488 == 0b1010111111111010);

    sr = shift_register<16>(a.data() + 260, 6, 9, 16);
    auto r489 = sr.shift();
    REQUIRE(r489 == 0b0101111111110100);

    sr = shift_register<16>(a.data() + 260, 5, 10, 16);
    auto r490 = sr.shift();
    REQUIRE(r490 == 0b1011111111101000);

    sr = shift_register<16>(a.data() + 260, 4, 11, 16);
    auto r491 = sr.shift();
    REQUIRE(r491 == 0b0111111111010000);

    sr = shift_register<16>(a.data() + 260, 3, 12, 16);
    auto r492 = sr.shift();
    REQUIRE(r492 == 0b1111111110100000);

    sr = shift_register<16>(a.data() + 260, 2, 13, 16);
    auto r493 = sr.shift();
    REQUIRE(r493 == 0b1111111101000000);

    sr = shift_register<16>(a.data() + 260, 1, 14, 16);
    auto r494 = sr.shift();
    REQUIRE(r494 == 0b1111111010000001);

    sr = shift_register<16>(a.data() + 268, 13, 0, 16);
    auto r495 = sr.shift();
    REQUIRE(r495 == 0b0101010000001100);

    sr = shift_register<16>(a.data() + 268, 12, 1, 16);
    auto r496 = sr.shift();
    REQUIRE(r496 == 0b1010100000011001);

    sr = shift_register<16>(a.data() + 268, 11, 2, 16);
    auto r497 = sr.shift();
    REQUIRE(r497 == 0b0101000000110010);

    sr = shift_register<16>(a.data() + 268, 10, 3, 16);
    auto r498 = sr.shift();
    REQUIRE(r498 == 0b1010000001100101);

    sr = shift_register<16>(a.data() + 268, 9, 4, 16);
    auto r499 = sr.shift();
    REQUIRE(r499 == 0b0100000011001010);

    sr = shift_register<16>(a.data() + 268, 8, 5, 16);
    auto r500 = sr.shift();
    REQUIRE(r500 == 0b1000000110010100);

    sr = shift_register<16>(a.data() + 268, 7, 6, 16);
    auto r501 = sr.shift();
    REQUIRE(r501 == 0b0000001100101001);

    sr = shift_register<16>(a.data() + 268, 6, 7, 16);
    auto r502 = sr.shift();
    REQUIRE(r502 == 0b0000011001010010);

    sr = shift_register<16>(a.data() + 268, 5, 8, 16);
    auto r503 = sr.shift();
    REQUIRE(r503 == 0b0000110010100101);

    sr = shift_register<16>(a.data() + 268, 4, 9, 16);
    auto r504 = sr.shift();
    REQUIRE(r504 == 0b0001100101001011);

    sr = shift_register<16>(a.data() + 268, 3, 10, 16);
    auto r505 = sr.shift();
    REQUIRE(r505 == 0b0011001010010111);

    sr = shift_register<16>(a.data() + 268, 2, 11, 16);
    auto r506 = sr.shift();
    REQUIRE(r506 == 0b0110010100101111);

    sr = shift_register<16>(a.data() + 268, 1, 12, 16);
    auto r507 = sr.shift();
    REQUIRE(r507 == 0b1100101001011110);

    sr = shift_register<16>(a.data() + 268, 0, 13, 16);
    auto r508 = sr.shift();
    REQUIRE(r508 == 0b1001010010111100);

    sr = shift_register<16>(a.data() + 269, 15, 14, 16);
    auto r509 = sr.shift();
    REQUIRE(r509 == 0b0010100101111000);

    sr = shift_register<16>(a.data() + 276, 11, 0, 16);
    auto r510 = sr.shift();
    REQUIRE(r510 == 0b0111000000100110);

    sr = shift_register<16>(a.data() + 276, 10, 1, 16);
    auto r511 = sr.shift();
    REQUIRE(r511 == 0b1110000001001100);

    sr = shift_register<16>(a.data() + 276, 9, 2, 16);
    auto r512 = sr.shift();
    REQUIRE(r512 == 0b1100000010011000);

    sr = shift_register<16>(a.data() + 276, 8, 3, 16);
    auto r513 = sr.shift();
    REQUIRE(r513 == 0b1000000100110001);

    sr = shift_register<16>(a.data() + 276, 7, 4, 16);
    auto r514 = sr.shift();
    REQUIRE(r514 == 0b0000001001100010);

    sr = shift_register<16>(a.data() + 276, 6, 5, 16);
    auto r515 = sr.shift();
    REQUIRE(r515 == 0b0000010011000100);

    sr = shift_register<16>(a.data() + 276, 5, 6, 16);
    auto r516 = sr.shift();
    REQUIRE(r516 == 0b0000100110001000);

    sr = shift_register<16>(a.data() + 276, 4, 7, 16);
    auto r517 = sr.shift();
    REQUIRE(r517 == 0b0001001100010000);

    sr = shift_register<16>(a.data() + 276, 3, 8, 16);
    auto r518 = sr.shift();
    REQUIRE(r518 == 0b0010011000100000);

    sr = shift_register<16>(a.data() + 276, 2, 9, 16);
    auto r519 = sr.shift();
    REQUIRE(r519 == 0b0100110001000000);

    sr = shift_register<16>(a.data() + 276, 1, 10, 16);
    auto r520 = sr.shift();
    REQUIRE(r520 == 0b1001100010000000);

    sr = shift_register<16>(a.data() + 276, 0, 11, 16);
    auto r521 = sr.shift();
    REQUIRE(r521 == 0b0011000100000001);

    sr = shift_register<16>(a.data() + 277, 15, 12, 16);
    auto r522 = sr.shift();
    REQUIRE(r522 == 0b0110001000000011);

    sr = shift_register<16>(a.data() + 277, 14, 13, 16);
    auto r523 = sr.shift();
    REQUIRE(r523 == 0b1100010000000110);

    sr = shift_register<16>(a.data() + 277, 13, 14, 16);
    auto r524 = sr.shift();
    REQUIRE(r524 == 0b1000100000001100);

    sr = shift_register<16>(a.data() + 284, 9, 0, 16);
    auto r525 = sr.shift();
    REQUIRE(r525 == 0b1100001101111001);

    sr = shift_register<16>(a.data() + 284, 8, 1, 16);
    auto r526 = sr.shift();
    REQUIRE(r526 == 0b1000011011110010);

    sr = shift_register<16>(a.data() + 284, 7, 2, 16);
    auto r527 = sr.shift();
    REQUIRE(r527 == 0b0000110111100101);

    sr = shift_register<16>(a.data() + 284, 6, 3, 16);
    auto r528 = sr.shift();
    REQUIRE(r528 == 0b0001101111001010);

    sr = shift_register<16>(a.data() + 284, 5, 4, 16);
    auto r529 = sr.shift();
    REQUIRE(r529 == 0b0011011110010101);

    sr = shift_register<16>(a.data() + 284, 4, 5, 16);
    auto r530 = sr.shift();
    REQUIRE(r530 == 0b0110111100101010);

    sr = shift_register<16>(a.data() + 284, 3, 6, 16);
    auto r531 = sr.shift();
    REQUIRE(r531 == 0b1101111001010101);

    sr = shift_register<16>(a.data() + 284, 2, 7, 16);
    auto r532 = sr.shift();
    REQUIRE(r532 == 0b1011110010101010);

    sr = shift_register<16>(a.data() + 284, 1, 8, 16);
    auto r533 = sr.shift();
    REQUIRE(r533 == 0b0111100101010100);

    sr = shift_register<16>(a.data() + 284, 0, 9, 16);
    auto r534 = sr.shift();
    REQUIRE(r534 == 0b1111001010101000);

    sr = shift_register<16>(a.data() + 285, 15, 10, 16);
    auto r535 = sr.shift();
    REQUIRE(r535 == 0b1110010101010001);

    sr = shift_register<16>(a.data() + 285, 14, 11, 16);
    auto r536 = sr.shift();
    REQUIRE(r536 == 0b1100101010100011);

    sr = shift_register<16>(a.data() + 285, 13, 12, 16);
    auto r537 = sr.shift();
    REQUIRE(r537 == 0b1001010101000110);

    sr = shift_register<16>(a.data() + 285, 12, 13, 16);
    auto r538 = sr.shift();
    REQUIRE(r538 == 0b0010101010001101);

    sr = shift_register<16>(a.data() + 285, 11, 14, 16);
    auto r539 = sr.shift();
    REQUIRE(r539 == 0b0101010100011010);

    sr = shift_register<16>(a.data() + 292, 7, 0, 16);
    auto r540 = sr.shift();
    REQUIRE(r540 == 0b0001101101000001);

    sr = shift_register<16>(a.data() + 292, 6, 1, 16);
    auto r541 = sr.shift();
    REQUIRE(r541 == 0b0011011010000011);

    sr = shift_register<16>(a.data() + 292, 5, 2, 16);
    auto r542 = sr.shift();
    REQUIRE(r542 == 0b0110110100000110);

    sr = shift_register<16>(a.data() + 292, 4, 3, 16);
    auto r543 = sr.shift();
    REQUIRE(r543 == 0b1101101000001100);

    sr = shift_register<16>(a.data() + 292, 3, 4, 16);
    auto r544 = sr.shift();
    REQUIRE(r544 == 0b1011010000011001);

    sr = shift_register<16>(a.data() + 292, 2, 5, 16);
    auto r545 = sr.shift();
    REQUIRE(r545 == 0b0110100000110010);

    sr = shift_register<16>(a.data() + 292, 1, 6, 16);
    auto r546 = sr.shift();
    REQUIRE(r546 == 0b1101000001100101);

    sr = shift_register<16>(a.data() + 292, 0, 7, 16);
    auto r547 = sr.shift();
    REQUIRE(r547 == 0b1010000011001011);

    sr = shift_register<16>(a.data() + 293, 15, 8, 16);
    auto r548 = sr.shift();
    REQUIRE(r548 == 0b0100000110010110);

    sr = shift_register<16>(a.data() + 293, 14, 9, 16);
    auto r549 = sr.shift();
    REQUIRE(r549 == 0b1000001100101100);

    sr = shift_register<16>(a.data() + 293, 13, 10, 16);
    auto r550 = sr.shift();
    REQUIRE(r550 == 0b0000011001011000);

    sr = shift_register<16>(a.data() + 293, 12, 11, 16);
    auto r551 = sr.shift();
    REQUIRE(r551 == 0b0000110010110000);

    sr = shift_register<16>(a.data() + 293, 11, 12, 16);
    auto r552 = sr.shift();
    REQUIRE(r552 == 0b0001100101100001);

    sr = shift_register<16>(a.data() + 293, 10, 13, 16);
    auto r553 = sr.shift();
    REQUIRE(r553 == 0b0011001011000010);

    sr = shift_register<16>(a.data() + 293, 9, 14, 16);
    auto r554 = sr.shift();
    REQUIRE(r554 == 0b0110010110000100);

    sr = shift_register<16>(a.data() + 300, 5, 0, 16);
    auto r555 = sr.shift();
    REQUIRE(r555 == 0b0100100111010011);

    sr = shift_register<16>(a.data() + 300, 4, 1, 16);
    auto r556 = sr.shift();
    REQUIRE(r556 == 0b1001001110100111);

    sr = shift_register<16>(a.data() + 300, 3, 2, 16);
    auto r557 = sr.shift();
    REQUIRE(r557 == 0b0010011101001110);

    sr = shift_register<16>(a.data() + 300, 2, 3, 16);
    auto r558 = sr.shift();
    REQUIRE(r558 == 0b0100111010011101);

    sr = shift_register<16>(a.data() + 300, 1, 4, 16);
    auto r559 = sr.shift();
    REQUIRE(r559 == 0b1001110100111011);

    sr = shift_register<16>(a.data() + 300, 0, 5, 16);
    auto r560 = sr.shift();
    REQUIRE(r560 == 0b0011101001110111);

    sr = shift_register<16>(a.data() + 301, 15, 6, 16);
    auto r561 = sr.shift();
    REQUIRE(r561 == 0b0111010011101110);

    sr = shift_register<16>(a.data() + 301, 14, 7, 16);
    auto r562 = sr.shift();
    REQUIRE(r562 == 0b1110100111011101);

    sr = shift_register<16>(a.data() + 301, 13, 8, 16);
    auto r563 = sr.shift();
    REQUIRE(r563 == 0b1101001110111010);

    sr = shift_register<16>(a.data() + 301, 12, 9, 16);
    auto r564 = sr.shift();
    REQUIRE(r564 == 0b1010011101110100);

    sr = shift_register<16>(a.data() + 301, 11, 10, 16);
    auto r565 = sr.shift();
    REQUIRE(r565 == 0b0100111011101000);

    sr = shift_register<16>(a.data() + 301, 10, 11, 16);
    auto r566 = sr.shift();
    REQUIRE(r566 == 0b1001110111010000);

    sr = shift_register<16>(a.data() + 301, 9, 12, 16);
    auto r567 = sr.shift();
    REQUIRE(r567 == 0b0011101110100001);

    sr = shift_register<16>(a.data() + 301, 8, 13, 16);
    auto r568 = sr.shift();
    REQUIRE(r568 == 0b0111011101000011);

    sr = shift_register<16>(a.data() + 301, 7, 14, 16);
    auto r569 = sr.shift();
    REQUIRE(r569 == 0b1110111010000111);

    sr = shift_register<16>(a.data() + 308, 3, 0, 16);
    auto r570 = sr.shift();
    REQUIRE(r570 == 0b0001111111101001);

    sr = shift_register<16>(a.data() + 308, 2, 1, 16);
    auto r571 = sr.shift();
    REQUIRE(r571 == 0b0011111111010011);

    sr = shift_register<16>(a.data() + 308, 1, 2, 16);
    auto r572 = sr.shift();
    REQUIRE(r572 == 0b0111111110100111);

    sr = shift_register<16>(a.data() + 308, 0, 3, 16);
    auto r573 = sr.shift();
    REQUIRE(r573 == 0b1111111101001110);

    sr = shift_register<16>(a.data() + 309, 15, 4, 16);
    auto r574 = sr.shift();
    REQUIRE(r574 == 0b1111111010011101);

    sr = shift_register<16>(a.data() + 309, 14, 5, 16);
    auto r575 = sr.shift();
    REQUIRE(r575 == 0b1111110100111010);

    sr = shift_register<16>(a.data() + 309, 13, 6, 16);
    auto r576 = sr.shift();
    REQUIRE(r576 == 0b1111101001110101);

    sr = shift_register<16>(a.data() + 309, 12, 7, 16);
    auto r577 = sr.shift();
    REQUIRE(r577 == 0b1111010011101010);

    sr = shift_register<16>(a.data() + 309, 11, 8, 16);
    auto r578 = sr.shift();
    REQUIRE(r578 == 0b1110100111010100);

    sr = shift_register<16>(a.data() + 309, 10, 9, 16);
    auto r579 = sr.shift();
    REQUIRE(r579 == 0b1101001110101000);

    sr = shift_register<16>(a.data() + 309, 9, 10, 16);
    auto r580 = sr.shift();
    REQUIRE(r580 == 0b1010011101010000);

    sr = shift_register<16>(a.data() + 309, 8, 11, 16);
    auto r581 = sr.shift();
    REQUIRE(r581 == 0b0100111010100000);

    sr = shift_register<16>(a.data() + 309, 7, 12, 16);
    auto r582 = sr.shift();
    REQUIRE(r582 == 0b1001110101000001);

    sr = shift_register<16>(a.data() + 309, 6, 13, 16);
    auto r583 = sr.shift();
    REQUIRE(r583 == 0b0011101010000011);

    sr = shift_register<16>(a.data() + 309, 5, 14, 16);
    auto r584 = sr.shift();
    REQUIRE(r584 == 0b0111010100000110);

    sr = shift_register<16>(a.data() + 316, 1, 0, 16);
    auto r585 = sr.shift();
    REQUIRE(r585 == 0b1011100010011010);

    sr = shift_register<16>(a.data() + 316, 0, 1, 16);
    auto r586 = sr.shift();
    REQUIRE(r586 == 0b0111000100110101);

    sr = shift_register<16>(a.data() + 317, 15, 2, 16);
    auto r587 = sr.shift();
    REQUIRE(r587 == 0b1110001001101011);

    sr = shift_register<16>(a.data() + 317, 14, 3, 16);
    auto r588 = sr.shift();
    REQUIRE(r588 == 0b1100010011010110);

    sr = shift_register<16>(a.data() + 317, 13, 4, 16);
    auto r589 = sr.shift();
    REQUIRE(r589 == 0b1000100110101101);

    sr = shift_register<16>(a.data() + 317, 12, 5, 16);
    auto r590 = sr.shift();
    REQUIRE(r590 == 0b0001001101011011);

    sr = shift_register<16>(a.data() + 317, 11, 6, 16);
    auto r591 = sr.shift();
    REQUIRE(r591 == 0b0010011010110111);

    sr = shift_register<16>(a.data() + 317, 10, 7, 16);
    auto r592 = sr.shift();
    REQUIRE(r592 == 0b0100110101101110);

    sr = shift_register<16>(a.data() + 317, 9, 8, 16);
    auto r593 = sr.shift();
    REQUIRE(r593 == 0b1001101011011101);

    sr = shift_register<16>(a.data() + 317, 8, 9, 16);
    auto r594 = sr.shift();
    REQUIRE(r594 == 0b0011010110111011);

    sr = shift_register<16>(a.data() + 317, 7, 10, 16);
    auto r595 = sr.shift();
    REQUIRE(r595 == 0b0110101101110110);

    sr = shift_register<16>(a.data() + 317, 6, 11, 16);
    auto r596 = sr.shift();
    REQUIRE(r596 == 0b1101011011101100);

    sr = shift_register<16>(a.data() + 317, 5, 12, 16);
    auto r597 = sr.shift();
    REQUIRE(r597 == 0b1010110111011000);

    sr = shift_register<16>(a.data() + 317, 4, 13, 16);
    auto r598 = sr.shift();
    REQUIRE(r598 == 0b0101101110110001);

    sr = shift_register<16>(a.data() + 317, 3, 14, 16);
    auto r599 = sr.shift();
    REQUIRE(r599 == 0b1011011101100010);

    sr = shift_register<16>(a.data() + 325, 15, 0, 16);
    auto r600 = sr.shift();
    REQUIRE(r600 == 0b1000001111001001);

    sr = shift_register<16>(a.data() + 325, 14, 1, 16);
    auto r601 = sr.shift();
    REQUIRE(r601 == 0b0000011110010010);

    sr = shift_register<16>(a.data() + 325, 13, 2, 16);
    auto r602 = sr.shift();
    REQUIRE(r602 == 0b0000111100100101);

    sr = shift_register<16>(a.data() + 325, 12, 3, 16);
    auto r603 = sr.shift();
    REQUIRE(r603 == 0b0001111001001011);

    sr = shift_register<16>(a.data() + 325, 11, 4, 16);
    auto r604 = sr.shift();
    REQUIRE(r604 == 0b0011110010010111);

    sr = shift_register<16>(a.data() + 325, 10, 5, 16);
    auto r605 = sr.shift();
    REQUIRE(r605 == 0b0111100100101111);

    sr = shift_register<16>(a.data() + 325, 9, 6, 16);
    auto r606 = sr.shift();
    REQUIRE(r606 == 0b1111001001011110);

    sr = shift_register<16>(a.data() + 325, 8, 7, 16);
    auto r607 = sr.shift();
    REQUIRE(r607 == 0b1110010010111101);

    sr = shift_register<16>(a.data() + 325, 7, 8, 16);
    auto r608 = sr.shift();
    REQUIRE(r608 == 0b1100100101111011);

    sr = shift_register<16>(a.data() + 325, 6, 9, 16);
    auto r609 = sr.shift();
    REQUIRE(r609 == 0b1001001011110110);

    sr = shift_register<16>(a.data() + 325, 5, 10, 16);
    auto r610 = sr.shift();
    REQUIRE(r610 == 0b0010010111101100);

    sr = shift_register<16>(a.data() + 325, 4, 11, 16);
    auto r611 = sr.shift();
    REQUIRE(r611 == 0b0100101111011001);

    sr = shift_register<16>(a.data() + 325, 3, 12, 16);
    auto r612 = sr.shift();
    REQUIRE(r612 == 0b1001011110110010);

    sr = shift_register<16>(a.data() + 325, 2, 13, 16);
    auto r613 = sr.shift();
    REQUIRE(r613 == 0b0010111101100101);

    sr = shift_register<16>(a.data() + 325, 1, 14, 16);
    auto r614 = sr.shift();
    REQUIRE(r614 == 0b0101111011001010);

    sr = shift_register<16>(a.data() + 333, 13, 0, 16);
    auto r615 = sr.shift();
    REQUIRE(r615 == 0b0100000000000001);

    sr = shift_register<16>(a.data() + 333, 12, 1, 16);
    auto r616 = sr.shift();
    REQUIRE(r616 == 0b1000000000000010);

    sr = shift_register<16>(a.data() + 333, 11, 2, 16);
    auto r617 = sr.shift();
    REQUIRE(r617 == 0b0000000000000101);

    sr = shift_register<16>(a.data() + 333, 10, 3, 16);
    auto r618 = sr.shift();
    REQUIRE(r618 == 0b0000000000001010);

    sr = shift_register<16>(a.data() + 333, 9, 4, 16);
    auto r619 = sr.shift();
    REQUIRE(r619 == 0b0000000000010101);

    sr = shift_register<16>(a.data() + 333, 8, 5, 16);
    auto r620 = sr.shift();
    REQUIRE(r620 == 0b0000000000101011);

    sr = shift_register<16>(a.data() + 333, 7, 6, 16);
    auto r621 = sr.shift();
    REQUIRE(r621 == 0b0000000001010110);

    sr = shift_register<16>(a.data() + 333, 6, 7, 16);
    auto r622 = sr.shift();
    REQUIRE(r622 == 0b0000000010101100);

    sr = shift_register<16>(a.data() + 333, 5, 8, 16);
    auto r623 = sr.shift();
    REQUIRE(r623 == 0b0000000101011000);

    sr = shift_register<16>(a.data() + 333, 4, 9, 16);
    auto r624 = sr.shift();
    REQUIRE(r624 == 0b0000001010110000);

    sr = shift_register<16>(a.data() + 333, 3, 10, 16);
    auto r625 = sr.shift();
    REQUIRE(r625 == 0b0000010101100001);

    sr = shift_register<16>(a.data() + 333, 2, 11, 16);
    auto r626 = sr.shift();
    REQUIRE(r626 == 0b0000101011000011);

    sr = shift_register<16>(a.data() + 333, 1, 12, 16);
    auto r627 = sr.shift();
    REQUIRE(r627 == 0b0001010110000111);

    sr = shift_register<16>(a.data() + 333, 0, 13, 16);
    auto r628 = sr.shift();
    REQUIRE(r628 == 0b0010101100001111);

    sr = shift_register<16>(a.data() + 334, 15, 14, 16);
    auto r629 = sr.shift();
    REQUIRE(r629 == 0b0101011000011111);

    sr = shift_register<16>(a.data() + 341, 11, 0, 16);
    auto r630 = sr.shift();
    REQUIRE(r630 == 0b0101111111011000);

    sr = shift_register<16>(a.data() + 341, 10, 1, 16);
    auto r631 = sr.shift();
    REQUIRE(r631 == 0b1011111110110000);

    sr = shift_register<16>(a.data() + 341, 9, 2, 16);
    auto r632 = sr.shift();
    REQUIRE(r632 == 0b0111111101100001);

    sr = shift_register<16>(a.data() + 341, 8, 3, 16);
    auto r633 = sr.shift();
    REQUIRE(r633 == 0b1111111011000011);

    sr = shift_register<16>(a.data() + 341, 7, 4, 16);
    auto r634 = sr.shift();
    REQUIRE(r634 == 0b1111110110000110);

    sr = shift_register<16>(a.data() + 341, 6, 5, 16);
    auto r635 = sr.shift();
    REQUIRE(r635 == 0b1111101100001100);

    sr = shift_register<16>(a.data() + 341, 5, 6, 16);
    auto r636 = sr.shift();
    REQUIRE(r636 == 0b1111011000011001);

    sr = shift_register<16>(a.data() + 341, 4, 7, 16);
    auto r637 = sr.shift();
    REQUIRE(r637 == 0b1110110000110011);

    sr = shift_register<16>(a.data() + 341, 3, 8, 16);
    auto r638 = sr.shift();
    REQUIRE(r638 == 0b1101100001100110);

    sr = shift_register<16>(a.data() + 341, 2, 9, 16);
    auto r639 = sr.shift();
    REQUIRE(r639 == 0b1011000011001101);

    sr = shift_register<16>(a.data() + 341, 1, 10, 16);
    auto r640 = sr.shift();
    REQUIRE(r640 == 0b0110000110011010);

    sr = shift_register<16>(a.data() + 341, 0, 11, 16);
    auto r641 = sr.shift();
    REQUIRE(r641 == 0b1100001100110101);

    sr = shift_register<16>(a.data() + 342, 15, 12, 16);
    auto r642 = sr.shift();
    REQUIRE(r642 == 0b1000011001101010);

    sr = shift_register<16>(a.data() + 342, 14, 13, 16);
    auto r643 = sr.shift();
    REQUIRE(r643 == 0b0000110011010100);

    sr = shift_register<16>(a.data() + 342, 13, 14, 16);
    auto r644 = sr.shift();
    REQUIRE(r644 == 0b0001100110101001);

    sr = shift_register<16>(a.data() + 349, 9, 0, 16);
    auto r645 = sr.shift();
    REQUIRE(r645 == 0b0101111110010001);

    sr = shift_register<16>(a.data() + 349, 8, 1, 16);
    auto r646 = sr.shift();
    REQUIRE(r646 == 0b1011111100100011);

    sr = shift_register<16>(a.data() + 349, 7, 2, 16);
    auto r647 = sr.shift();
    REQUIRE(r647 == 0b0111111001000110);

    sr = shift_register<16>(a.data() + 349, 6, 3, 16);
    auto r648 = sr.shift();
    REQUIRE(r648 == 0b1111110010001101);

    sr = shift_register<16>(a.data() + 349, 5, 4, 16);
    auto r649 = sr.shift();
    REQUIRE(r649 == 0b1111100100011010);

    sr = shift_register<16>(a.data() + 349, 4, 5, 16);
    auto r650 = sr.shift();
    REQUIRE(r650 == 0b1111001000110101);

    sr = shift_register<16>(a.data() + 349, 3, 6, 16);
    auto r651 = sr.shift();
    REQUIRE(r651 == 0b1110010001101011);

    sr = shift_register<16>(a.data() + 349, 2, 7, 16);
    auto r652 = sr.shift();
    REQUIRE(r652 == 0b1100100011010110);

    sr = shift_register<16>(a.data() + 349, 1, 8, 16);
    auto r653 = sr.shift();
    REQUIRE(r653 == 0b1001000110101101);

    sr = shift_register<16>(a.data() + 349, 0, 9, 16);
    auto r654 = sr.shift();
    REQUIRE(r654 == 0b0010001101011011);

    sr = shift_register<16>(a.data() + 350, 15, 10, 16);
    auto r655 = sr.shift();
    REQUIRE(r655 == 0b0100011010110110);

    sr = shift_register<16>(a.data() + 350, 14, 11, 16);
    auto r656 = sr.shift();
    REQUIRE(r656 == 0b1000110101101100);

    sr = shift_register<16>(a.data() + 350, 13, 12, 16);
    auto r657 = sr.shift();
    REQUIRE(r657 == 0b0001101011011001);

    sr = shift_register<16>(a.data() + 350, 12, 13, 16);
    auto r658 = sr.shift();
    REQUIRE(r658 == 0b0011010110110011);

    sr = shift_register<16>(a.data() + 350, 11, 14, 16);
    auto r659 = sr.shift();
    REQUIRE(r659 == 0b0110101101100110);

    sr = shift_register<16>(a.data() + 357, 7, 0, 16);
    auto r660 = sr.shift();
    REQUIRE(r660 == 0b0111110110000101);

    sr = shift_register<16>(a.data() + 357, 6, 1, 16);
    auto r661 = sr.shift();
    REQUIRE(r661 == 0b1111101100001010);

    sr = shift_register<16>(a.data() + 357, 5, 2, 16);
    auto r662 = sr.shift();
    REQUIRE(r662 == 0b1111011000010100);

    sr = shift_register<16>(a.data() + 357, 4, 3, 16);
    auto r663 = sr.shift();
    REQUIRE(r663 == 0b1110110000101000);

    sr = shift_register<16>(a.data() + 357, 3, 4, 16);
    auto r664 = sr.shift();
    REQUIRE(r664 == 0b1101100001010000);

    sr = shift_register<16>(a.data() + 357, 2, 5, 16);
    auto r665 = sr.shift();
    REQUIRE(r665 == 0b1011000010100001);

    sr = shift_register<16>(a.data() + 357, 1, 6, 16);
    auto r666 = sr.shift();
    REQUIRE(r666 == 0b0110000101000010);

    sr = shift_register<16>(a.data() + 357, 0, 7, 16);
    auto r667 = sr.shift();
    REQUIRE(r667 == 0b1100001010000100);

    sr = shift_register<16>(a.data() + 358, 15, 8, 16);
    auto r668 = sr.shift();
    REQUIRE(r668 == 0b1000010100001000);

    sr = shift_register<16>(a.data() + 358, 14, 9, 16);
    auto r669 = sr.shift();
    REQUIRE(r669 == 0b0000101000010001);

    sr = shift_register<16>(a.data() + 358, 13, 10, 16);
    auto r670 = sr.shift();
    REQUIRE(r670 == 0b0001010000100011);

    sr = shift_register<16>(a.data() + 358, 12, 11, 16);
    auto r671 = sr.shift();
    REQUIRE(r671 == 0b0010100001000110);

    sr = shift_register<16>(a.data() + 358, 11, 12, 16);
    auto r672 = sr.shift();
    REQUIRE(r672 == 0b0101000010001100);

    sr = shift_register<16>(a.data() + 358, 10, 13, 16);
    auto r673 = sr.shift();
    REQUIRE(r673 == 0b1010000100011001);

    sr = shift_register<16>(a.data() + 358, 9, 14, 16);
    auto r674 = sr.shift();
    REQUIRE(r674 == 0b0100001000110011);

    sr = shift_register<16>(a.data() + 365, 5, 0, 16);
    auto r675 = sr.shift();
    REQUIRE(r675 == 0b1101010101001011);

    sr = shift_register<16>(a.data() + 365, 4, 1, 16);
    auto r676 = sr.shift();
    REQUIRE(r676 == 0b1010101010010111);

    sr = shift_register<16>(a.data() + 365, 3, 2, 16);
    auto r677 = sr.shift();
    REQUIRE(r677 == 0b0101010100101111);

    sr = shift_register<16>(a.data() + 365, 2, 3, 16);
    auto r678 = sr.shift();
    REQUIRE(r678 == 0b1010101001011110);

    sr = shift_register<16>(a.data() + 365, 1, 4, 16);
    auto r679 = sr.shift();
    REQUIRE(r679 == 0b0101010010111101);

    sr = shift_register<16>(a.data() + 365, 0, 5, 16);
    auto r680 = sr.shift();
    REQUIRE(r680 == 0b1010100101111011);

    sr = shift_register<16>(a.data() + 366, 15, 6, 16);
    auto r681 = sr.shift();
    REQUIRE(r681 == 0b0101001011110110);

    sr = shift_register<16>(a.data() + 366, 14, 7, 16);
    auto r682 = sr.shift();
    REQUIRE(r682 == 0b1010010111101100);

    sr = shift_register<16>(a.data() + 366, 13, 8, 16);
    auto r683 = sr.shift();
    REQUIRE(r683 == 0b0100101111011001);

    sr = shift_register<16>(a.data() + 366, 12, 9, 16);
    auto r684 = sr.shift();
    REQUIRE(r684 == 0b1001011110110010);

    sr = shift_register<16>(a.data() + 366, 11, 10, 16);
    auto r685 = sr.shift();
    REQUIRE(r685 == 0b0010111101100101);

    sr = shift_register<16>(a.data() + 366, 10, 11, 16);
    auto r686 = sr.shift();
    REQUIRE(r686 == 0b0101111011001010);

    sr = shift_register<16>(a.data() + 366, 9, 12, 16);
    auto r687 = sr.shift();
    REQUIRE(r687 == 0b1011110110010100);

    sr = shift_register<16>(a.data() + 366, 8, 13, 16);
    auto r688 = sr.shift();
    REQUIRE(r688 == 0b0111101100101001);

    sr = shift_register<16>(a.data() + 366, 7, 14, 16);
    auto r689 = sr.shift();
    REQUIRE(r689 == 0b1111011001010011);

    sr = shift_register<16>(a.data() + 373, 3, 0, 16);
    auto r690 = sr.shift();
    REQUIRE(r690 == 0b1100001011100000);

    sr = shift_register<16>(a.data() + 373, 2, 1, 16);
    auto r691 = sr.shift();
    REQUIRE(r691 == 0b1000010111000001);

    sr = shift_register<16>(a.data() + 373, 1, 2, 16);
    auto r692 = sr.shift();
    REQUIRE(r692 == 0b0000101110000010);

    sr = shift_register<16>(a.data() + 373, 0, 3, 16);
    auto r693 = sr.shift();
    REQUIRE(r693 == 0b0001011100000101);

    sr = shift_register<16>(a.data() + 374, 15, 4, 16);
    auto r694 = sr.shift();
    REQUIRE(r694 == 0b0010111000001010);

    sr = shift_register<16>(a.data() + 374, 14, 5, 16);
    auto r695 = sr.shift();
    REQUIRE(r695 == 0b0101110000010101);

    sr = shift_register<16>(a.data() + 374, 13, 6, 16);
    auto r696 = sr.shift();
    REQUIRE(r696 == 0b1011100000101011);

    sr = shift_register<16>(a.data() + 374, 12, 7, 16);
    auto r697 = sr.shift();
    REQUIRE(r697 == 0b0111000001010110);

    sr = shift_register<16>(a.data() + 374, 11, 8, 16);
    auto r698 = sr.shift();
    REQUIRE(r698 == 0b1110000010101100);

    sr = shift_register<16>(a.data() + 374, 10, 9, 16);
    auto r699 = sr.shift();
    REQUIRE(r699 == 0b1100000101011000);

    sr = shift_register<16>(a.data() + 374, 9, 10, 16);
    auto r700 = sr.shift();
    REQUIRE(r700 == 0b1000001010110000);

    sr = shift_register<16>(a.data() + 374, 8, 11, 16);
    auto r701 = sr.shift();
    REQUIRE(r701 == 0b0000010101100001);

    sr = shift_register<16>(a.data() + 374, 7, 12, 16);
    auto r702 = sr.shift();
    REQUIRE(r702 == 0b0000101011000010);

    sr = shift_register<16>(a.data() + 374, 6, 13, 16);
    auto r703 = sr.shift();
    REQUIRE(r703 == 0b0001010110000101);

    sr = shift_register<16>(a.data() + 374, 5, 14, 16);
    auto r704 = sr.shift();
    REQUIRE(r704 == 0b0010101100001011);

    sr = shift_register<16>(a.data() + 381, 1, 0, 16);
    auto r705 = sr.shift();
    REQUIRE(r705 == 0b1001111011010110);

    sr = shift_register<16>(a.data() + 381, 0, 1, 16);
    auto r706 = sr.shift();
    REQUIRE(r706 == 0b0011110110101100);

    sr = shift_register<16>(a.data() + 382, 15, 2, 16);
    auto r707 = sr.shift();
    REQUIRE(r707 == 0b0111101101011001);

    sr = shift_register<16>(a.data() + 382, 14, 3, 16);
    auto r708 = sr.shift();
    REQUIRE(r708 == 0b1111011010110011);

    sr = shift_register<16>(a.data() + 382, 13, 4, 16);
    auto r709 = sr.shift();
    REQUIRE(r709 == 0b1110110101100110);

    sr = shift_register<16>(a.data() + 382, 12, 5, 16);
    auto r710 = sr.shift();
    REQUIRE(r710 == 0b1101101011001100);

    sr = shift_register<16>(a.data() + 382, 11, 6, 16);
    auto r711 = sr.shift();
    REQUIRE(r711 == 0b1011010110011001);

    sr = shift_register<16>(a.data() + 382, 10, 7, 16);
    auto r712 = sr.shift();
    REQUIRE(r712 == 0b0110101100110010);

    sr = shift_register<16>(a.data() + 382, 9, 8, 16);
    auto r713 = sr.shift();
    REQUIRE(r713 == 0b1101011001100101);

    sr = shift_register<16>(a.data() + 382, 8, 9, 16);
    auto r714 = sr.shift();
    REQUIRE(r714 == 0b1010110011001011);

    sr = shift_register<16>(a.data() + 382, 7, 10, 16);
    auto r715 = sr.shift();
    REQUIRE(r715 == 0b0101100110010110);

    sr = shift_register<16>(a.data() + 382, 6, 11, 16);
    auto r716 = sr.shift();
    REQUIRE(r716 == 0b1011001100101100);

    sr = shift_register<16>(a.data() + 382, 5, 12, 16);
    auto r717 = sr.shift();
    REQUIRE(r717 == 0b0110011001011001);

    sr = shift_register<16>(a.data() + 382, 4, 13, 16);
    auto r718 = sr.shift();
    REQUIRE(r718 == 0b1100110010110011);

    sr = shift_register<16>(a.data() + 382, 3, 14, 16);
    auto r719 = sr.shift();
    REQUIRE(r719 == 0b1001100101100110);

    sr = shift_register<16>(a.data() + 390, 15, 0, 16);
    auto r720 = sr.shift();
    REQUIRE(r720 == 0b0111110001001100);

    sr = shift_register<16>(a.data() + 390, 14, 1, 16);
    auto r721 = sr.shift();
    REQUIRE(r721 == 0b1111100010011000);

    sr = shift_register<16>(a.data() + 390, 13, 2, 16);
    auto r722 = sr.shift();
    REQUIRE(r722 == 0b1111000100110001);

    sr = shift_register<16>(a.data() + 390, 12, 3, 16);
    auto r723 = sr.shift();
    REQUIRE(r723 == 0b1110001001100011);

    sr = shift_register<16>(a.data() + 390, 11, 4, 16);
    auto r724 = sr.shift();
    REQUIRE(r724 == 0b1100010011000110);

    sr = shift_register<16>(a.data() + 390, 10, 5, 16);
    auto r725 = sr.shift();
    REQUIRE(r725 == 0b1000100110001101);

    sr = shift_register<16>(a.data() + 390, 9, 6, 16);
    auto r726 = sr.shift();
    REQUIRE(r726 == 0b0001001100011011);

    sr = shift_register<16>(a.data() + 390, 8, 7, 16);
    auto r727 = sr.shift();
    REQUIRE(r727 == 0b0010011000110111);

    sr = shift_register<16>(a.data() + 390, 7, 8, 16);
    auto r728 = sr.shift();
    REQUIRE(r728 == 0b0100110001101110);

    sr = shift_register<16>(a.data() + 390, 6, 9, 16);
    auto r729 = sr.shift();
    REQUIRE(r729 == 0b1001100011011101);

    sr = shift_register<16>(a.data() + 390, 5, 10, 16);
    auto r730 = sr.shift();
    REQUIRE(r730 == 0b0011000110111011);

    sr = shift_register<16>(a.data() + 390, 4, 11, 16);
    auto r731 = sr.shift();
    REQUIRE(r731 == 0b0110001101110111);

    sr = shift_register<16>(a.data() + 390, 3, 12, 16);
    auto r732 = sr.shift();
    REQUIRE(r732 == 0b1100011011101111);

    sr = shift_register<16>(a.data() + 390, 2, 13, 16);
    auto r733 = sr.shift();
    REQUIRE(r733 == 0b1000110111011111);

    sr = shift_register<16>(a.data() + 390, 1, 14, 16);
    auto r734 = sr.shift();
    REQUIRE(r734 == 0b0001101110111110);

    sr = shift_register<16>(a.data() + 398, 13, 0, 16);
    auto r735 = sr.shift();
    REQUIRE(r735 == 0b0111001111100001);

    sr = shift_register<16>(a.data() + 398, 12, 1, 16);
    auto r736 = sr.shift();
    REQUIRE(r736 == 0b1110011111000011);

    sr = shift_register<16>(a.data() + 398, 11, 2, 16);
    auto r737 = sr.shift();
    REQUIRE(r737 == 0b1100111110000110);

    sr = shift_register<16>(a.data() + 398, 10, 3, 16);
    auto r738 = sr.shift();
    REQUIRE(r738 == 0b1001111100001101);

    sr = shift_register<16>(a.data() + 398, 9, 4, 16);
    auto r739 = sr.shift();
    REQUIRE(r739 == 0b0011111000011010);

    sr = shift_register<16>(a.data() + 398, 8, 5, 16);
    auto r740 = sr.shift();
    REQUIRE(r740 == 0b0111110000110100);

    sr = shift_register<16>(a.data() + 398, 7, 6, 16);
    auto r741 = sr.shift();
    REQUIRE(r741 == 0b1111100001101000);

    sr = shift_register<16>(a.data() + 398, 6, 7, 16);
    auto r742 = sr.shift();
    REQUIRE(r742 == 0b1111000011010000);

    sr = shift_register<16>(a.data() + 398, 5, 8, 16);
    auto r743 = sr.shift();
    REQUIRE(r743 == 0b1110000110100001);

    sr = shift_register<16>(a.data() + 398, 4, 9, 16);
    auto r744 = sr.shift();
    REQUIRE(r744 == 0b1100001101000010);

    sr = shift_register<16>(a.data() + 398, 3, 10, 16);
    auto r745 = sr.shift();
    REQUIRE(r745 == 0b1000011010000101);

    sr = shift_register<16>(a.data() + 398, 2, 11, 16);
    auto r746 = sr.shift();
    REQUIRE(r746 == 0b0000110100001010);

    sr = shift_register<16>(a.data() + 398, 1, 12, 16);
    auto r747 = sr.shift();
    REQUIRE(r747 == 0b0001101000010101);

    sr = shift_register<16>(a.data() + 398, 0, 13, 16);
    auto r748 = sr.shift();
    REQUIRE(r748 == 0b0011010000101011);

    sr = shift_register<16>(a.data() + 399, 15, 14, 16);
    auto r749 = sr.shift();
    REQUIRE(r749 == 0b0110100001010111);

    sr = shift_register<16>(a.data() + 406, 11, 0, 16);
    auto r750 = sr.shift();
    REQUIRE(r750 == 0b0100111100110101);

    sr = shift_register<16>(a.data() + 406, 10, 1, 16);
    auto r751 = sr.shift();
    REQUIRE(r751 == 0b1001111001101011);

    sr = shift_register<16>(a.data() + 406, 9, 2, 16);
    auto r752 = sr.shift();
    REQUIRE(r752 == 0b0011110011010111);

    sr = shift_register<16>(a.data() + 406, 8, 3, 16);
    auto r753 = sr.shift();
    REQUIRE(r753 == 0b0111100110101110);

    sr = shift_register<16>(a.data() + 406, 7, 4, 16);
    auto r754 = sr.shift();
    REQUIRE(r754 == 0b1111001101011101);

    sr = shift_register<16>(a.data() + 406, 6, 5, 16);
    auto r755 = sr.shift();
    REQUIRE(r755 == 0b1110011010111011);

    sr = shift_register<16>(a.data() + 406, 5, 6, 16);
    auto r756 = sr.shift();
    REQUIRE(r756 == 0b1100110101110111);

    sr = shift_register<16>(a.data() + 406, 4, 7, 16);
    auto r757 = sr.shift();
    REQUIRE(r757 == 0b1001101011101110);

    sr = shift_register<16>(a.data() + 406, 3, 8, 16);
    auto r758 = sr.shift();
    REQUIRE(r758 == 0b0011010111011100);

    sr = shift_register<16>(a.data() + 406, 2, 9, 16);
    auto r759 = sr.shift();
    REQUIRE(r759 == 0b0110101110111000);

    sr = shift_register<16>(a.data() + 406, 1, 10, 16);
    auto r760 = sr.shift();
    REQUIRE(r760 == 0b1101011101110000);

    sr = shift_register<16>(a.data() + 406, 0, 11, 16);
    auto r761 = sr.shift();
    REQUIRE(r761 == 0b1010111011100000);

    sr = shift_register<16>(a.data() + 407, 15, 12, 16);
    auto r762 = sr.shift();
    REQUIRE(r762 == 0b0101110111000000);

    sr = shift_register<16>(a.data() + 407, 14, 13, 16);
    auto r763 = sr.shift();
    REQUIRE(r763 == 0b1011101110000000);

    sr = shift_register<16>(a.data() + 407, 13, 14, 16);
    auto r764 = sr.shift();
    REQUIRE(r764 == 0b0111011100000001);

    sr = shift_register<16>(a.data() + 414, 9, 0, 16);
    auto r765 = sr.shift();
    REQUIRE(r765 == 0b0001100110110100);

    sr = shift_register<16>(a.data() + 414, 8, 1, 16);
    auto r766 = sr.shift();
    REQUIRE(r766 == 0b0011001101101000);

    sr = shift_register<16>(a.data() + 414, 7, 2, 16);
    auto r767 = sr.shift();
    REQUIRE(r767 == 0b0110011011010001);

    sr = shift_register<16>(a.data() + 414, 6, 3, 16);
    auto r768 = sr.shift();
    REQUIRE(r768 == 0b1100110110100011);

    sr = shift_register<16>(a.data() + 414, 5, 4, 16);
    auto r769 = sr.shift();
    REQUIRE(r769 == 0b1001101101000110);

    sr = shift_register<16>(a.data() + 414, 4, 5, 16);
    auto r770 = sr.shift();
    REQUIRE(r770 == 0b0011011010001101);

    sr = shift_register<16>(a.data() + 414, 3, 6, 16);
    auto r771 = sr.shift();
    REQUIRE(r771 == 0b0110110100011010);

    sr = shift_register<16>(a.data() + 414, 2, 7, 16);
    auto r772 = sr.shift();
    REQUIRE(r772 == 0b1101101000110101);

    sr = shift_register<16>(a.data() + 414, 1, 8, 16);
    auto r773 = sr.shift();
    REQUIRE(r773 == 0b1011010001101011);

    sr = shift_register<16>(a.data() + 414, 0, 9, 16);
    auto r774 = sr.shift();
    REQUIRE(r774 == 0b0110100011010111);

    sr = shift_register<16>(a.data() + 415, 15, 10, 16);
    auto r775 = sr.shift();
    REQUIRE(r775 == 0b1101000110101111);

    sr = shift_register<16>(a.data() + 415, 14, 11, 16);
    auto r776 = sr.shift();
    REQUIRE(r776 == 0b1010001101011111);

    sr = shift_register<16>(a.data() + 415, 13, 12, 16);
    auto r777 = sr.shift();
    REQUIRE(r777 == 0b0100011010111111);

    sr = shift_register<16>(a.data() + 415, 12, 13, 16);
    auto r778 = sr.shift();
    REQUIRE(r778 == 0b1000110101111111);

    sr = shift_register<16>(a.data() + 415, 11, 14, 16);
    auto r779 = sr.shift();
    REQUIRE(r779 == 0b0001101011111111);

    sr = shift_register<16>(a.data() + 422, 7, 0, 16);
    auto r780 = sr.shift();
    REQUIRE(r780 == 0b1100111110101000);

    sr = shift_register<16>(a.data() + 422, 6, 1, 16);
    auto r781 = sr.shift();
    REQUIRE(r781 == 0b1001111101010000);

    sr = shift_register<16>(a.data() + 422, 5, 2, 16);
    auto r782 = sr.shift();
    REQUIRE(r782 == 0b0011111010100000);

    sr = shift_register<16>(a.data() + 422, 4, 3, 16);
    auto r783 = sr.shift();
    REQUIRE(r783 == 0b0111110101000001);

    sr = shift_register<16>(a.data() + 422, 3, 4, 16);
    auto r784 = sr.shift();
    REQUIRE(r784 == 0b1111101010000011);

    sr = shift_register<16>(a.data() + 422, 2, 5, 16);
    auto r785 = sr.shift();
    REQUIRE(r785 == 0b1111010100000111);

    sr = shift_register<16>(a.data() + 422, 1, 6, 16);
    auto r786 = sr.shift();
    REQUIRE(r786 == 0b1110101000001110);

    sr = shift_register<16>(a.data() + 422, 0, 7, 16);
    auto r787 = sr.shift();
    REQUIRE(r787 == 0b1101010000011100);

    sr = shift_register<16>(a.data() + 423, 15, 8, 16);
    auto r788 = sr.shift();
    REQUIRE(r788 == 0b1010100000111001);

    sr = shift_register<16>(a.data() + 423, 14, 9, 16);
    auto r789 = sr.shift();
    REQUIRE(r789 == 0b0101000001110011);

    sr = shift_register<16>(a.data() + 423, 13, 10, 16);
    auto r790 = sr.shift();
    REQUIRE(r790 == 0b1010000011100111);

    sr = shift_register<16>(a.data() + 423, 12, 11, 16);
    auto r791 = sr.shift();
    REQUIRE(r791 == 0b0100000111001110);

    sr = shift_register<16>(a.data() + 423, 11, 12, 16);
    auto r792 = sr.shift();
    REQUIRE(r792 == 0b1000001110011101);

    sr = shift_register<16>(a.data() + 423, 10, 13, 16);
    auto r793 = sr.shift();
    REQUIRE(r793 == 0b0000011100111010);

    sr = shift_register<16>(a.data() + 423, 9, 14, 16);
    auto r794 = sr.shift();
    REQUIRE(r794 == 0b0000111001110100);

    sr = shift_register<16>(a.data() + 430, 5, 0, 16);
    auto r795 = sr.shift();
    REQUIRE(r795 == 0b0011001010101101);

    sr = shift_register<16>(a.data() + 430, 4, 1, 16);
    auto r796 = sr.shift();
    REQUIRE(r796 == 0b0110010101011010);

    sr = shift_register<16>(a.data() + 430, 3, 2, 16);
    auto r797 = sr.shift();
    REQUIRE(r797 == 0b1100101010110100);

    sr = shift_register<16>(a.data() + 430, 2, 3, 16);
    auto r798 = sr.shift();
    REQUIRE(r798 == 0b1001010101101001);

    sr = shift_register<16>(a.data() + 430, 1, 4, 16);
    auto r799 = sr.shift();
    REQUIRE(r799 == 0b0010101011010010);

    sr = shift_register<16>(a.data() + 430, 0, 5, 16);
    auto r800 = sr.shift();
    REQUIRE(r800 == 0b0101010110100100);

    sr = shift_register<16>(a.data() + 431, 15, 6, 16);
    auto r801 = sr.shift();
    REQUIRE(r801 == 0b1010101101001000);

    sr = shift_register<16>(a.data() + 431, 14, 7, 16);
    auto r802 = sr.shift();
    REQUIRE(r802 == 0b0101011010010000);

    sr = shift_register<16>(a.data() + 431, 13, 8, 16);
    auto r803 = sr.shift();
    REQUIRE(r803 == 0b1010110100100001);

    sr = shift_register<16>(a.data() + 431, 12, 9, 16);
    auto r804 = sr.shift();
    REQUIRE(r804 == 0b0101101001000011);

    sr = shift_register<16>(a.data() + 431, 11, 10, 16);
    auto r805 = sr.shift();
    REQUIRE(r805 == 0b1011010010000111);

    sr = shift_register<16>(a.data() + 431, 10, 11, 16);
    auto r806 = sr.shift();
    REQUIRE(r806 == 0b0110100100001110);

    sr = shift_register<16>(a.data() + 431, 9, 12, 16);
    auto r807 = sr.shift();
    REQUIRE(r807 == 0b1101001000011100);

    sr = shift_register<16>(a.data() + 431, 8, 13, 16);
    auto r808 = sr.shift();
    REQUIRE(r808 == 0b1010010000111001);

    sr = shift_register<16>(a.data() + 431, 7, 14, 16);
    auto r809 = sr.shift();
    REQUIRE(r809 == 0b0100100001110010);

    sr = shift_register<16>(a.data() + 438, 3, 0, 16);
    auto r810 = sr.shift();
    REQUIRE(r810 == 0b0111100110110000);

    sr = shift_register<16>(a.data() + 438, 2, 1, 16);
    auto r811 = sr.shift();
    REQUIRE(r811 == 0b1111001101100001);

    sr = shift_register<16>(a.data() + 438, 1, 2, 16);
    auto r812 = sr.shift();
    REQUIRE(r812 == 0b1110011011000010);

    sr = shift_register<16>(a.data() + 438, 0, 3, 16);
    auto r813 = sr.shift();
    REQUIRE(r813 == 0b1100110110000101);

    sr = shift_register<16>(a.data() + 439, 15, 4, 16);
    auto r814 = sr.shift();
    REQUIRE(r814 == 0b1001101100001010);

    sr = shift_register<16>(a.data() + 439, 14, 5, 16);
    auto r815 = sr.shift();
    REQUIRE(r815 == 0b0011011000010101);

    sr = shift_register<16>(a.data() + 439, 13, 6, 16);
    auto r816 = sr.shift();
    REQUIRE(r816 == 0b0110110000101010);

    sr = shift_register<16>(a.data() + 439, 12, 7, 16);
    auto r817 = sr.shift();
    REQUIRE(r817 == 0b1101100001010101);

    sr = shift_register<16>(a.data() + 439, 11, 8, 16);
    auto r818 = sr.shift();
    REQUIRE(r818 == 0b1011000010101010);

    sr = shift_register<16>(a.data() + 439, 10, 9, 16);
    auto r819 = sr.shift();
    REQUIRE(r819 == 0b0110000101010101);

    sr = shift_register<16>(a.data() + 439, 9, 10, 16);
    auto r820 = sr.shift();
    REQUIRE(r820 == 0b1100001010101011);

    sr = shift_register<16>(a.data() + 439, 8, 11, 16);
    auto r821 = sr.shift();
    REQUIRE(r821 == 0b1000010101010110);

    sr = shift_register<16>(a.data() + 439, 7, 12, 16);
    auto r822 = sr.shift();
    REQUIRE(r822 == 0b0000101010101100);

    sr = shift_register<16>(a.data() + 439, 6, 13, 16);
    auto r823 = sr.shift();
    REQUIRE(r823 == 0b0001010101011001);

    sr = shift_register<16>(a.data() + 439, 5, 14, 16);
    auto r824 = sr.shift();
    REQUIRE(r824 == 0b0010101010110011);

    sr = shift_register<16>(a.data() + 446, 1, 0, 16);
    auto r825 = sr.shift();
    REQUIRE(r825 == 0b0010100111101000);

    sr = shift_register<16>(a.data() + 446, 0, 1, 16);
    auto r826 = sr.shift();
    REQUIRE(r826 == 0b0101001111010000);

    sr = shift_register<16>(a.data() + 447, 15, 2, 16);
    auto r827 = sr.shift();
    REQUIRE(r827 == 0b1010011110100001);

    sr = shift_register<16>(a.data() + 447, 14, 3, 16);
    auto r828 = sr.shift();
    REQUIRE(r828 == 0b0100111101000011);

    sr = shift_register<16>(a.data() + 447, 13, 4, 16);
    auto r829 = sr.shift();
    REQUIRE(r829 == 0b1001111010000111);

    sr = shift_register<16>(a.data() + 447, 12, 5, 16);
    auto r830 = sr.shift();
    REQUIRE(r830 == 0b0011110100001110);

    sr = shift_register<16>(a.data() + 447, 11, 6, 16);
    auto r831 = sr.shift();
    REQUIRE(r831 == 0b0111101000011101);

    sr = shift_register<16>(a.data() + 447, 10, 7, 16);
    auto r832 = sr.shift();
    REQUIRE(r832 == 0b1111010000111011);

    sr = shift_register<16>(a.data() + 447, 9, 8, 16);
    auto r833 = sr.shift();
    REQUIRE(r833 == 0b1110100001110111);

    sr = shift_register<16>(a.data() + 447, 8, 9, 16);
    auto r834 = sr.shift();
    REQUIRE(r834 == 0b1101000011101111);

    sr = shift_register<16>(a.data() + 447, 7, 10, 16);
    auto r835 = sr.shift();
    REQUIRE(r835 == 0b1010000111011111);

    sr = shift_register<16>(a.data() + 447, 6, 11, 16);
    auto r836 = sr.shift();
    REQUIRE(r836 == 0b0100001110111110);

    sr = shift_register<16>(a.data() + 447, 5, 12, 16);
    auto r837 = sr.shift();
    REQUIRE(r837 == 0b1000011101111101);

    sr = shift_register<16>(a.data() + 447, 4, 13, 16);
    auto r838 = sr.shift();
    REQUIRE(r838 == 0b0000111011111010);

    sr = shift_register<16>(a.data() + 447, 3, 14, 16);
    auto r839 = sr.shift();
    REQUIRE(r839 == 0b0001110111110100);

    sr = shift_register<16>(a.data() + 455, 15, 0, 16);
    auto r840 = sr.shift();
    REQUIRE(r840 == 0b0110000000010110);

    sr = shift_register<16>(a.data() + 455, 14, 1, 16);
    auto r841 = sr.shift();
    REQUIRE(r841 == 0b1100000000101100);

    sr = shift_register<16>(a.data() + 455, 13, 2, 16);
    auto r842 = sr.shift();
    REQUIRE(r842 == 0b1000000001011001);

    sr = shift_register<16>(a.data() + 455, 12, 3, 16);
    auto r843 = sr.shift();
    REQUIRE(r843 == 0b0000000010110011);

    sr = shift_register<16>(a.data() + 455, 11, 4, 16);
    auto r844 = sr.shift();
    REQUIRE(r844 == 0b0000000101100111);

    sr = shift_register<16>(a.data() + 455, 10, 5, 16);
    auto r845 = sr.shift();
    REQUIRE(r845 == 0b0000001011001110);

    sr = shift_register<16>(a.data() + 455, 9, 6, 16);
    auto r846 = sr.shift();
    REQUIRE(r846 == 0b0000010110011101);

    sr = shift_register<16>(a.data() + 455, 8, 7, 16);
    auto r847 = sr.shift();
    REQUIRE(r847 == 0b0000101100111010);

    sr = shift_register<16>(a.data() + 455, 7, 8, 16);
    auto r848 = sr.shift();
    REQUIRE(r848 == 0b0001011001110100);

    sr = shift_register<16>(a.data() + 455, 6, 9, 16);
    auto r849 = sr.shift();
    REQUIRE(r849 == 0b0010110011101001);

    sr = shift_register<16>(a.data() + 455, 5, 10, 16);
    auto r850 = sr.shift();
    REQUIRE(r850 == 0b0101100111010010);

    sr = shift_register<16>(a.data() + 455, 4, 11, 16);
    auto r851 = sr.shift();
    REQUIRE(r851 == 0b1011001110100101);

    sr = shift_register<16>(a.data() + 455, 3, 12, 16);
    auto r852 = sr.shift();
    REQUIRE(r852 == 0b0110011101001010);

    sr = shift_register<16>(a.data() + 455, 2, 13, 16);
    auto r853 = sr.shift();
    REQUIRE(r853 == 0b1100111010010100);

    sr = shift_register<16>(a.data() + 455, 1, 14, 16);
    auto r854 = sr.shift();
    REQUIRE(r854 == 0b1001110100101000);

    sr = shift_register<16>(a.data() + 463, 13, 0, 16);
    auto r855 = sr.shift();
    REQUIRE(r855 == 0b1101001111101000);

    sr = shift_register<16>(a.data() + 463, 12, 1, 16);
    auto r856 = sr.shift();
    REQUIRE(r856 == 0b1010011111010000);

    sr = shift_register<16>(a.data() + 463, 11, 2, 16);
    auto r857 = sr.shift();
    REQUIRE(r857 == 0b0100111110100000);

    sr = shift_register<16>(a.data() + 463, 10, 3, 16);
    auto r858 = sr.shift();
    REQUIRE(r858 == 0b1001111101000000);

    sr = shift_register<16>(a.data() + 463, 9, 4, 16);
    auto r859 = sr.shift();
    REQUIRE(r859 == 0b0011111010000000);

    sr = shift_register<16>(a.data() + 463, 8, 5, 16);
    auto r860 = sr.shift();
    REQUIRE(r860 == 0b0111110100000001);

    sr = shift_register<16>(a.data() + 463, 7, 6, 16);
    auto r861 = sr.shift();
    REQUIRE(r861 == 0b1111101000000011);

    sr = shift_register<16>(a.data() + 463, 6, 7, 16);
    auto r862 = sr.shift();
    REQUIRE(r862 == 0b1111010000000111);

    sr = shift_register<16>(a.data() + 463, 5, 8, 16);
    auto r863 = sr.shift();
    REQUIRE(r863 == 0b1110100000001110);

    sr = shift_register<16>(a.data() + 463, 4, 9, 16);
    auto r864 = sr.shift();
    REQUIRE(r864 == 0b1101000000011100);

    sr = shift_register<16>(a.data() + 463, 3, 10, 16);
    auto r865 = sr.shift();
    REQUIRE(r865 == 0b1010000000111001);

    sr = shift_register<16>(a.data() + 463, 2, 11, 16);
    auto r866 = sr.shift();
    REQUIRE(r866 == 0b0100000001110011);

    sr = shift_register<16>(a.data() + 463, 1, 12, 16);
    auto r867 = sr.shift();
    REQUIRE(r867 == 0b1000000011100111);

    sr = shift_register<16>(a.data() + 463, 0, 13, 16);
    auto r868 = sr.shift();
    REQUIRE(r868 == 0b0000000111001110);

    sr = shift_register<16>(a.data() + 464, 15, 14, 16);
    auto r869 = sr.shift();
    REQUIRE(r869 == 0b0000001110011100);

    sr = shift_register<16>(a.data() + 471, 11, 0, 16);
    auto r870 = sr.shift();
    REQUIRE(r870 == 0b0111100000011011);

    sr = shift_register<16>(a.data() + 471, 10, 1, 16);
    auto r871 = sr.shift();
    REQUIRE(r871 == 0b1111000000110111);

    sr = shift_register<16>(a.data() + 471, 9, 2, 16);
    auto r872 = sr.shift();
    REQUIRE(r872 == 0b1110000001101111);

    sr = shift_register<16>(a.data() + 471, 8, 3, 16);
    auto r873 = sr.shift();
    REQUIRE(r873 == 0b1100000011011111);

    sr = shift_register<16>(a.data() + 471, 7, 4, 16);
    auto r874 = sr.shift();
    REQUIRE(r874 == 0b1000000110111110);

    sr = shift_register<16>(a.data() + 471, 6, 5, 16);
    auto r875 = sr.shift();
    REQUIRE(r875 == 0b0000001101111100);

    sr = shift_register<16>(a.data() + 471, 5, 6, 16);
    auto r876 = sr.shift();
    REQUIRE(r876 == 0b0000011011111001);

    sr = shift_register<16>(a.data() + 471, 4, 7, 16);
    auto r877 = sr.shift();
    REQUIRE(r877 == 0b0000110111110010);

    sr = shift_register<16>(a.data() + 471, 3, 8, 16);
    auto r878 = sr.shift();
    REQUIRE(r878 == 0b0001101111100100);

    sr = shift_register<16>(a.data() + 471, 2, 9, 16);
    auto r879 = sr.shift();
    REQUIRE(r879 == 0b0011011111001001);

    sr = shift_register<16>(a.data() + 471, 1, 10, 16);
    auto r880 = sr.shift();
    REQUIRE(r880 == 0b0110111110010010);

    sr = shift_register<16>(a.data() + 471, 0, 11, 16);
    auto r881 = sr.shift();
    REQUIRE(r881 == 0b1101111100100101);

    sr = shift_register<16>(a.data() + 472, 15, 12, 16);
    auto r882 = sr.shift();
    REQUIRE(r882 == 0b1011111001001011);

    sr = shift_register<16>(a.data() + 472, 14, 13, 16);
    auto r883 = sr.shift();
    REQUIRE(r883 == 0b0111110010010111);

    sr = shift_register<16>(a.data() + 472, 13, 14, 16);
    auto r884 = sr.shift();
    REQUIRE(r884 == 0b1111100100101110);

    sr = shift_register<16>(a.data() + 479, 9, 0, 16);
    auto r885 = sr.shift();
    REQUIRE(r885 == 0b1000010100000011);

    sr = shift_register<16>(a.data() + 479, 8, 1, 16);
    auto r886 = sr.shift();
    REQUIRE(r886 == 0b0000101000000110);

    sr = shift_register<16>(a.data() + 479, 7, 2, 16);
    auto r887 = sr.shift();
    REQUIRE(r887 == 0b0001010000001101);

    sr = shift_register<16>(a.data() + 479, 6, 3, 16);
    auto r888 = sr.shift();
    REQUIRE(r888 == 0b0010100000011011);

    sr = shift_register<16>(a.data() + 479, 5, 4, 16);
    auto r889 = sr.shift();
    REQUIRE(r889 == 0b0101000000110110);

    sr = shift_register<16>(a.data() + 479, 4, 5, 16);
    auto r890 = sr.shift();
    REQUIRE(r890 == 0b1010000001101100);

    sr = shift_register<16>(a.data() + 479, 3, 6, 16);
    auto r891 = sr.shift();
    REQUIRE(r891 == 0b0100000011011001);

    sr = shift_register<16>(a.data() + 479, 2, 7, 16);
    auto r892 = sr.shift();
    REQUIRE(r892 == 0b1000000110110010);

    sr = shift_register<16>(a.data() + 479, 1, 8, 16);
    auto r893 = sr.shift();
    REQUIRE(r893 == 0b0000001101100100);

    sr = shift_register<16>(a.data() + 479, 0, 9, 16);
    auto r894 = sr.shift();
    REQUIRE(r894 == 0b0000011011001000);

    sr = shift_register<16>(a.data() + 480, 15, 10, 16);
    auto r895 = sr.shift();
    REQUIRE(r895 == 0b0000110110010000);

    sr = shift_register<16>(a.data() + 480, 14, 11, 16);
    auto r896 = sr.shift();
    REQUIRE(r896 == 0b0001101100100001);

    sr = shift_register<16>(a.data() + 480, 13, 12, 16);
    auto r897 = sr.shift();
    REQUIRE(r897 == 0b0011011001000011);

    sr = shift_register<16>(a.data() + 480, 12, 13, 16);
    auto r898 = sr.shift();
    REQUIRE(r898 == 0b0110110010000110);

    sr = shift_register<16>(a.data() + 480, 11, 14, 16);
    auto r899 = sr.shift();
    REQUIRE(r899 == 0b1101100100001101);

    sr = shift_register<16>(a.data() + 487, 7, 0, 16);
    auto r900 = sr.shift();
    REQUIRE(r900 == 0b1101110011000100);

    sr = shift_register<16>(a.data() + 487, 6, 1, 16);
    auto r901 = sr.shift();
    REQUIRE(r901 == 0b1011100110001000);

    sr = shift_register<16>(a.data() + 487, 5, 2, 16);
    auto r902 = sr.shift();
    REQUIRE(r902 == 0b0111001100010000);

    sr = shift_register<16>(a.data() + 487, 4, 3, 16);
    auto r903 = sr.shift();
    REQUIRE(r903 == 0b1110011000100001);

    sr = shift_register<16>(a.data() + 487, 3, 4, 16);
    auto r904 = sr.shift();
    REQUIRE(r904 == 0b1100110001000011);

    sr = shift_register<16>(a.data() + 487, 2, 5, 16);
    auto r905 = sr.shift();
    REQUIRE(r905 == 0b1001100010000111);

    sr = shift_register<16>(a.data() + 487, 1, 6, 16);
    auto r906 = sr.shift();
    REQUIRE(r906 == 0b0011000100001110);

    sr = shift_register<16>(a.data() + 487, 0, 7, 16);
    auto r907 = sr.shift();
    REQUIRE(r907 == 0b0110001000011100);

    sr = shift_register<16>(a.data() + 488, 15, 8, 16);
    auto r908 = sr.shift();
    REQUIRE(r908 == 0b1100010000111001);

    sr = shift_register<16>(a.data() + 488, 14, 9, 16);
    auto r909 = sr.shift();
    REQUIRE(r909 == 0b1000100001110011);

    sr = shift_register<16>(a.data() + 488, 13, 10, 16);
    auto r910 = sr.shift();
    REQUIRE(r910 == 0b0001000011100111);

    sr = shift_register<16>(a.data() + 488, 12, 11, 16);
    auto r911 = sr.shift();
    REQUIRE(r911 == 0b0010000111001111);

    sr = shift_register<16>(a.data() + 488, 11, 12, 16);
    auto r912 = sr.shift();
    REQUIRE(r912 == 0b0100001110011110);

    sr = shift_register<16>(a.data() + 488, 10, 13, 16);
    auto r913 = sr.shift();
    REQUIRE(r913 == 0b1000011100111101);

    sr = shift_register<16>(a.data() + 488, 9, 14, 16);
    auto r914 = sr.shift();
    REQUIRE(r914 == 0b0000111001111011);

    sr = shift_register<16>(a.data() + 495, 5, 0, 16);
    auto r915 = sr.shift();
    REQUIRE(r915 == 0b1011110010111101);

    sr = shift_register<16>(a.data() + 495, 4, 1, 16);
    auto r916 = sr.shift();
    REQUIRE(r916 == 0b0111100101111010);

    sr = shift_register<16>(a.data() + 495, 3, 2, 16);
    auto r917 = sr.shift();
    REQUIRE(r917 == 0b1111001011110100);

    sr = shift_register<16>(a.data() + 495, 2, 3, 16);
    auto r918 = sr.shift();
    REQUIRE(r918 == 0b1110010111101001);

    sr = shift_register<16>(a.data() + 495, 1, 4, 16);
    auto r919 = sr.shift();
    REQUIRE(r919 == 0b1100101111010011);

    sr = shift_register<16>(a.data() + 495, 0, 5, 16);
    auto r920 = sr.shift();
    REQUIRE(r920 == 0b1001011110100111);

    sr = shift_register<16>(a.data() + 496, 15, 6, 16);
    auto r921 = sr.shift();
    REQUIRE(r921 == 0b0010111101001110);

    sr = shift_register<16>(a.data() + 496, 14, 7, 16);
    auto r922 = sr.shift();
    REQUIRE(r922 == 0b0101111010011101);

    sr = shift_register<16>(a.data() + 496, 13, 8, 16);
    auto r923 = sr.shift();
    REQUIRE(r923 == 0b1011110100111010);

    sr = shift_register<16>(a.data() + 496, 12, 9, 16);
    auto r924 = sr.shift();
    REQUIRE(r924 == 0b0111101001110100);

    sr = shift_register<16>(a.data() + 496, 11, 10, 16);
    auto r925 = sr.shift();
    REQUIRE(r925 == 0b1111010011101001);

    sr = shift_register<16>(a.data() + 496, 10, 11, 16);
    auto r926 = sr.shift();
    REQUIRE(r926 == 0b1110100111010011);

    sr = shift_register<16>(a.data() + 496, 9, 12, 16);
    auto r927 = sr.shift();
    REQUIRE(r927 == 0b1101001110100111);

    sr = shift_register<16>(a.data() + 496, 8, 13, 16);
    auto r928 = sr.shift();
    REQUIRE(r928 == 0b1010011101001110);

    sr = shift_register<16>(a.data() + 496, 7, 14, 16);
    auto r929 = sr.shift();
    REQUIRE(r929 == 0b0100111010011100);

    sr = shift_register<16>(a.data() + 503, 3, 0, 16);
    auto r930 = sr.shift();
    REQUIRE(r930 == 0b0011111000101101);

    sr = shift_register<16>(a.data() + 503, 2, 1, 16);
    auto r931 = sr.shift();
    REQUIRE(r931 == 0b0111110001011011);

    sr = shift_register<16>(a.data() + 503, 1, 2, 16);
    auto r932 = sr.shift();
    REQUIRE(r932 == 0b1111100010110110);

    sr = shift_register<16>(a.data() + 503, 0, 3, 16);
    auto r933 = sr.shift();
    REQUIRE(r933 == 0b1111000101101101);

    sr = shift_register<16>(a.data() + 504, 15, 4, 16);
    auto r934 = sr.shift();
    REQUIRE(r934 == 0b1110001011011010);

    sr = shift_register<16>(a.data() + 504, 14, 5, 16);
    auto r935 = sr.shift();
    REQUIRE(r935 == 0b1100010110110101);

    sr = shift_register<16>(a.data() + 504, 13, 6, 16);
    auto r936 = sr.shift();
    REQUIRE(r936 == 0b1000101101101011);

    sr = shift_register<16>(a.data() + 504, 12, 7, 16);
    auto r937 = sr.shift();
    REQUIRE(r937 == 0b0001011011010111);

    sr = shift_register<16>(a.data() + 504, 11, 8, 16);
    auto r938 = sr.shift();
    REQUIRE(r938 == 0b0010110110101110);

    sr = shift_register<16>(a.data() + 504, 10, 9, 16);
    auto r939 = sr.shift();
    REQUIRE(r939 == 0b0101101101011100);

    sr = shift_register<16>(a.data() + 504, 9, 10, 16);
    auto r940 = sr.shift();
    REQUIRE(r940 == 0b1011011010111001);

    sr = shift_register<16>(a.data() + 504, 8, 11, 16);
    auto r941 = sr.shift();
    REQUIRE(r941 == 0b0110110101110011);

    sr = shift_register<16>(a.data() + 504, 7, 12, 16);
    auto r942 = sr.shift();
    REQUIRE(r942 == 0b1101101011100111);

    sr = shift_register<16>(a.data() + 504, 6, 13, 16);
    auto r943 = sr.shift();
    REQUIRE(r943 == 0b1011010111001111);

    sr = shift_register<16>(a.data() + 504, 5, 14, 16);
    auto r944 = sr.shift();
    REQUIRE(r944 == 0b0110101110011111);

    sr = shift_register<16>(a.data() + 511, 1, 0, 16);
    auto r945 = sr.shift();
    REQUIRE(r945 == 0b0011001101010011);

    sr = shift_register<16>(a.data() + 511, 0, 1, 16);
    auto r946 = sr.shift();
    REQUIRE(r946 == 0b0110011010100110);

    sr = shift_register<16>(a.data() + 512, 15, 2, 16);
    auto r947 = sr.shift();
    REQUIRE(r947 == 0b1100110101001101);

    sr = shift_register<16>(a.data() + 512, 14, 3, 16);
    auto r948 = sr.shift();
    REQUIRE(r948 == 0b1001101010011011);

    sr = shift_register<16>(a.data() + 512, 13, 4, 16);
    auto r949 = sr.shift();
    REQUIRE(r949 == 0b0011010100110111);

    sr = shift_register<16>(a.data() + 512, 12, 5, 16);
    auto r950 = sr.shift();
    REQUIRE(r950 == 0b0110101001101110);

    sr = shift_register<16>(a.data() + 512, 11, 6, 16);
    auto r951 = sr.shift();
    REQUIRE(r951 == 0b1101010011011101);

    sr = shift_register<16>(a.data() + 512, 10, 7, 16);
    auto r952 = sr.shift();
    REQUIRE(r952 == 0b1010100110111010);

    sr = shift_register<16>(a.data() + 512, 9, 8, 16);
    auto r953 = sr.shift();
    REQUIRE(r953 == 0b0101001101110100);

    sr = shift_register<16>(a.data() + 512, 8, 9, 16);
    auto r954 = sr.shift();
    REQUIRE(r954 == 0b1010011011101000);

    sr = shift_register<16>(a.data() + 512, 7, 10, 16);
    auto r955 = sr.shift();
    REQUIRE(r955 == 0b0100110111010000);

    sr = shift_register<16>(a.data() + 512, 6, 11, 16);
    auto r956 = sr.shift();
    REQUIRE(r956 == 0b1001101110100001);

    sr = shift_register<16>(a.data() + 512, 5, 12, 16);
    auto r957 = sr.shift();
    REQUIRE(r957 == 0b0011011101000010);

    sr = shift_register<16>(a.data() + 512, 4, 13, 16);
    auto r958 = sr.shift();
    REQUIRE(r958 == 0b0110111010000101);

    sr = shift_register<16>(a.data() + 512, 3, 14, 16);
    auto r959 = sr.shift();
    REQUIRE(r959 == 0b1101110100001010);

    sr = shift_register<16>(a.data() + 520, 15, 0, 16);
    auto r960 = sr.shift();
    REQUIRE(r960 == 0b1000101010101111);

    sr = shift_register<16>(a.data() + 520, 14, 1, 16);
    auto r961 = sr.shift();
    REQUIRE(r961 == 0b0001010101011111);

    sr = shift_register<16>(a.data() + 520, 13, 2, 16);
    auto r962 = sr.shift();
    REQUIRE(r962 == 0b0010101010111111);

    sr = shift_register<16>(a.data() + 520, 12, 3, 16);
    auto r963 = sr.shift();
    REQUIRE(r963 == 0b0101010101111110);

    sr = shift_register<16>(a.data() + 520, 11, 4, 16);
    auto r964 = sr.shift();
    REQUIRE(r964 == 0b1010101011111100);

    sr = shift_register<16>(a.data() + 520, 10, 5, 16);
    auto r965 = sr.shift();
    REQUIRE(r965 == 0b0101010111111001);

    sr = shift_register<16>(a.data() + 520, 9, 6, 16);
    auto r966 = sr.shift();
    REQUIRE(r966 == 0b1010101111110010);

    sr = shift_register<16>(a.data() + 520, 8, 7, 16);
    auto r967 = sr.shift();
    REQUIRE(r967 == 0b0101011111100101);

    sr = shift_register<16>(a.data() + 520, 7, 8, 16);
    auto r968 = sr.shift();
    REQUIRE(r968 == 0b1010111111001010);

    sr = shift_register<16>(a.data() + 520, 6, 9, 16);
    auto r969 = sr.shift();
    REQUIRE(r969 == 0b0101111110010100);

    sr = shift_register<16>(a.data() + 520, 5, 10, 16);
    auto r970 = sr.shift();
    REQUIRE(r970 == 0b1011111100101001);

    sr = shift_register<16>(a.data() + 520, 4, 11, 16);
    auto r971 = sr.shift();
    REQUIRE(r971 == 0b0111111001010010);

    sr = shift_register<16>(a.data() + 520, 3, 12, 16);
    auto r972 = sr.shift();
    REQUIRE(r972 == 0b1111110010100101);

    sr = shift_register<16>(a.data() + 520, 2, 13, 16);
    auto r973 = sr.shift();
    REQUIRE(r973 == 0b1111100101001010);

    sr = shift_register<16>(a.data() + 520, 1, 14, 16);
    auto r974 = sr.shift();
    REQUIRE(r974 == 0b1111001010010101);

    sr = shift_register<16>(a.data() + 528, 13, 0, 16);
    auto r975 = sr.shift();
    REQUIRE(r975 == 0b0100101010011010);

    sr = shift_register<16>(a.data() + 528, 12, 1, 16);
    auto r976 = sr.shift();
    REQUIRE(r976 == 0b1001010100110100);

    sr = shift_register<16>(a.data() + 528, 11, 2, 16);
    auto r977 = sr.shift();
    REQUIRE(r977 == 0b0010101001101001);

    sr = shift_register<16>(a.data() + 528, 10, 3, 16);
    auto r978 = sr.shift();
    REQUIRE(r978 == 0b0101010011010010);

    sr = shift_register<16>(a.data() + 528, 9, 4, 16);
    auto r979 = sr.shift();
    REQUIRE(r979 == 0b1010100110100101);

    sr = shift_register<16>(a.data() + 528, 8, 5, 16);
    auto r980 = sr.shift();
    REQUIRE(r980 == 0b0101001101001011);

    sr = shift_register<16>(a.data() + 528, 7, 6, 16);
    auto r981 = sr.shift();
    REQUIRE(r981 == 0b1010011010010110);

    sr = shift_register<16>(a.data() + 528, 6, 7, 16);
    auto r982 = sr.shift();
    REQUIRE(r982 == 0b0100110100101100);

    sr = shift_register<16>(a.data() + 528, 5, 8, 16);
    auto r983 = sr.shift();
    REQUIRE(r983 == 0b1001101001011000);

    sr = shift_register<16>(a.data() + 528, 4, 9, 16);
    auto r984 = sr.shift();
    REQUIRE(r984 == 0b0011010010110000);

    sr = shift_register<16>(a.data() + 528, 3, 10, 16);
    auto r985 = sr.shift();
    REQUIRE(r985 == 0b0110100101100000);

    sr = shift_register<16>(a.data() + 528, 2, 11, 16);
    auto r986 = sr.shift();
    REQUIRE(r986 == 0b1101001011000000);

    sr = shift_register<16>(a.data() + 528, 1, 12, 16);
    auto r987 = sr.shift();
    REQUIRE(r987 == 0b1010010110000001);

    sr = shift_register<16>(a.data() + 528, 0, 13, 16);
    auto r988 = sr.shift();
    REQUIRE(r988 == 0b0100101100000011);

    sr = shift_register<16>(a.data() + 529, 15, 14, 16);
    auto r989 = sr.shift();
    REQUIRE(r989 == 0b1001011000000111);

    sr = shift_register<16>(a.data() + 536, 11, 0, 16);
    auto r990 = sr.shift();
    REQUIRE(r990 == 0b0010000001111101);

    sr = shift_register<16>(a.data() + 536, 10, 1, 16);
    auto r991 = sr.shift();
    REQUIRE(r991 == 0b0100000011111010);

    sr = shift_register<16>(a.data() + 536, 9, 2, 16);
    auto r992 = sr.shift();
    REQUIRE(r992 == 0b1000000111110101);

    sr = shift_register<16>(a.data() + 536, 8, 3, 16);
    auto r993 = sr.shift();
    REQUIRE(r993 == 0b0000001111101011);

    sr = shift_register<16>(a.data() + 536, 7, 4, 16);
    auto r994 = sr.shift();
    REQUIRE(r994 == 0b0000011111010111);

    sr = shift_register<16>(a.data() + 536, 6, 5, 16);
    auto r995 = sr.shift();
    REQUIRE(r995 == 0b0000111110101111);

    sr = shift_register<16>(a.data() + 536, 5, 6, 16);
    auto r996 = sr.shift();
    REQUIRE(r996 == 0b0001111101011111);

    sr = shift_register<16>(a.data() + 536, 4, 7, 16);
    auto r997 = sr.shift();
    REQUIRE(r997 == 0b0011111010111111);

    sr = shift_register<16>(a.data() + 536, 3, 8, 16);
    auto r998 = sr.shift();
    REQUIRE(r998 == 0b0111110101111110);

    sr = shift_register<16>(a.data() + 536, 2, 9, 16);
    auto r999 = sr.shift();
    REQUIRE(r999 == 0b1111101011111101);

    sr = shift_register<16>(a.data() + 536, 1, 10, 16);
    auto r1000 = sr.shift();
    REQUIRE(r1000 == 0b1111010111111011);

    sr = shift_register<16>(a.data() + 536, 0, 11, 16);
    auto r1001 = sr.shift();
    REQUIRE(r1001 == 0b1110101111110110);

    sr = shift_register<16>(a.data() + 537, 15, 12, 16);
    auto r1002 = sr.shift();
    REQUIRE(r1002 == 0b1101011111101100);

    sr = shift_register<16>(a.data() + 537, 14, 13, 16);
    auto r1003 = sr.shift();
    REQUIRE(r1003 == 0b1010111111011000);

    sr = shift_register<16>(a.data() + 537, 13, 14, 16);
    auto r1004 = sr.shift();
    REQUIRE(r1004 == 0b0101111110110000);

    sr = shift_register<16>(a.data() + 544, 9, 0, 16);
    auto r1005 = sr.shift();
    REQUIRE(r1005 == 0b1000011001011011);

    sr = shift_register<16>(a.data() + 544, 8, 1, 16);
    auto r1006 = sr.shift();
    REQUIRE(r1006 == 0b0000110010110111);

    sr = shift_register<16>(a.data() + 544, 7, 2, 16);
    auto r1007 = sr.shift();
    REQUIRE(r1007 == 0b0001100101101110);

    sr = shift_register<16>(a.data() + 544, 6, 3, 16);
    auto r1008 = sr.shift();
    REQUIRE(r1008 == 0b0011001011011100);

    sr = shift_register<16>(a.data() + 544, 5, 4, 16);
    auto r1009 = sr.shift();
    REQUIRE(r1009 == 0b0110010110111001);

    sr = shift_register<16>(a.data() + 544, 4, 5, 16);
    auto r1010 = sr.shift();
    REQUIRE(r1010 == 0b1100101101110010);

    sr = shift_register<16>(a.data() + 544, 3, 6, 16);
    auto r1011 = sr.shift();
    REQUIRE(r1011 == 0b1001011011100100);

    sr = shift_register<16>(a.data() + 544, 2, 7, 16);
    auto r1012 = sr.shift();
    REQUIRE(r1012 == 0b0010110111001001);

    sr = shift_register<16>(a.data() + 544, 1, 8, 16);
    auto r1013 = sr.shift();
    REQUIRE(r1013 == 0b0101101110010011);

    sr = shift_register<16>(a.data() + 544, 0, 9, 16);
    auto r1014 = sr.shift();
    REQUIRE(r1014 == 0b1011011100100111);

    sr = shift_register<16>(a.data() + 545, 15, 10, 16);
    auto r1015 = sr.shift();
    REQUIRE(r1015 == 0b0110111001001111);

    sr = shift_register<16>(a.data() + 545, 14, 11, 16);
    auto r1016 = sr.shift();
    REQUIRE(r1016 == 0b1101110010011111);

    sr = shift_register<16>(a.data() + 545, 13, 12, 16);
    auto r1017 = sr.shift();
    REQUIRE(r1017 == 0b1011100100111110);

    sr = shift_register<16>(a.data() + 545, 12, 13, 16);
    auto r1018 = sr.shift();
    REQUIRE(r1018 == 0b0111001001111101);

    sr = shift_register<16>(a.data() + 545, 11, 14, 16);
    auto r1019 = sr.shift();
    REQUIRE(r1019 == 0b1110010011111010);

    sr = shift_register<16>(a.data() + 552, 7, 0, 16);
    auto r1020 = sr.shift();
    REQUIRE(r1020 == 0b1000010111000011);

    sr = shift_register<16>(a.data() + 552, 6, 1, 16);
    auto r1021 = sr.shift();
    REQUIRE(r1021 == 0b0000101110000111);

    sr = shift_register<16>(a.data() + 552, 5, 2, 16);
    auto r1022 = sr.shift();
    REQUIRE(r1022 == 0b0001011100001110);

    sr = shift_register<16>(a.data() + 552, 4, 3, 16);
    auto r1023 = sr.shift();
    REQUIRE(r1023 == 0b0010111000011101);

    sr = shift_register<16>(a.data() + 552, 3, 4, 16);
    auto r1024 = sr.shift();
    REQUIRE(r1024 == 0b0101110000111010);

    sr = shift_register<16>(a.data() + 552, 2, 5, 16);
    auto r1025 = sr.shift();
    REQUIRE(r1025 == 0b1011100001110100);

    sr = shift_register<16>(a.data() + 552, 1, 6, 16);
    auto r1026 = sr.shift();
    REQUIRE(r1026 == 0b0111000011101001);

    sr = shift_register<16>(a.data() + 552, 0, 7, 16);
    auto r1027 = sr.shift();
    REQUIRE(r1027 == 0b1110000111010011);

    sr = shift_register<16>(a.data() + 553, 15, 8, 16);
    auto r1028 = sr.shift();
    REQUIRE(r1028 == 0b1100001110100111);

    sr = shift_register<16>(a.data() + 553, 14, 9, 16);
    auto r1029 = sr.shift();
    REQUIRE(r1029 == 0b1000011101001111);

    sr = shift_register<16>(a.data() + 553, 13, 10, 16);
    auto r1030 = sr.shift();
    REQUIRE(r1030 == 0b0000111010011111);

    sr = shift_register<16>(a.data() + 553, 12, 11, 16);
    auto r1031 = sr.shift();
    REQUIRE(r1031 == 0b0001110100111110);

    sr = shift_register<16>(a.data() + 553, 11, 12, 16);
    auto r1032 = sr.shift();
    REQUIRE(r1032 == 0b0011101001111100);

    sr = shift_register<16>(a.data() + 553, 10, 13, 16);
    auto r1033 = sr.shift();
    REQUIRE(r1033 == 0b0111010011111001);

    sr = shift_register<16>(a.data() + 553, 9, 14, 16);
    auto r1034 = sr.shift();
    REQUIRE(r1034 == 0b1110100111110011);

    sr = shift_register<16>(a.data() + 560, 5, 0, 16);
    auto r1035 = sr.shift();
    REQUIRE(r1035 == 0b0001001010010000);

    sr = shift_register<16>(a.data() + 560, 4, 1, 16);
    auto r1036 = sr.shift();
    REQUIRE(r1036 == 0b0010010100100001);

    sr = shift_register<16>(a.data() + 560, 3, 2, 16);
    auto r1037 = sr.shift();
    REQUIRE(r1037 == 0b0100101001000010);

    sr = shift_register<16>(a.data() + 560, 2, 3, 16);
    auto r1038 = sr.shift();
    REQUIRE(r1038 == 0b1001010010000101);

    sr = shift_register<16>(a.data() + 560, 1, 4, 16);
    auto r1039 = sr.shift();
    REQUIRE(r1039 == 0b0010100100001010);

    sr = shift_register<16>(a.data() + 560, 0, 5, 16);
    auto r1040 = sr.shift();
    REQUIRE(r1040 == 0b0101001000010100);

    sr = shift_register<16>(a.data() + 561, 15, 6, 16);
    auto r1041 = sr.shift();
    REQUIRE(r1041 == 0b1010010000101000);

    sr = shift_register<16>(a.data() + 561, 14, 7, 16);
    auto r1042 = sr.shift();
    REQUIRE(r1042 == 0b0100100001010001);

    sr = shift_register<16>(a.data() + 561, 13, 8, 16);
    auto r1043 = sr.shift();
    REQUIRE(r1043 == 0b1001000010100011);

    sr = shift_register<16>(a.data() + 561, 12, 9, 16);
    auto r1044 = sr.shift();
    REQUIRE(r1044 == 0b0010000101000111);

    sr = shift_register<16>(a.data() + 561, 11, 10, 16);
    auto r1045 = sr.shift();
    REQUIRE(r1045 == 0b0100001010001110);

    sr = shift_register<16>(a.data() + 561, 10, 11, 16);
    auto r1046 = sr.shift();
    REQUIRE(r1046 == 0b1000010100011101);

    sr = shift_register<16>(a.data() + 561, 9, 12, 16);
    auto r1047 = sr.shift();
    REQUIRE(r1047 == 0b0000101000111011);

    sr = shift_register<16>(a.data() + 561, 8, 13, 16);
    auto r1048 = sr.shift();
    REQUIRE(r1048 == 0b0001010001110110);

    sr = shift_register<16>(a.data() + 561, 7, 14, 16);
    auto r1049 = sr.shift();
    REQUIRE(r1049 == 0b0010100011101101);

    sr = shift_register<16>(a.data() + 568, 3, 0, 16);
    auto r1050 = sr.shift();
    REQUIRE(r1050 == 0b1101000011010010);

    sr = shift_register<16>(a.data() + 568, 2, 1, 16);
    auto r1051 = sr.shift();
    REQUIRE(r1051 == 0b1010000110100100);

    sr = shift_register<16>(a.data() + 568, 1, 2, 16);
    auto r1052 = sr.shift();
    REQUIRE(r1052 == 0b0100001101001000);

    sr = shift_register<16>(a.data() + 568, 0, 3, 16);
    auto r1053 = sr.shift();
    REQUIRE(r1053 == 0b1000011010010001);

    sr = shift_register<16>(a.data() + 569, 15, 4, 16);
    auto r1054 = sr.shift();
    REQUIRE(r1054 == 0b0000110100100010);

    sr = shift_register<16>(a.data() + 569, 14, 5, 16);
    auto r1055 = sr.shift();
    REQUIRE(r1055 == 0b0001101001000101);

    sr = shift_register<16>(a.data() + 569, 13, 6, 16);
    auto r1056 = sr.shift();
    REQUIRE(r1056 == 0b0011010010001011);

    sr = shift_register<16>(a.data() + 569, 12, 7, 16);
    auto r1057 = sr.shift();
    REQUIRE(r1057 == 0b0110100100010111);

    sr = shift_register<16>(a.data() + 569, 11, 8, 16);
    auto r1058 = sr.shift();
    REQUIRE(r1058 == 0b1101001000101110);

    sr = shift_register<16>(a.data() + 569, 10, 9, 16);
    auto r1059 = sr.shift();
    REQUIRE(r1059 == 0b1010010001011101);

    sr = shift_register<16>(a.data() + 569, 9, 10, 16);
    auto r1060 = sr.shift();
    REQUIRE(r1060 == 0b0100100010111011);

    sr = shift_register<16>(a.data() + 569, 8, 11, 16);
    auto r1061 = sr.shift();
    REQUIRE(r1061 == 0b1001000101110111);

    sr = shift_register<16>(a.data() + 569, 7, 12, 16);
    auto r1062 = sr.shift();
    REQUIRE(r1062 == 0b0010001011101110);

    sr = shift_register<16>(a.data() + 569, 6, 13, 16);
    auto r1063 = sr.shift();
    REQUIRE(r1063 == 0b0100010111011100);

    sr = shift_register<16>(a.data() + 569, 5, 14, 16);
    auto r1064 = sr.shift();
    REQUIRE(r1064 == 0b1000101110111001);

    sr = shift_register<16>(a.data() + 576, 1, 0, 16);
    auto r1065 = sr.shift();
    REQUIRE(r1065 == 0b0100111101000011);

    sr = shift_register<16>(a.data() + 576, 0, 1, 16);
    auto r1066 = sr.shift();
    REQUIRE(r1066 == 0b1001111010000111);

    sr = shift_register<16>(a.data() + 577, 15, 2, 16);
    auto r1067 = sr.shift();
    REQUIRE(r1067 == 0b0011110100001110);

    sr = shift_register<16>(a.data() + 577, 14, 3, 16);
    auto r1068 = sr.shift();
    REQUIRE(r1068 == 0b0111101000011101);

    sr = shift_register<16>(a.data() + 577, 13, 4, 16);
    auto r1069 = sr.shift();
    REQUIRE(r1069 == 0b1111010000111011);

    sr = shift_register<16>(a.data() + 577, 12, 5, 16);
    auto r1070 = sr.shift();
    REQUIRE(r1070 == 0b1110100001110110);

    sr = shift_register<16>(a.data() + 577, 11, 6, 16);
    auto r1071 = sr.shift();
    REQUIRE(r1071 == 0b1101000011101100);

    sr = shift_register<16>(a.data() + 577, 10, 7, 16);
    auto r1072 = sr.shift();
    REQUIRE(r1072 == 0b1010000111011000);

    sr = shift_register<16>(a.data() + 577, 9, 8, 16);
    auto r1073 = sr.shift();
    REQUIRE(r1073 == 0b0100001110110001);

    sr = shift_register<16>(a.data() + 577, 8, 9, 16);
    auto r1074 = sr.shift();
    REQUIRE(r1074 == 0b1000011101100011);

    sr = shift_register<16>(a.data() + 577, 7, 10, 16);
    auto r1075 = sr.shift();
    REQUIRE(r1075 == 0b0000111011000111);

    sr = shift_register<16>(a.data() + 577, 6, 11, 16);
    auto r1076 = sr.shift();
    REQUIRE(r1076 == 0b0001110110001110);

    sr = shift_register<16>(a.data() + 577, 5, 12, 16);
    auto r1077 = sr.shift();
    REQUIRE(r1077 == 0b0011101100011100);

    sr = shift_register<16>(a.data() + 577, 4, 13, 16);
    auto r1078 = sr.shift();
    REQUIRE(r1078 == 0b0111011000111000);

    sr = shift_register<16>(a.data() + 577, 3, 14, 16);
    auto r1079 = sr.shift();
    REQUIRE(r1079 == 0b1110110001110001);

    sr = shift_register<16>(a.data() + 585, 15, 0, 16);
    auto r1080 = sr.shift();
    REQUIRE(r1080 == 0b1111111101001110);

    sr = shift_register<16>(a.data() + 585, 14, 1, 16);
    auto r1081 = sr.shift();
    REQUIRE(r1081 == 0b1111111010011100);

    sr = shift_register<16>(a.data() + 585, 13, 2, 16);
    auto r1082 = sr.shift();
    REQUIRE(r1082 == 0b1111110100111001);

    sr = shift_register<16>(a.data() + 585, 12, 3, 16);
    auto r1083 = sr.shift();
    REQUIRE(r1083 == 0b1111101001110010);

    sr = shift_register<16>(a.data() + 585, 11, 4, 16);
    auto r1084 = sr.shift();
    REQUIRE(r1084 == 0b1111010011100101);

    sr = shift_register<16>(a.data() + 585, 10, 5, 16);
    auto r1085 = sr.shift();
    REQUIRE(r1085 == 0b1110100111001011);

    sr = shift_register<16>(a.data() + 585, 9, 6, 16);
    auto r1086 = sr.shift();
    REQUIRE(r1086 == 0b1101001110010110);

    sr = shift_register<16>(a.data() + 585, 8, 7, 16);
    auto r1087 = sr.shift();
    REQUIRE(r1087 == 0b1010011100101100);

    sr = shift_register<16>(a.data() + 585, 7, 8, 16);
    auto r1088 = sr.shift();
    REQUIRE(r1088 == 0b0100111001011001);

    sr = shift_register<16>(a.data() + 585, 6, 9, 16);
    auto r1089 = sr.shift();
    REQUIRE(r1089 == 0b1001110010110011);

    sr = shift_register<16>(a.data() + 585, 5, 10, 16);
    auto r1090 = sr.shift();
    REQUIRE(r1090 == 0b0011100101100110);

    sr = shift_register<16>(a.data() + 585, 4, 11, 16);
    auto r1091 = sr.shift();
    REQUIRE(r1091 == 0b0111001011001100);

    sr = shift_register<16>(a.data() + 585, 3, 12, 16);
    auto r1092 = sr.shift();
    REQUIRE(r1092 == 0b1110010110011000);

    sr = shift_register<16>(a.data() + 585, 2, 13, 16);
    auto r1093 = sr.shift();
    REQUIRE(r1093 == 0b1100101100110001);

    sr = shift_register<16>(a.data() + 585, 1, 14, 16);
    auto r1094 = sr.shift();
    REQUIRE(r1094 == 0b1001011001100010);

    sr = shift_register<16>(a.data() + 593, 13, 0, 16);
    auto r1095 = sr.shift();
    REQUIRE(r1095 == 0b0010001100100001);

    sr = shift_register<16>(a.data() + 593, 12, 1, 16);
    auto r1096 = sr.shift();
    REQUIRE(r1096 == 0b0100011001000010);

    sr = shift_register<16>(a.data() + 593, 11, 2, 16);
    auto r1097 = sr.shift();
    REQUIRE(r1097 == 0b1000110010000100);

    sr = shift_register<16>(a.data() + 593, 10, 3, 16);
    auto r1098 = sr.shift();
    REQUIRE(r1098 == 0b0001100100001000);

    sr = shift_register<16>(a.data() + 593, 9, 4, 16);
    auto r1099 = sr.shift();
    REQUIRE(r1099 == 0b0011001000010001);

    sr = shift_register<16>(a.data() + 593, 8, 5, 16);
    auto r1100 = sr.shift();
    REQUIRE(r1100 == 0b0110010000100011);

    sr = shift_register<16>(a.data() + 593, 7, 6, 16);
    auto r1101 = sr.shift();
    REQUIRE(r1101 == 0b1100100001000110);

    sr = shift_register<16>(a.data() + 593, 6, 7, 16);
    auto r1102 = sr.shift();
    REQUIRE(r1102 == 0b1001000010001101);

    sr = shift_register<16>(a.data() + 593, 5, 8, 16);
    auto r1103 = sr.shift();
    REQUIRE(r1103 == 0b0010000100011010);

    sr = shift_register<16>(a.data() + 593, 4, 9, 16);
    auto r1104 = sr.shift();
    REQUIRE(r1104 == 0b0100001000110101);

    sr = shift_register<16>(a.data() + 593, 3, 10, 16);
    auto r1105 = sr.shift();
    REQUIRE(r1105 == 0b1000010001101010);

    sr = shift_register<16>(a.data() + 593, 2, 11, 16);
    auto r1106 = sr.shift();
    REQUIRE(r1106 == 0b0000100011010100);

    sr = shift_register<16>(a.data() + 593, 1, 12, 16);
    auto r1107 = sr.shift();
    REQUIRE(r1107 == 0b0001000110101001);

    sr = shift_register<16>(a.data() + 593, 0, 13, 16);
    auto r1108 = sr.shift();
    REQUIRE(r1108 == 0b0010001101010010);

    sr = shift_register<16>(a.data() + 594, 15, 14, 16);
    auto r1109 = sr.shift();
    REQUIRE(r1109 == 0b0100011010100100);

    sr = shift_register<16>(a.data() + 601, 11, 0, 16);
    auto r1110 = sr.shift();
    REQUIRE(r1110 == 0b1100110000110101);

    sr = shift_register<16>(a.data() + 601, 10, 1, 16);
    auto r1111 = sr.shift();
    REQUIRE(r1111 == 0b1001100001101010);

    sr = shift_register<16>(a.data() + 601, 9, 2, 16);
    auto r1112 = sr.shift();
    REQUIRE(r1112 == 0b0011000011010100);

    sr = shift_register<16>(a.data() + 601, 8, 3, 16);
    auto r1113 = sr.shift();
    REQUIRE(r1113 == 0b0110000110101001);

    sr = shift_register<16>(a.data() + 601, 7, 4, 16);
    auto r1114 = sr.shift();
    REQUIRE(r1114 == 0b1100001101010010);

    sr = shift_register<16>(a.data() + 601, 6, 5, 16);
    auto r1115 = sr.shift();
    REQUIRE(r1115 == 0b1000011010100100);

    sr = shift_register<16>(a.data() + 601, 5, 6, 16);
    auto r1116 = sr.shift();
    REQUIRE(r1116 == 0b0000110101001001);

    sr = shift_register<16>(a.data() + 601, 4, 7, 16);
    auto r1117 = sr.shift();
    REQUIRE(r1117 == 0b0001101010010010);

    sr = shift_register<16>(a.data() + 601, 3, 8, 16);
    auto r1118 = sr.shift();
    REQUIRE(r1118 == 0b0011010100100101);

    sr = shift_register<16>(a.data() + 601, 2, 9, 16);
    auto r1119 = sr.shift();
    REQUIRE(r1119 == 0b0110101001001011);

    sr = shift_register<16>(a.data() + 601, 1, 10, 16);
    auto r1120 = sr.shift();
    REQUIRE(r1120 == 0b1101010010010110);

    sr = shift_register<16>(a.data() + 601, 0, 11, 16);
    auto r1121 = sr.shift();
    REQUIRE(r1121 == 0b1010100100101100);

    sr = shift_register<16>(a.data() + 602, 15, 12, 16);
    auto r1122 = sr.shift();
    REQUIRE(r1122 == 0b0101001001011000);

    sr = shift_register<16>(a.data() + 602, 14, 13, 16);
    auto r1123 = sr.shift();
    REQUIRE(r1123 == 0b1010010010110001);

    sr = shift_register<16>(a.data() + 602, 13, 14, 16);
    auto r1124 = sr.shift();
    REQUIRE(r1124 == 0b0100100101100010);

    sr = shift_register<16>(a.data() + 609, 9, 0, 16);
    auto r1125 = sr.shift();
    REQUIRE(r1125 == 0b1011101001001100);

    sr = shift_register<16>(a.data() + 609, 8, 1, 16);
    auto r1126 = sr.shift();
    REQUIRE(r1126 == 0b0111010010011000);

    sr = shift_register<16>(a.data() + 609, 7, 2, 16);
    auto r1127 = sr.shift();
    REQUIRE(r1127 == 0b1110100100110000);

    sr = shift_register<16>(a.data() + 609, 6, 3, 16);
    auto r1128 = sr.shift();
    REQUIRE(r1128 == 0b1101001001100000);

    sr = shift_register<16>(a.data() + 609, 5, 4, 16);
    auto r1129 = sr.shift();
    REQUIRE(r1129 == 0b1010010011000000);

    sr = shift_register<16>(a.data() + 609, 4, 5, 16);
    auto r1130 = sr.shift();
    REQUIRE(r1130 == 0b0100100110000001);

    sr = shift_register<16>(a.data() + 609, 3, 6, 16);
    auto r1131 = sr.shift();
    REQUIRE(r1131 == 0b1001001100000010);

    sr = shift_register<16>(a.data() + 609, 2, 7, 16);
    auto r1132 = sr.shift();
    REQUIRE(r1132 == 0b0010011000000100);

    sr = shift_register<16>(a.data() + 609, 1, 8, 16);
    auto r1133 = sr.shift();
    REQUIRE(r1133 == 0b0100110000001000);

    sr = shift_register<16>(a.data() + 609, 0, 9, 16);
    auto r1134 = sr.shift();
    REQUIRE(r1134 == 0b1001100000010001);

    sr = shift_register<16>(a.data() + 610, 15, 10, 16);
    auto r1135 = sr.shift();
    REQUIRE(r1135 == 0b0011000000100010);

    sr = shift_register<16>(a.data() + 610, 14, 11, 16);
    auto r1136 = sr.shift();
    REQUIRE(r1136 == 0b0110000001000101);

    sr = shift_register<16>(a.data() + 610, 13, 12, 16);
    auto r1137 = sr.shift();
    REQUIRE(r1137 == 0b1100000010001011);

    sr = shift_register<16>(a.data() + 610, 12, 13, 16);
    auto r1138 = sr.shift();
    REQUIRE(r1138 == 0b1000000100010110);

    sr = shift_register<16>(a.data() + 610, 11, 14, 16);
    auto r1139 = sr.shift();
    REQUIRE(r1139 == 0b0000001000101100);

    sr = shift_register<16>(a.data() + 617, 7, 0, 16);
    auto r1140 = sr.shift();
    REQUIRE(r1140 == 0b1110001011110111);

    sr = shift_register<16>(a.data() + 617, 6, 1, 16);
    auto r1141 = sr.shift();
    REQUIRE(r1141 == 0b1100010111101111);

    sr = shift_register<16>(a.data() + 617, 5, 2, 16);
    auto r1142 = sr.shift();
    REQUIRE(r1142 == 0b1000101111011110);

    sr = shift_register<16>(a.data() + 617, 4, 3, 16);
    auto r1143 = sr.shift();
    REQUIRE(r1143 == 0b0001011110111100);

    sr = shift_register<16>(a.data() + 617, 3, 4, 16);
    auto r1144 = sr.shift();
    REQUIRE(r1144 == 0b0010111101111000);

    sr = shift_register<16>(a.data() + 617, 2, 5, 16);
    auto r1145 = sr.shift();
    REQUIRE(r1145 == 0b0101111011110001);

    sr = shift_register<16>(a.data() + 617, 1, 6, 16);
    auto r1146 = sr.shift();
    REQUIRE(r1146 == 0b1011110111100011);

    sr = shift_register<16>(a.data() + 617, 0, 7, 16);
    auto r1147 = sr.shift();
    REQUIRE(r1147 == 0b0111101111000110);

    sr = shift_register<16>(a.data() + 618, 15, 8, 16);
    auto r1148 = sr.shift();
    REQUIRE(r1148 == 0b1111011110001101);

    sr = shift_register<16>(a.data() + 618, 14, 9, 16);
    auto r1149 = sr.shift();
    REQUIRE(r1149 == 0b1110111100011011);

    sr = shift_register<16>(a.data() + 618, 13, 10, 16);
    auto r1150 = sr.shift();
    REQUIRE(r1150 == 0b1101111000110110);

    sr = shift_register<16>(a.data() + 618, 12, 11, 16);
    auto r1151 = sr.shift();
    REQUIRE(r1151 == 0b1011110001101101);

    sr = shift_register<16>(a.data() + 618, 11, 12, 16);
    auto r1152 = sr.shift();
    REQUIRE(r1152 == 0b0111100011011010);

    sr = shift_register<16>(a.data() + 618, 10, 13, 16);
    auto r1153 = sr.shift();
    REQUIRE(r1153 == 0b1111000110110100);

    sr = shift_register<16>(a.data() + 618, 9, 14, 16);
    auto r1154 = sr.shift();
    REQUIRE(r1154 == 0b1110001101101001);

    sr = shift_register<16>(a.data() + 625, 5, 0, 16);
    auto r1155 = sr.shift();
    REQUIRE(r1155 == 0b1111100001101001);

    sr = shift_register<16>(a.data() + 625, 4, 1, 16);
    auto r1156 = sr.shift();
    REQUIRE(r1156 == 0b1111000011010010);

    sr = shift_register<16>(a.data() + 625, 3, 2, 16);
    auto r1157 = sr.shift();
    REQUIRE(r1157 == 0b1110000110100100);

    sr = shift_register<16>(a.data() + 625, 2, 3, 16);
    auto r1158 = sr.shift();
    REQUIRE(r1158 == 0b1100001101001001);

    sr = shift_register<16>(a.data() + 625, 1, 4, 16);
    auto r1159 = sr.shift();
    REQUIRE(r1159 == 0b1000011010010011);

    sr = shift_register<16>(a.data() + 625, 0, 5, 16);
    auto r1160 = sr.shift();
    REQUIRE(r1160 == 0b0000110100100110);

    sr = shift_register<16>(a.data() + 626, 15, 6, 16);
    auto r1161 = sr.shift();
    REQUIRE(r1161 == 0b0001101001001100);

    sr = shift_register<16>(a.data() + 626, 14, 7, 16);
    auto r1162 = sr.shift();
    REQUIRE(r1162 == 0b0011010010011001);

    sr = shift_register<16>(a.data() + 626, 13, 8, 16);
    auto r1163 = sr.shift();
    REQUIRE(r1163 == 0b0110100100110011);

    sr = shift_register<16>(a.data() + 626, 12, 9, 16);
    auto r1164 = sr.shift();
    REQUIRE(r1164 == 0b1101001001100111);

    sr = shift_register<16>(a.data() + 626, 11, 10, 16);
    auto r1165 = sr.shift();
    REQUIRE(r1165 == 0b1010010011001111);

    sr = shift_register<16>(a.data() + 626, 10, 11, 16);
    auto r1166 = sr.shift();
    REQUIRE(r1166 == 0b0100100110011110);

    sr = shift_register<16>(a.data() + 626, 9, 12, 16);
    auto r1167 = sr.shift();
    REQUIRE(r1167 == 0b1001001100111101);

    sr = shift_register<16>(a.data() + 626, 8, 13, 16);
    auto r1168 = sr.shift();
    REQUIRE(r1168 == 0b0010011001111010);

    sr = shift_register<16>(a.data() + 626, 7, 14, 16);
    auto r1169 = sr.shift();
    REQUIRE(r1169 == 0b0100110011110100);

    sr = shift_register<16>(a.data() + 633, 3, 0, 16);
    auto r1170 = sr.shift();
    REQUIRE(r1170 == 0b1000001101100100);

    sr = shift_register<16>(a.data() + 633, 2, 1, 16);
    auto r1171 = sr.shift();
    REQUIRE(r1171 == 0b0000011011001001);

    sr = shift_register<16>(a.data() + 633, 1, 2, 16);
    auto r1172 = sr.shift();
    REQUIRE(r1172 == 0b0000110110010010);

    sr = shift_register<16>(a.data() + 633, 0, 3, 16);
    auto r1173 = sr.shift();
    REQUIRE(r1173 == 0b0001101100100100);

    sr = shift_register<16>(a.data() + 634, 15, 4, 16);
    auto r1174 = sr.shift();
    REQUIRE(r1174 == 0b0011011001001001);

    sr = shift_register<16>(a.data() + 634, 14, 5, 16);
    auto r1175 = sr.shift();
    REQUIRE(r1175 == 0b0110110010010010);

    sr = shift_register<16>(a.data() + 634, 13, 6, 16);
    auto r1176 = sr.shift();
    REQUIRE(r1176 == 0b1101100100100101);

    sr = shift_register<16>(a.data() + 634, 12, 7, 16);
    auto r1177 = sr.shift();
    REQUIRE(r1177 == 0b1011001001001010);

    sr = shift_register<16>(a.data() + 634, 11, 8, 16);
    auto r1178 = sr.shift();
    REQUIRE(r1178 == 0b0110010010010101);

    sr = shift_register<16>(a.data() + 634, 10, 9, 16);
    auto r1179 = sr.shift();
    REQUIRE(r1179 == 0b1100100100101010);

    sr = shift_register<16>(a.data() + 634, 9, 10, 16);
    auto r1180 = sr.shift();
    REQUIRE(r1180 == 0b1001001001010100);

    sr = shift_register<16>(a.data() + 634, 8, 11, 16);
    auto r1181 = sr.shift();
    REQUIRE(r1181 == 0b0010010010101001);

    sr = shift_register<16>(a.data() + 634, 7, 12, 16);
    auto r1182 = sr.shift();
    REQUIRE(r1182 == 0b0100100101010011);

    sr = shift_register<16>(a.data() + 634, 6, 13, 16);
    auto r1183 = sr.shift();
    REQUIRE(r1183 == 0b1001001010100110);

    sr = shift_register<16>(a.data() + 634, 5, 14, 16);
    auto r1184 = sr.shift();
    REQUIRE(r1184 == 0b0010010101001101);

    sr = shift_register<16>(a.data() + 641, 1, 0, 16);
    auto r1185 = sr.shift();
    REQUIRE(r1185 == 0b0010111010110000);

    sr = shift_register<16>(a.data() + 641, 0, 1, 16);
    auto r1186 = sr.shift();
    REQUIRE(r1186 == 0b0101110101100000);

    sr = shift_register<16>(a.data() + 642, 15, 2, 16);
    auto r1187 = sr.shift();
    REQUIRE(r1187 == 0b1011101011000000);

    sr = shift_register<16>(a.data() + 642, 14, 3, 16);
    auto r1188 = sr.shift();
    REQUIRE(r1188 == 0b0111010110000000);

    sr = shift_register<16>(a.data() + 642, 13, 4, 16);
    auto r1189 = sr.shift();
    REQUIRE(r1189 == 0b1110101100000000);

    sr = shift_register<16>(a.data() + 642, 12, 5, 16);
    auto r1190 = sr.shift();
    REQUIRE(r1190 == 0b1101011000000001);

    sr = shift_register<16>(a.data() + 642, 11, 6, 16);
    auto r1191 = sr.shift();
    REQUIRE(r1191 == 0b1010110000000011);

    sr = shift_register<16>(a.data() + 642, 10, 7, 16);
    auto r1192 = sr.shift();
    REQUIRE(r1192 == 0b0101100000000110);

    sr = shift_register<16>(a.data() + 642, 9, 8, 16);
    auto r1193 = sr.shift();
    REQUIRE(r1193 == 0b1011000000001101);

    sr = shift_register<16>(a.data() + 642, 8, 9, 16);
    auto r1194 = sr.shift();
    REQUIRE(r1194 == 0b0110000000011011);

    sr = shift_register<16>(a.data() + 642, 7, 10, 16);
    auto r1195 = sr.shift();
    REQUIRE(r1195 == 0b1100000000110110);

    sr = shift_register<16>(a.data() + 642, 6, 11, 16);
    auto r1196 = sr.shift();
    REQUIRE(r1196 == 0b1000000001101100);

    sr = shift_register<16>(a.data() + 642, 5, 12, 16);
    auto r1197 = sr.shift();
    REQUIRE(r1197 == 0b0000000011011000);

    sr = shift_register<16>(a.data() + 642, 4, 13, 16);
    auto r1198 = sr.shift();
    REQUIRE(r1198 == 0b0000000110110001);

    sr = shift_register<16>(a.data() + 642, 3, 14, 16);
    auto r1199 = sr.shift();
    REQUIRE(r1199 == 0b0000001101100011);

    sr = shift_register<16>(a.data() + 650, 15, 0, 16);
    auto r1200 = sr.shift();
    REQUIRE(r1200 == 0b1000101001111000);

    sr = shift_register<16>(a.data() + 650, 14, 1, 16);
    auto r1201 = sr.shift();
    REQUIRE(r1201 == 0b0001010011110001);

    sr = shift_register<16>(a.data() + 650, 13, 2, 16);
    auto r1202 = sr.shift();
    REQUIRE(r1202 == 0b0010100111100011);

    sr = shift_register<16>(a.data() + 650, 12, 3, 16);
    auto r1203 = sr.shift();
    REQUIRE(r1203 == 0b0101001111000110);

    sr = shift_register<16>(a.data() + 650, 11, 4, 16);
    auto r1204 = sr.shift();
    REQUIRE(r1204 == 0b1010011110001100);

    sr = shift_register<16>(a.data() + 650, 10, 5, 16);
    auto r1205 = sr.shift();
    REQUIRE(r1205 == 0b0100111100011001);

    sr = shift_register<16>(a.data() + 650, 9, 6, 16);
    auto r1206 = sr.shift();
    REQUIRE(r1206 == 0b1001111000110010);

    sr = shift_register<16>(a.data() + 650, 8, 7, 16);
    auto r1207 = sr.shift();
    REQUIRE(r1207 == 0b0011110001100101);

    sr = shift_register<16>(a.data() + 650, 7, 8, 16);
    auto r1208 = sr.shift();
    REQUIRE(r1208 == 0b0111100011001011);

    sr = shift_register<16>(a.data() + 650, 6, 9, 16);
    auto r1209 = sr.shift();
    REQUIRE(r1209 == 0b1111000110010111);

    sr = shift_register<16>(a.data() + 650, 5, 10, 16);
    auto r1210 = sr.shift();
    REQUIRE(r1210 == 0b1110001100101111);

    sr = shift_register<16>(a.data() + 650, 4, 11, 16);
    auto r1211 = sr.shift();
    REQUIRE(r1211 == 0b1100011001011111);

    sr = shift_register<16>(a.data() + 650, 3, 12, 16);
    auto r1212 = sr.shift();
    REQUIRE(r1212 == 0b1000110010111111);

    sr = shift_register<16>(a.data() + 650, 2, 13, 16);
    auto r1213 = sr.shift();
    REQUIRE(r1213 == 0b0001100101111111);

    sr = shift_register<16>(a.data() + 650, 1, 14, 16);
    auto r1214 = sr.shift();
    REQUIRE(r1214 == 0b0011001011111111);

    sr = shift_register<16>(a.data() + 658, 13, 0, 16);
    auto r1215 = sr.shift();
    REQUIRE(r1215 == 0b1001101101011011);

    sr = shift_register<16>(a.data() + 658, 12, 1, 16);
    auto r1216 = sr.shift();
    REQUIRE(r1216 == 0b0011011010110110);

    sr = shift_register<16>(a.data() + 658, 11, 2, 16);
    auto r1217 = sr.shift();
    REQUIRE(r1217 == 0b0110110101101101);

    sr = shift_register<16>(a.data() + 658, 10, 3, 16);
    auto r1218 = sr.shift();
    REQUIRE(r1218 == 0b1101101011011011);

    sr = shift_register<16>(a.data() + 658, 9, 4, 16);
    auto r1219 = sr.shift();
    REQUIRE(r1219 == 0b1011010110110110);

    sr = shift_register<16>(a.data() + 658, 8, 5, 16);
    auto r1220 = sr.shift();
    REQUIRE(r1220 == 0b0110101101101100);

    sr = shift_register<16>(a.data() + 658, 7, 6, 16);
    auto r1221 = sr.shift();
    REQUIRE(r1221 == 0b1101011011011001);

    sr = shift_register<16>(a.data() + 658, 6, 7, 16);
    auto r1222 = sr.shift();
    REQUIRE(r1222 == 0b1010110110110010);

    sr = shift_register<16>(a.data() + 658, 5, 8, 16);
    auto r1223 = sr.shift();
    REQUIRE(r1223 == 0b0101101101100100);

    sr = shift_register<16>(a.data() + 658, 4, 9, 16);
    auto r1224 = sr.shift();
    REQUIRE(r1224 == 0b1011011011001000);

    sr = shift_register<16>(a.data() + 658, 3, 10, 16);
    auto r1225 = sr.shift();
    REQUIRE(r1225 == 0b0110110110010000);

    sr = shift_register<16>(a.data() + 658, 2, 11, 16);
    auto r1226 = sr.shift();
    REQUIRE(r1226 == 0b1101101100100000);

    sr = shift_register<16>(a.data() + 658, 1, 12, 16);
    auto r1227 = sr.shift();
    REQUIRE(r1227 == 0b1011011001000001);

    sr = shift_register<16>(a.data() + 658, 0, 13, 16);
    auto r1228 = sr.shift();
    REQUIRE(r1228 == 0b0110110010000010);

    sr = shift_register<16>(a.data() + 659, 15, 14, 16);
    auto r1229 = sr.shift();
    REQUIRE(r1229 == 0b1101100100000100);

    sr = shift_register<16>(a.data() + 666, 11, 0, 16);
    auto r1230 = sr.shift();
    REQUIRE(r1230 == 0b0000100111011100);

    sr = shift_register<16>(a.data() + 666, 10, 1, 16);
    auto r1231 = sr.shift();
    REQUIRE(r1231 == 0b0001001110111000);

    sr = shift_register<16>(a.data() + 666, 9, 2, 16);
    auto r1232 = sr.shift();
    REQUIRE(r1232 == 0b0010011101110000);

    sr = shift_register<16>(a.data() + 666, 8, 3, 16);
    auto r1233 = sr.shift();
    REQUIRE(r1233 == 0b0100111011100001);

    sr = shift_register<16>(a.data() + 666, 7, 4, 16);
    auto r1234 = sr.shift();
    REQUIRE(r1234 == 0b1001110111000010);

    sr = shift_register<16>(a.data() + 666, 6, 5, 16);
    auto r1235 = sr.shift();
    REQUIRE(r1235 == 0b0011101110000100);

    sr = shift_register<16>(a.data() + 666, 5, 6, 16);
    auto r1236 = sr.shift();
    REQUIRE(r1236 == 0b0111011100001000);

    sr = shift_register<16>(a.data() + 666, 4, 7, 16);
    auto r1237 = sr.shift();
    REQUIRE(r1237 == 0b1110111000010000);

    sr = shift_register<16>(a.data() + 666, 3, 8, 16);
    auto r1238 = sr.shift();
    REQUIRE(r1238 == 0b1101110000100000);

    sr = shift_register<16>(a.data() + 666, 2, 9, 16);
    auto r1239 = sr.shift();
    REQUIRE(r1239 == 0b1011100001000000);

    sr = shift_register<16>(a.data() + 666, 1, 10, 16);
    auto r1240 = sr.shift();
    REQUIRE(r1240 == 0b0111000010000000);

    sr = shift_register<16>(a.data() + 666, 0, 11, 16);
    auto r1241 = sr.shift();
    REQUIRE(r1241 == 0b1110000100000000);

    sr = shift_register<16>(a.data() + 667, 15, 12, 16);
    auto r1242 = sr.shift();
    REQUIRE(r1242 == 0b1100001000000001);

    sr = shift_register<16>(a.data() + 667, 14, 13, 16);
    auto r1243 = sr.shift();
    REQUIRE(r1243 == 0b1000010000000010);

    sr = shift_register<16>(a.data() + 667, 13, 14, 16);
    auto r1244 = sr.shift();
    REQUIRE(r1244 == 0b0000100000000100);

    sr = shift_register<16>(a.data() + 674, 9, 0, 16);
    auto r1245 = sr.shift();
    REQUIRE(r1245 == 0b0111101101000110);

    sr = shift_register<16>(a.data() + 674, 8, 1, 16);
    auto r1246 = sr.shift();
    REQUIRE(r1246 == 0b1111011010001101);

    sr = shift_register<16>(a.data() + 674, 7, 2, 16);
    auto r1247 = sr.shift();
    REQUIRE(r1247 == 0b1110110100011011);

    sr = shift_register<16>(a.data() + 674, 6, 3, 16);
    auto r1248 = sr.shift();
    REQUIRE(r1248 == 0b1101101000110110);

    sr = shift_register<16>(a.data() + 674, 5, 4, 16);
    auto r1249 = sr.shift();
    REQUIRE(r1249 == 0b1011010001101100);

    sr = shift_register<16>(a.data() + 674, 4, 5, 16);
    auto r1250 = sr.shift();
    REQUIRE(r1250 == 0b0110100011011001);

    sr = shift_register<16>(a.data() + 674, 3, 6, 16);
    auto r1251 = sr.shift();
    REQUIRE(r1251 == 0b1101000110110010);

    sr = shift_register<16>(a.data() + 674, 2, 7, 16);
    auto r1252 = sr.shift();
    REQUIRE(r1252 == 0b1010001101100100);

    sr = shift_register<16>(a.data() + 674, 1, 8, 16);
    auto r1253 = sr.shift();
    REQUIRE(r1253 == 0b0100011011001001);

    sr = shift_register<16>(a.data() + 674, 0, 9, 16);
    auto r1254 = sr.shift();
    REQUIRE(r1254 == 0b1000110110010010);

    sr = shift_register<16>(a.data() + 675, 15, 10, 16);
    auto r1255 = sr.shift();
    REQUIRE(r1255 == 0b0001101100100101);

    sr = shift_register<16>(a.data() + 675, 14, 11, 16);
    auto r1256 = sr.shift();
    REQUIRE(r1256 == 0b0011011001001011);

    sr = shift_register<16>(a.data() + 675, 13, 12, 16);
    auto r1257 = sr.shift();
    REQUIRE(r1257 == 0b0110110010010110);

    sr = shift_register<16>(a.data() + 675, 12, 13, 16);
    auto r1258 = sr.shift();
    REQUIRE(r1258 == 0b1101100100101100);

    sr = shift_register<16>(a.data() + 675, 11, 14, 16);
    auto r1259 = sr.shift();
    REQUIRE(r1259 == 0b1011001001011001);

    sr = shift_register<16>(a.data() + 682, 7, 0, 16);
    auto r1260 = sr.shift();
    REQUIRE(r1260 == 0b0001011110010010);

    sr = shift_register<16>(a.data() + 682, 6, 1, 16);
    auto r1261 = sr.shift();
    REQUIRE(r1261 == 0b0010111100100100);

    sr = shift_register<16>(a.data() + 682, 5, 2, 16);
    auto r1262 = sr.shift();
    REQUIRE(r1262 == 0b0101111001001001);

    sr = shift_register<16>(a.data() + 682, 4, 3, 16);
    auto r1263 = sr.shift();
    REQUIRE(r1263 == 0b1011110010010011);

    sr = shift_register<16>(a.data() + 682, 3, 4, 16);
    auto r1264 = sr.shift();
    REQUIRE(r1264 == 0b0111100100100110);

    sr = shift_register<16>(a.data() + 682, 2, 5, 16);
    auto r1265 = sr.shift();
    REQUIRE(r1265 == 0b1111001001001100);

    sr = shift_register<16>(a.data() + 682, 1, 6, 16);
    auto r1266 = sr.shift();
    REQUIRE(r1266 == 0b1110010010011000);

    sr = shift_register<16>(a.data() + 682, 0, 7, 16);
    auto r1267 = sr.shift();
    REQUIRE(r1267 == 0b1100100100110001);

    sr = shift_register<16>(a.data() + 683, 15, 8, 16);
    auto r1268 = sr.shift();
    REQUIRE(r1268 == 0b1001001001100011);

    sr = shift_register<16>(a.data() + 683, 14, 9, 16);
    auto r1269 = sr.shift();
    REQUIRE(r1269 == 0b0010010011000111);

    sr = shift_register<16>(a.data() + 683, 13, 10, 16);
    auto r1270 = sr.shift();
    REQUIRE(r1270 == 0b0100100110001111);

    sr = shift_register<16>(a.data() + 683, 12, 11, 16);
    auto r1271 = sr.shift();
    REQUIRE(r1271 == 0b1001001100011110);

    sr = shift_register<16>(a.data() + 683, 11, 12, 16);
    auto r1272 = sr.shift();
    REQUIRE(r1272 == 0b0010011000111101);

    sr = shift_register<16>(a.data() + 683, 10, 13, 16);
    auto r1273 = sr.shift();
    REQUIRE(r1273 == 0b0100110001111011);

    sr = shift_register<16>(a.data() + 683, 9, 14, 16);
    auto r1274 = sr.shift();
    REQUIRE(r1274 == 0b1001100011110111);

    sr = shift_register<16>(a.data() + 690, 5, 0, 16);
    auto r1275 = sr.shift();
    REQUIRE(r1275 == 0b0001111101110100);

    sr = shift_register<16>(a.data() + 690, 4, 1, 16);
    auto r1276 = sr.shift();
    REQUIRE(r1276 == 0b0011111011101000);

    sr = shift_register<16>(a.data() + 690, 3, 2, 16);
    auto r1277 = sr.shift();
    REQUIRE(r1277 == 0b0111110111010001);

    sr = shift_register<16>(a.data() + 690, 2, 3, 16);
    auto r1278 = sr.shift();
    REQUIRE(r1278 == 0b1111101110100011);

    sr = shift_register<16>(a.data() + 690, 1, 4, 16);
    auto r1279 = sr.shift();
    REQUIRE(r1279 == 0b1111011101000111);

    sr = shift_register<16>(a.data() + 690, 0, 5, 16);
    auto r1280 = sr.shift();
    REQUIRE(r1280 == 0b1110111010001111);

    sr = shift_register<16>(a.data() + 691, 15, 6, 16);
    auto r1281 = sr.shift();
    REQUIRE(r1281 == 0b1101110100011110);

    sr = shift_register<16>(a.data() + 691, 14, 7, 16);
    auto r1282 = sr.shift();
    REQUIRE(r1282 == 0b1011101000111100);

    sr = shift_register<16>(a.data() + 691, 13, 8, 16);
    auto r1283 = sr.shift();
    REQUIRE(r1283 == 0b0111010001111000);

    sr = shift_register<16>(a.data() + 691, 12, 9, 16);
    auto r1284 = sr.shift();
    REQUIRE(r1284 == 0b1110100011110000);

    sr = shift_register<16>(a.data() + 691, 11, 10, 16);
    auto r1285 = sr.shift();
    REQUIRE(r1285 == 0b1101000111100000);

    sr = shift_register<16>(a.data() + 691, 10, 11, 16);
    auto r1286 = sr.shift();
    REQUIRE(r1286 == 0b1010001111000001);

    sr = shift_register<16>(a.data() + 691, 9, 12, 16);
    auto r1287 = sr.shift();
    REQUIRE(r1287 == 0b0100011110000011);

    sr = shift_register<16>(a.data() + 691, 8, 13, 16);
    auto r1288 = sr.shift();
    REQUIRE(r1288 == 0b1000111100000110);

    sr = shift_register<16>(a.data() + 691, 7, 14, 16);
    auto r1289 = sr.shift();
    REQUIRE(r1289 == 0b0001111000001100);

    sr = shift_register<16>(a.data() + 698, 3, 0, 16);
    auto r1290 = sr.shift();
    REQUIRE(r1290 == 0b1000010100111101);

    sr = shift_register<16>(a.data() + 698, 2, 1, 16);
    auto r1291 = sr.shift();
    REQUIRE(r1291 == 0b0000101001111011);

    sr = shift_register<16>(a.data() + 698, 1, 2, 16);
    auto r1292 = sr.shift();
    REQUIRE(r1292 == 0b0001010011110111);

    sr = shift_register<16>(a.data() + 698, 0, 3, 16);
    auto r1293 = sr.shift();
    REQUIRE(r1293 == 0b0010100111101111);

    sr = shift_register<16>(a.data() + 699, 15, 4, 16);
    auto r1294 = sr.shift();
    REQUIRE(r1294 == 0b0101001111011110);

    sr = shift_register<16>(a.data() + 699, 14, 5, 16);
    auto r1295 = sr.shift();
    REQUIRE(r1295 == 0b1010011110111101);

    sr = shift_register<16>(a.data() + 699, 13, 6, 16);
    auto r1296 = sr.shift();
    REQUIRE(r1296 == 0b0100111101111010);

    sr = shift_register<16>(a.data() + 699, 12, 7, 16);
    auto r1297 = sr.shift();
    REQUIRE(r1297 == 0b1001111011110100);

    sr = shift_register<16>(a.data() + 699, 11, 8, 16);
    auto r1298 = sr.shift();
    REQUIRE(r1298 == 0b0011110111101001);

    sr = shift_register<16>(a.data() + 699, 10, 9, 16);
    auto r1299 = sr.shift();
    REQUIRE(r1299 == 0b0111101111010011);

    sr = shift_register<16>(a.data() + 699, 9, 10, 16);
    auto r1300 = sr.shift();
    REQUIRE(r1300 == 0b1111011110100110);

    sr = shift_register<16>(a.data() + 699, 8, 11, 16);
    auto r1301 = sr.shift();
    REQUIRE(r1301 == 0b1110111101001101);

    sr = shift_register<16>(a.data() + 699, 7, 12, 16);
    auto r1302 = sr.shift();
    REQUIRE(r1302 == 0b1101111010011011);

    sr = shift_register<16>(a.data() + 699, 6, 13, 16);
    auto r1303 = sr.shift();
    REQUIRE(r1303 == 0b1011110100110110);

    sr = shift_register<16>(a.data() + 699, 5, 14, 16);
    auto r1304 = sr.shift();
    REQUIRE(r1304 == 0b0111101001101101);

    sr = shift_register<16>(a.data() + 706, 1, 0, 16);
    auto r1305 = sr.shift();
    REQUIRE(r1305 == 0b1101001011100110);

    sr = shift_register<16>(a.data() + 706, 0, 1, 16);
    auto r1306 = sr.shift();
    REQUIRE(r1306 == 0b1010010111001101);

    sr = shift_register<16>(a.data() + 707, 15, 2, 16);
    auto r1307 = sr.shift();
    REQUIRE(r1307 == 0b0100101110011011);

    sr = shift_register<16>(a.data() + 707, 14, 3, 16);
    auto r1308 = sr.shift();
    REQUIRE(r1308 == 0b1001011100110110);

    sr = shift_register<16>(a.data() + 707, 13, 4, 16);
    auto r1309 = sr.shift();
    REQUIRE(r1309 == 0b0010111001101100);

    sr = shift_register<16>(a.data() + 707, 12, 5, 16);
    auto r1310 = sr.shift();
    REQUIRE(r1310 == 0b0101110011011001);

    sr = shift_register<16>(a.data() + 707, 11, 6, 16);
    auto r1311 = sr.shift();
    REQUIRE(r1311 == 0b1011100110110011);

    sr = shift_register<16>(a.data() + 707, 10, 7, 16);
    auto r1312 = sr.shift();
    REQUIRE(r1312 == 0b0111001101100111);

    sr = shift_register<16>(a.data() + 707, 9, 8, 16);
    auto r1313 = sr.shift();
    REQUIRE(r1313 == 0b1110011011001110);

    sr = shift_register<16>(a.data() + 707, 8, 9, 16);
    auto r1314 = sr.shift();
    REQUIRE(r1314 == 0b1100110110011101);

    sr = shift_register<16>(a.data() + 707, 7, 10, 16);
    auto r1315 = sr.shift();
    REQUIRE(r1315 == 0b1001101100111011);

    sr = shift_register<16>(a.data() + 707, 6, 11, 16);
    auto r1316 = sr.shift();
    REQUIRE(r1316 == 0b0011011001110111);

    sr = shift_register<16>(a.data() + 707, 5, 12, 16);
    auto r1317 = sr.shift();
    REQUIRE(r1317 == 0b0110110011101111);

    sr = shift_register<16>(a.data() + 707, 4, 13, 16);
    auto r1318 = sr.shift();
    REQUIRE(r1318 == 0b1101100111011111);

    sr = shift_register<16>(a.data() + 707, 3, 14, 16);
    auto r1319 = sr.shift();
    REQUIRE(r1319 == 0b1011001110111111);

    sr = shift_register<16>(a.data() + 715, 15, 0, 16);
    auto r1320 = sr.shift();
    REQUIRE(r1320 == 0b1100011000101010);

    sr = shift_register<16>(a.data() + 715, 14, 1, 16);
    auto r1321 = sr.shift();
    REQUIRE(r1321 == 0b1000110001010101);

    sr = shift_register<16>(a.data() + 715, 13, 2, 16);
    auto r1322 = sr.shift();
    REQUIRE(r1322 == 0b0001100010101011);

    sr = shift_register<16>(a.data() + 715, 12, 3, 16);
    auto r1323 = sr.shift();
    REQUIRE(r1323 == 0b0011000101010111);

    sr = shift_register<16>(a.data() + 715, 11, 4, 16);
    auto r1324 = sr.shift();
    REQUIRE(r1324 == 0b0110001010101110);

    sr = shift_register<16>(a.data() + 715, 10, 5, 16);
    auto r1325 = sr.shift();
    REQUIRE(r1325 == 0b1100010101011101);

    sr = shift_register<16>(a.data() + 715, 9, 6, 16);
    auto r1326 = sr.shift();
    REQUIRE(r1326 == 0b1000101010111010);

    sr = shift_register<16>(a.data() + 715, 8, 7, 16);
    auto r1327 = sr.shift();
    REQUIRE(r1327 == 0b0001010101110101);

    sr = shift_register<16>(a.data() + 715, 7, 8, 16);
    auto r1328 = sr.shift();
    REQUIRE(r1328 == 0b0010101011101010);

    sr = shift_register<16>(a.data() + 715, 6, 9, 16);
    auto r1329 = sr.shift();
    REQUIRE(r1329 == 0b0101010111010101);

    sr = shift_register<16>(a.data() + 715, 5, 10, 16);
    auto r1330 = sr.shift();
    REQUIRE(r1330 == 0b1010101110101010);

    sr = shift_register<16>(a.data() + 715, 4, 11, 16);
    auto r1331 = sr.shift();
    REQUIRE(r1331 == 0b0101011101010100);

    sr = shift_register<16>(a.data() + 715, 3, 12, 16);
    auto r1332 = sr.shift();
    REQUIRE(r1332 == 0b1010111010101001);

    sr = shift_register<16>(a.data() + 715, 2, 13, 16);
    auto r1333 = sr.shift();
    REQUIRE(r1333 == 0b0101110101010010);

    sr = shift_register<16>(a.data() + 715, 1, 14, 16);
    auto r1334 = sr.shift();
    REQUIRE(r1334 == 0b1011101010100101);

    sr = shift_register<16>(a.data() + 723, 13, 0, 16);
    auto r1335 = sr.shift();
    REQUIRE(r1335 == 0b0001110001001100);

    sr = shift_register<16>(a.data() + 723, 12, 1, 16);
    auto r1336 = sr.shift();
    REQUIRE(r1336 == 0b0011100010011000);

    sr = shift_register<16>(a.data() + 723, 11, 2, 16);
    auto r1337 = sr.shift();
    REQUIRE(r1337 == 0b0111000100110000);

    sr = shift_register<16>(a.data() + 723, 10, 3, 16);
    auto r1338 = sr.shift();
    REQUIRE(r1338 == 0b1110001001100000);

    sr = shift_register<16>(a.data() + 723, 9, 4, 16);
    auto r1339 = sr.shift();
    REQUIRE(r1339 == 0b1100010011000001);

    sr = shift_register<16>(a.data() + 723, 8, 5, 16);
    auto r1340 = sr.shift();
    REQUIRE(r1340 == 0b1000100110000010);

    sr = shift_register<16>(a.data() + 723, 7, 6, 16);
    auto r1341 = sr.shift();
    REQUIRE(r1341 == 0b0001001100000101);

    sr = shift_register<16>(a.data() + 723, 6, 7, 16);
    auto r1342 = sr.shift();
    REQUIRE(r1342 == 0b0010011000001010);

    sr = shift_register<16>(a.data() + 723, 5, 8, 16);
    auto r1343 = sr.shift();
    REQUIRE(r1343 == 0b0100110000010101);

    sr = shift_register<16>(a.data() + 723, 4, 9, 16);
    auto r1344 = sr.shift();
    REQUIRE(r1344 == 0b1001100000101011);

    sr = shift_register<16>(a.data() + 723, 3, 10, 16);
    auto r1345 = sr.shift();
    REQUIRE(r1345 == 0b0011000001010111);

    sr = shift_register<16>(a.data() + 723, 2, 11, 16);
    auto r1346 = sr.shift();
    REQUIRE(r1346 == 0b0110000010101110);

    sr = shift_register<16>(a.data() + 723, 1, 12, 16);
    auto r1347 = sr.shift();
    REQUIRE(r1347 == 0b1100000101011100);

    sr = shift_register<16>(a.data() + 723, 0, 13, 16);
    auto r1348 = sr.shift();
    REQUIRE(r1348 == 0b1000001010111000);

    sr = shift_register<16>(a.data() + 724, 15, 14, 16);
    auto r1349 = sr.shift();
    REQUIRE(r1349 == 0b0000010101110001);

    sr = shift_register<16>(a.data() + 731, 11, 0, 16);
    auto r1350 = sr.shift();
    REQUIRE(r1350 == 0b0010110000110110);

    sr = shift_register<16>(a.data() + 731, 10, 1, 16);
    auto r1351 = sr.shift();
    REQUIRE(r1351 == 0b0101100001101101);

    sr = shift_register<16>(a.data() + 731, 9, 2, 16);
    auto r1352 = sr.shift();
    REQUIRE(r1352 == 0b1011000011011010);

    sr = shift_register<16>(a.data() + 731, 8, 3, 16);
    auto r1353 = sr.shift();
    REQUIRE(r1353 == 0b0110000110110101);

    sr = shift_register<16>(a.data() + 731, 7, 4, 16);
    auto r1354 = sr.shift();
    REQUIRE(r1354 == 0b1100001101101010);

    sr = shift_register<16>(a.data() + 731, 6, 5, 16);
    auto r1355 = sr.shift();
    REQUIRE(r1355 == 0b1000011011010100);

    sr = shift_register<16>(a.data() + 731, 5, 6, 16);
    auto r1356 = sr.shift();
    REQUIRE(r1356 == 0b0000110110101001);

    sr = shift_register<16>(a.data() + 731, 4, 7, 16);
    auto r1357 = sr.shift();
    REQUIRE(r1357 == 0b0001101101010011);

    sr = shift_register<16>(a.data() + 731, 3, 8, 16);
    auto r1358 = sr.shift();
    REQUIRE(r1358 == 0b0011011010100110);

    sr = shift_register<16>(a.data() + 731, 2, 9, 16);
    auto r1359 = sr.shift();
    REQUIRE(r1359 == 0b0110110101001100);

    sr = shift_register<16>(a.data() + 731, 1, 10, 16);
    auto r1360 = sr.shift();
    REQUIRE(r1360 == 0b1101101010011001);

    sr = shift_register<16>(a.data() + 731, 0, 11, 16);
    auto r1361 = sr.shift();
    REQUIRE(r1361 == 0b1011010100110010);

    sr = shift_register<16>(a.data() + 732, 15, 12, 16);
    auto r1362 = sr.shift();
    REQUIRE(r1362 == 0b0110101001100100);

    sr = shift_register<16>(a.data() + 732, 14, 13, 16);
    auto r1363 = sr.shift();
    REQUIRE(r1363 == 0b1101010011001001);

    sr = shift_register<16>(a.data() + 732, 13, 14, 16);
    auto r1364 = sr.shift();
    REQUIRE(r1364 == 0b1010100110010010);

    sr = shift_register<16>(a.data() + 739, 9, 0, 16);
    auto r1365 = sr.shift();
    REQUIRE(r1365 == 0b0011001000111110);

    sr = shift_register<16>(a.data() + 739, 8, 1, 16);
    auto r1366 = sr.shift();
    REQUIRE(r1366 == 0b0110010001111101);

    sr = shift_register<16>(a.data() + 739, 7, 2, 16);
    auto r1367 = sr.shift();
    REQUIRE(r1367 == 0b1100100011111011);

    sr = shift_register<16>(a.data() + 739, 6, 3, 16);
    auto r1368 = sr.shift();
    REQUIRE(r1368 == 0b1001000111110111);

    sr = shift_register<16>(a.data() + 739, 5, 4, 16);
    auto r1369 = sr.shift();
    REQUIRE(r1369 == 0b0010001111101110);

    sr = shift_register<16>(a.data() + 739, 4, 5, 16);
    auto r1370 = sr.shift();
    REQUIRE(r1370 == 0b0100011111011100);

    sr = shift_register<16>(a.data() + 739, 3, 6, 16);
    auto r1371 = sr.shift();
    REQUIRE(r1371 == 0b1000111110111001);

    sr = shift_register<16>(a.data() + 739, 2, 7, 16);
    auto r1372 = sr.shift();
    REQUIRE(r1372 == 0b0001111101110011);

    sr = shift_register<16>(a.data() + 739, 1, 8, 16);
    auto r1373 = sr.shift();
    REQUIRE(r1373 == 0b0011111011100111);

    sr = shift_register<16>(a.data() + 739, 0, 9, 16);
    auto r1374 = sr.shift();
    REQUIRE(r1374 == 0b0111110111001110);

    sr = shift_register<16>(a.data() + 740, 15, 10, 16);
    auto r1375 = sr.shift();
    REQUIRE(r1375 == 0b1111101110011100);

    sr = shift_register<16>(a.data() + 740, 14, 11, 16);
    auto r1376 = sr.shift();
    REQUIRE(r1376 == 0b1111011100111000);

    sr = shift_register<16>(a.data() + 740, 13, 12, 16);
    auto r1377 = sr.shift();
    REQUIRE(r1377 == 0b1110111001110001);

    sr = shift_register<16>(a.data() + 740, 12, 13, 16);
    auto r1378 = sr.shift();
    REQUIRE(r1378 == 0b1101110011100010);

    sr = shift_register<16>(a.data() + 740, 11, 14, 16);
    auto r1379 = sr.shift();
    REQUIRE(r1379 == 0b1011100111000101);

    sr = shift_register<16>(a.data() + 747, 7, 0, 16);
    auto r1380 = sr.shift();
    REQUIRE(r1380 == 0b0101011001110111);

    sr = shift_register<16>(a.data() + 747, 6, 1, 16);
    auto r1381 = sr.shift();
    REQUIRE(r1381 == 0b1010110011101111);

    sr = shift_register<16>(a.data() + 747, 5, 2, 16);
    auto r1382 = sr.shift();
    REQUIRE(r1382 == 0b0101100111011110);

    sr = shift_register<16>(a.data() + 747, 4, 3, 16);
    auto r1383 = sr.shift();
    REQUIRE(r1383 == 0b1011001110111101);

    sr = shift_register<16>(a.data() + 747, 3, 4, 16);
    auto r1384 = sr.shift();
    REQUIRE(r1384 == 0b0110011101111011);

    sr = shift_register<16>(a.data() + 747, 2, 5, 16);
    auto r1385 = sr.shift();
    REQUIRE(r1385 == 0b1100111011110111);

    sr = shift_register<16>(a.data() + 747, 1, 6, 16);
    auto r1386 = sr.shift();
    REQUIRE(r1386 == 0b1001110111101111);

    sr = shift_register<16>(a.data() + 747, 0, 7, 16);
    auto r1387 = sr.shift();
    REQUIRE(r1387 == 0b0011101111011111);

    sr = shift_register<16>(a.data() + 748, 15, 8, 16);
    auto r1388 = sr.shift();
    REQUIRE(r1388 == 0b0111011110111111);

    sr = shift_register<16>(a.data() + 748, 14, 9, 16);
    auto r1389 = sr.shift();
    REQUIRE(r1389 == 0b1110111101111111);

    sr = shift_register<16>(a.data() + 748, 13, 10, 16);
    auto r1390 = sr.shift();
    REQUIRE(r1390 == 0b1101111011111111);

    sr = shift_register<16>(a.data() + 748, 12, 11, 16);
    auto r1391 = sr.shift();
    REQUIRE(r1391 == 0b1011110111111110);

    sr = shift_register<16>(a.data() + 748, 11, 12, 16);
    auto r1392 = sr.shift();
    REQUIRE(r1392 == 0b0111101111111101);

    sr = shift_register<16>(a.data() + 748, 10, 13, 16);
    auto r1393 = sr.shift();
    REQUIRE(r1393 == 0b1111011111111011);

    sr = shift_register<16>(a.data() + 748, 9, 14, 16);
    auto r1394 = sr.shift();
    REQUIRE(r1394 == 0b1110111111110111);

    sr = shift_register<16>(a.data() + 755, 5, 0, 16);
    auto r1395 = sr.shift();
    REQUIRE(r1395 == 0b0100010101111001);

    sr = shift_register<16>(a.data() + 755, 4, 1, 16);
    auto r1396 = sr.shift();
    REQUIRE(r1396 == 0b1000101011110011);

    sr = shift_register<16>(a.data() + 755, 3, 2, 16);
    auto r1397 = sr.shift();
    REQUIRE(r1397 == 0b0001010111100110);

    sr = shift_register<16>(a.data() + 755, 2, 3, 16);
    auto r1398 = sr.shift();
    REQUIRE(r1398 == 0b0010101111001100);

    sr = shift_register<16>(a.data() + 755, 1, 4, 16);
    auto r1399 = sr.shift();
    REQUIRE(r1399 == 0b0101011110011001);

    sr = shift_register<16>(a.data() + 755, 0, 5, 16);
    auto r1400 = sr.shift();
    REQUIRE(r1400 == 0b1010111100110011);

    sr = shift_register<16>(a.data() + 756, 15, 6, 16);
    auto r1401 = sr.shift();
    REQUIRE(r1401 == 0b0101111001100111);

    sr = shift_register<16>(a.data() + 756, 14, 7, 16);
    auto r1402 = sr.shift();
    REQUIRE(r1402 == 0b1011110011001110);

    sr = shift_register<16>(a.data() + 756, 13, 8, 16);
    auto r1403 = sr.shift();
    REQUIRE(r1403 == 0b0111100110011101);

    sr = shift_register<16>(a.data() + 756, 12, 9, 16);
    auto r1404 = sr.shift();
    REQUIRE(r1404 == 0b1111001100111011);

    sr = shift_register<16>(a.data() + 756, 11, 10, 16);
    auto r1405 = sr.shift();
    REQUIRE(r1405 == 0b1110011001110111);

    sr = shift_register<16>(a.data() + 756, 10, 11, 16);
    auto r1406 = sr.shift();
    REQUIRE(r1406 == 0b1100110011101111);

    sr = shift_register<16>(a.data() + 756, 9, 12, 16);
    auto r1407 = sr.shift();
    REQUIRE(r1407 == 0b1001100111011110);

    sr = shift_register<16>(a.data() + 756, 8, 13, 16);
    auto r1408 = sr.shift();
    REQUIRE(r1408 == 0b0011001110111100);

    sr = shift_register<16>(a.data() + 756, 7, 14, 16);
    auto r1409 = sr.shift();
    REQUIRE(r1409 == 0b0110011101111001);

    sr = shift_register<16>(a.data() + 763, 3, 0, 16);
    auto r1410 = sr.shift();
    REQUIRE(r1410 == 0b1001000110101011);

    sr = shift_register<16>(a.data() + 763, 2, 1, 16);
    auto r1411 = sr.shift();
    REQUIRE(r1411 == 0b0010001101010110);

    sr = shift_register<16>(a.data() + 763, 1, 2, 16);
    auto r1412 = sr.shift();
    REQUIRE(r1412 == 0b0100011010101100);

    sr = shift_register<16>(a.data() + 763, 0, 3, 16);
    auto r1413 = sr.shift();
    REQUIRE(r1413 == 0b1000110101011000);

    sr = shift_register<16>(a.data() + 764, 15, 4, 16);
    auto r1414 = sr.shift();
    REQUIRE(r1414 == 0b0001101010110001);

    sr = shift_register<16>(a.data() + 764, 14, 5, 16);
    auto r1415 = sr.shift();
    REQUIRE(r1415 == 0b0011010101100011);

    sr = shift_register<16>(a.data() + 764, 13, 6, 16);
    auto r1416 = sr.shift();
    REQUIRE(r1416 == 0b0110101011000110);

    sr = shift_register<16>(a.data() + 764, 12, 7, 16);
    auto r1417 = sr.shift();
    REQUIRE(r1417 == 0b1101010110001100);

    sr = shift_register<16>(a.data() + 764, 11, 8, 16);
    auto r1418 = sr.shift();
    REQUIRE(r1418 == 0b1010101100011001);

    sr = shift_register<16>(a.data() + 764, 10, 9, 16);
    auto r1419 = sr.shift();
    REQUIRE(r1419 == 0b0101011000110011);

    sr = shift_register<16>(a.data() + 764, 9, 10, 16);
    auto r1420 = sr.shift();
    REQUIRE(r1420 == 0b1010110001100110);

    sr = shift_register<16>(a.data() + 764, 8, 11, 16);
    auto r1421 = sr.shift();
    REQUIRE(r1421 == 0b0101100011001100);

    sr = shift_register<16>(a.data() + 764, 7, 12, 16);
    auto r1422 = sr.shift();
    REQUIRE(r1422 == 0b1011000110011001);

    sr = shift_register<16>(a.data() + 764, 6, 13, 16);
    auto r1423 = sr.shift();
    REQUIRE(r1423 == 0b0110001100110011);

    sr = shift_register<16>(a.data() + 764, 5, 14, 16);
    auto r1424 = sr.shift();
    REQUIRE(r1424 == 0b1100011001100110);

    sr = shift_register<16>(a.data() + 771, 1, 0, 16);
    auto r1425 = sr.shift();
    REQUIRE(r1425 == 0b1101000011111111);

    sr = shift_register<16>(a.data() + 771, 0, 1, 16);
    auto r1426 = sr.shift();
    REQUIRE(r1426 == 0b1010000111111110);

    sr = shift_register<16>(a.data() + 772, 15, 2, 16);
    auto r1427 = sr.shift();
    REQUIRE(r1427 == 0b0100001111111100);

    sr = shift_register<16>(a.data() + 772, 14, 3, 16);
    auto r1428 = sr.shift();
    REQUIRE(r1428 == 0b1000011111111001);

    sr = shift_register<16>(a.data() + 772, 13, 4, 16);
    auto r1429 = sr.shift();
    REQUIRE(r1429 == 0b0000111111110011);

    sr = shift_register<16>(a.data() + 772, 12, 5, 16);
    auto r1430 = sr.shift();
    REQUIRE(r1430 == 0b0001111111100110);

    sr = shift_register<16>(a.data() + 772, 11, 6, 16);
    auto r1431 = sr.shift();
    REQUIRE(r1431 == 0b0011111111001100);

    sr = shift_register<16>(a.data() + 772, 10, 7, 16);
    auto r1432 = sr.shift();
    REQUIRE(r1432 == 0b0111111110011001);

    sr = shift_register<16>(a.data() + 772, 9, 8, 16);
    auto r1433 = sr.shift();
    REQUIRE(r1433 == 0b1111111100110011);

    sr = shift_register<16>(a.data() + 772, 8, 9, 16);
    auto r1434 = sr.shift();
    REQUIRE(r1434 == 0b1111111001100110);

    sr = shift_register<16>(a.data() + 772, 7, 10, 16);
    auto r1435 = sr.shift();
    REQUIRE(r1435 == 0b1111110011001100);

    sr = shift_register<16>(a.data() + 772, 6, 11, 16);
    auto r1436 = sr.shift();
    REQUIRE(r1436 == 0b1111100110011001);

    sr = shift_register<16>(a.data() + 772, 5, 12, 16);
    auto r1437 = sr.shift();
    REQUIRE(r1437 == 0b1111001100110010);

    sr = shift_register<16>(a.data() + 772, 4, 13, 16);
    auto r1438 = sr.shift();
    REQUIRE(r1438 == 0b1110011001100100);

    sr = shift_register<16>(a.data() + 772, 3, 14, 16);
    auto r1439 = sr.shift();
    REQUIRE(r1439 == 0b1100110011001001);

    sr = shift_register<16>(a.data() + 780, 15, 0, 16);
    auto r1440 = sr.shift();
    REQUIRE(r1440 == 0b1011101001001101);

    sr = shift_register<16>(a.data() + 780, 14, 1, 16);
    auto r1441 = sr.shift();
    REQUIRE(r1441 == 0b0111010010011011);

    sr = shift_register<16>(a.data() + 780, 13, 2, 16);
    auto r1442 = sr.shift();
    REQUIRE(r1442 == 0b1110100100110110);

    sr = shift_register<16>(a.data() + 780, 12, 3, 16);
    auto r1443 = sr.shift();
    REQUIRE(r1443 == 0b1101001001101100);

    sr = shift_register<16>(a.data() + 780, 11, 4, 16);
    auto r1444 = sr.shift();
    REQUIRE(r1444 == 0b1010010011011001);

    sr = shift_register<16>(a.data() + 780, 10, 5, 16);
    auto r1445 = sr.shift();
    REQUIRE(r1445 == 0b0100100110110011);

    sr = shift_register<16>(a.data() + 780, 9, 6, 16);
    auto r1446 = sr.shift();
    REQUIRE(r1446 == 0b1001001101100110);

    sr = shift_register<16>(a.data() + 780, 8, 7, 16);
    auto r1447 = sr.shift();
    REQUIRE(r1447 == 0b0010011011001100);

    sr = shift_register<16>(a.data() + 780, 7, 8, 16);
    auto r1448 = sr.shift();
    REQUIRE(r1448 == 0b0100110110011000);

    sr = shift_register<16>(a.data() + 780, 6, 9, 16);
    auto r1449 = sr.shift();
    REQUIRE(r1449 == 0b1001101100110000);

    sr = shift_register<16>(a.data() + 780, 5, 10, 16);
    auto r1450 = sr.shift();
    REQUIRE(r1450 == 0b0011011001100001);

    sr = shift_register<16>(a.data() + 780, 4, 11, 16);
    auto r1451 = sr.shift();
    REQUIRE(r1451 == 0b0110110011000011);

    sr = shift_register<16>(a.data() + 780, 3, 12, 16);
    auto r1452 = sr.shift();
    REQUIRE(r1452 == 0b1101100110000111);

    sr = shift_register<16>(a.data() + 780, 2, 13, 16);
    auto r1453 = sr.shift();
    REQUIRE(r1453 == 0b1011001100001111);

    sr = shift_register<16>(a.data() + 780, 1, 14, 16);
    auto r1454 = sr.shift();
    REQUIRE(r1454 == 0b0110011000011111);

    sr = shift_register<16>(a.data() + 788, 13, 0, 16);
    auto r1455 = sr.shift();
    REQUIRE(r1455 == 0b0011000001000100);

    sr = shift_register<16>(a.data() + 788, 12, 1, 16);
    auto r1456 = sr.shift();
    REQUIRE(r1456 == 0b0110000010001000);

    sr = shift_register<16>(a.data() + 788, 11, 2, 16);
    auto r1457 = sr.shift();
    REQUIRE(r1457 == 0b1100000100010000);

    sr = shift_register<16>(a.data() + 788, 10, 3, 16);
    auto r1458 = sr.shift();
    REQUIRE(r1458 == 0b1000001000100001);

    sr = shift_register<16>(a.data() + 788, 9, 4, 16);
    auto r1459 = sr.shift();
    REQUIRE(r1459 == 0b0000010001000010);

    sr = shift_register<16>(a.data() + 788, 8, 5, 16);
    auto r1460 = sr.shift();
    REQUIRE(r1460 == 0b0000100010000101);

    sr = shift_register<16>(a.data() + 788, 7, 6, 16);
    auto r1461 = sr.shift();
    REQUIRE(r1461 == 0b0001000100001010);

    sr = shift_register<16>(a.data() + 788, 6, 7, 16);
    auto r1462 = sr.shift();
    REQUIRE(r1462 == 0b0010001000010101);

    sr = shift_register<16>(a.data() + 788, 5, 8, 16);
    auto r1463 = sr.shift();
    REQUIRE(r1463 == 0b0100010000101010);

    sr = shift_register<16>(a.data() + 788, 4, 9, 16);
    auto r1464 = sr.shift();
    REQUIRE(r1464 == 0b1000100001010101);

    sr = shift_register<16>(a.data() + 788, 3, 10, 16);
    auto r1465 = sr.shift();
    REQUIRE(r1465 == 0b0001000010101011);

    sr = shift_register<16>(a.data() + 788, 2, 11, 16);
    auto r1466 = sr.shift();
    REQUIRE(r1466 == 0b0010000101010111);

    sr = shift_register<16>(a.data() + 788, 1, 12, 16);
    auto r1467 = sr.shift();
    REQUIRE(r1467 == 0b0100001010101110);

    sr = shift_register<16>(a.data() + 788, 0, 13, 16);
    auto r1468 = sr.shift();
    REQUIRE(r1468 == 0b1000010101011101);

    sr = shift_register<16>(a.data() + 789, 15, 14, 16);
    auto r1469 = sr.shift();
    REQUIRE(r1469 == 0b0000101010111011);

    sr = shift_register<16>(a.data() + 796, 11, 0, 16);
    auto r1470 = sr.shift();
    REQUIRE(r1470 == 0b0000001110101101);

    sr = shift_register<16>(a.data() + 796, 10, 1, 16);
    auto r1471 = sr.shift();
    REQUIRE(r1471 == 0b0000011101011011);

    sr = shift_register<16>(a.data() + 796, 9, 2, 16);
    auto r1472 = sr.shift();
    REQUIRE(r1472 == 0b0000111010110111);

    sr = shift_register<16>(a.data() + 796, 8, 3, 16);
    auto r1473 = sr.shift();
    REQUIRE(r1473 == 0b0001110101101110);

    sr = shift_register<16>(a.data() + 796, 7, 4, 16);
    auto r1474 = sr.shift();
    REQUIRE(r1474 == 0b0011101011011101);

    sr = shift_register<16>(a.data() + 796, 6, 5, 16);
    auto r1475 = sr.shift();
    REQUIRE(r1475 == 0b0111010110111010);

    sr = shift_register<16>(a.data() + 796, 5, 6, 16);
    auto r1476 = sr.shift();
    REQUIRE(r1476 == 0b1110101101110100);

    sr = shift_register<16>(a.data() + 796, 4, 7, 16);
    auto r1477 = sr.shift();
    REQUIRE(r1477 == 0b1101011011101000);

    sr = shift_register<16>(a.data() + 796, 3, 8, 16);
    auto r1478 = sr.shift();
    REQUIRE(r1478 == 0b1010110111010001);

    sr = shift_register<16>(a.data() + 796, 2, 9, 16);
    auto r1479 = sr.shift();
    REQUIRE(r1479 == 0b0101101110100011);

    sr = shift_register<16>(a.data() + 796, 1, 10, 16);
    auto r1480 = sr.shift();
    REQUIRE(r1480 == 0b1011011101000111);

    sr = shift_register<16>(a.data() + 796, 0, 11, 16);
    auto r1481 = sr.shift();
    REQUIRE(r1481 == 0b0110111010001111);

    sr = shift_register<16>(a.data() + 797, 15, 12, 16);
    auto r1482 = sr.shift();
    REQUIRE(r1482 == 0b1101110100011111);

    sr = shift_register<16>(a.data() + 797, 14, 13, 16);
    auto r1483 = sr.shift();
    REQUIRE(r1483 == 0b1011101000111111);

    sr = shift_register<16>(a.data() + 797, 13, 14, 16);
    auto r1484 = sr.shift();
    REQUIRE(r1484 == 0b0111010001111110);

    sr = shift_register<16>(a.data() + 804, 9, 0, 16);
    auto r1485 = sr.shift();
    REQUIRE(r1485 == 0b1000110001111100);

    sr = shift_register<16>(a.data() + 804, 8, 1, 16);
    auto r1486 = sr.shift();
    REQUIRE(r1486 == 0b0001100011111000);

    sr = shift_register<16>(a.data() + 804, 7, 2, 16);
    auto r1487 = sr.shift();
    REQUIRE(r1487 == 0b0011000111110000);

    sr = shift_register<16>(a.data() + 804, 6, 3, 16);
    auto r1488 = sr.shift();
    REQUIRE(r1488 == 0b0110001111100000);

    sr = shift_register<16>(a.data() + 804, 5, 4, 16);
    auto r1489 = sr.shift();
    REQUIRE(r1489 == 0b1100011111000001);

    sr = shift_register<16>(a.data() + 804, 4, 5, 16);
    auto r1490 = sr.shift();
    REQUIRE(r1490 == 0b1000111110000010);

    sr = shift_register<16>(a.data() + 804, 3, 6, 16);
    auto r1491 = sr.shift();
    REQUIRE(r1491 == 0b0001111100000101);

    sr = shift_register<16>(a.data() + 804, 2, 7, 16);
    auto r1492 = sr.shift();
    REQUIRE(r1492 == 0b0011111000001011);

    sr = shift_register<16>(a.data() + 804, 1, 8, 16);
    auto r1493 = sr.shift();
    REQUIRE(r1493 == 0b0111110000010111);

    sr = shift_register<16>(a.data() + 804, 0, 9, 16);
    auto r1494 = sr.shift();
    REQUIRE(r1494 == 0b1111100000101111);

    sr = shift_register<16>(a.data() + 805, 15, 10, 16);
    auto r1495 = sr.shift();
    REQUIRE(r1495 == 0b1111000001011110);

    sr = shift_register<16>(a.data() + 805, 14, 11, 16);
    auto r1496 = sr.shift();
    REQUIRE(r1496 == 0b1110000010111100);

    sr = shift_register<16>(a.data() + 805, 13, 12, 16);
    auto r1497 = sr.shift();
    REQUIRE(r1497 == 0b1100000101111001);

    sr = shift_register<16>(a.data() + 805, 12, 13, 16);
    auto r1498 = sr.shift();
    REQUIRE(r1498 == 0b1000001011110011);

    sr = shift_register<16>(a.data() + 805, 11, 14, 16);
    auto r1499 = sr.shift();
    REQUIRE(r1499 == 0b0000010111100110);



  }
  TEST_CASE_FIXTURE(test_fixture_8bit, "sr #2") {

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
