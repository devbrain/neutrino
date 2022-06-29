//
// Created by igor on 29/06/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_IMAGE_BITMASK_BITMASK_TRAITS_HH
#define INCLUDE_NEUTRINO_ASSETS_IMAGE_BITMASK_BITMASK_TRAITS_HH

#include <array>
#include <cstdint>

namespace neutrino::assets::detail {
  template <int BitsInWord>
  struct bitmask_traits;


  template <>
  struct bitmask_traits<32> {
    using word_t = uint32_t;
    static constexpr auto bits_in_word = 32;
    static constexpr auto exp = 5;
    static constexpr word_t unit = 1;
    static constexpr word_t zero = 0;
    static constexpr std::array<word_t, 33> mask = {
        0b00000000000000000000000000000000,
        0b10000000000000000000000000000000,
        0b11000000000000000000000000000000,
        0b11100000000000000000000000000000,
        0b11110000000000000000000000000000,
        0b11111000000000000000000000000000,
        0b11111100000000000000000000000000,
        0b11111110000000000000000000000000,
        0b11111111000000000000000000000000,
        0b11111111100000000000000000000000,
        0b11111111110000000000000000000000,
        0b11111111111000000000000000000000,
        0b11111111111100000000000000000000,
        0b11111111111110000000000000000000,
        0b11111111111111000000000000000000,
        0b11111111111111100000000000000000,
        0b11111111111111110000000000000000,
        0b11111111111111111000000000000000,
        0b11111111111111111100000000000000,
        0b11111111111111111110000000000000,
        0b11111111111111111111000000000000,
        0b11111111111111111111100000000000,
        0b11111111111111111111110000000000,
        0b11111111111111111111111000000000,
        0b11111111111111111111111100000000,
        0b11111111111111111111111110000000,
        0b11111111111111111111111111000000,
        0b11111111111111111111111111100000,
        0b11111111111111111111111111110000,
        0b11111111111111111111111111111000,
        0b11111111111111111111111111111100,
        0b11111111111111111111111111111110,
        0b11111111111111111111111111111111
    };
  };

  template <>
  struct bitmask_traits<16> {
    using word_t = uint16_t;
    static constexpr auto bits_in_word = 16;
    static constexpr auto exp = 4;
    static constexpr word_t unit = 1;
    static constexpr word_t zero = 0;
    static constexpr std::array<word_t, 17> mask = {
        0b0000000000000000,
        0b1000000000000000,
        0b1100000000000000,
        0b1110000000000000,
        0b1111000000000000,
        0b1111100000000000,
        0b1111110000000000,
        0b1111111000000000,
        0b1111111100000000,
        0b1111111110000000,
        0b1111111111000000,
        0b1111111111100000,
        0b1111111111110000,
        0b1111111111111000,
        0b1111111111111100,
        0b1111111111111110,
        0b1111111111111111
    };

  };

  template <>
  struct bitmask_traits<8> {
    using word_t = uint8_t;
    static constexpr auto bits_in_word = 8;
    static constexpr auto exp = 3;
    static constexpr word_t unit = 1;
    static constexpr word_t zero = 0;

    static constexpr std::array<word_t, 9> mask = {
        0b00000000, //0
        0b10000000, //1
        0b11000000, //2
        0b11100000, //3
        0b11110000, //4
        0b11111000, //5
        0b11111100, //6
        0b11111110, //7
        0b11111111  //8
    };
  };
}

#endif //INCLUDE_NEUTRINO_ASSETS_IMAGE_BITMASK_BITMASK_TRAITS_HH
