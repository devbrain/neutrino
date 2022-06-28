//
// Created by igor on 27/06/2022.
//

#ifndef APPS_FON_TRAITS_HH
#define APPS_FON_TRAITS_HH

#include <array>
#include <string>

namespace detail {
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

    template <class T>
    static T reverse_bits (T n) {
      short bits = bits_in_word;
      T m = ~T (0); // equivalent to uint32_t mask = 0b11111111111111111111111111111111;

      while (bits >>= 1) {
        m ^= m << (bits); // will convert mask to 0b00000000000000001111111111111111;
        n = (n & ~m) >> bits | (n & m) << bits; // divide and conquer
      }

      return n;
    }

  };
} // ns detail

template <int BitsInWord>
void print_bits (typename detail::bitmask_traits<BitsInWord>::word_t w) {
  for (int i = detail::bitmask_traits<BitsInWord>::bits_in_word - 1; i >= 0; i--) {

    if (w & (detail::bitmask_traits<BitsInWord>::unit << i)) {
      std::cout << 1;
    }
    else {
      std::cout << 0;
    }
  }
  std::cout << std::endl;
}

template <typename T>
std::string binary (T n) {
  auto bits = (short) (sizeof (n) * 8);
  std::string s;
  for (short k = (bits - 1); k >= 0; k--) {
    s += (n & (1 << k)) ? "1" : "0";
  }
  return s;
}

#endif //APPS_FON_TRAITS_HH
