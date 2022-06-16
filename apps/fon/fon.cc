//
// Created by igor on 02/05/2022.
//

#include <iostream>
#include <cstdint>
#include <tuple>
#include <array>
#include <vector>
#include <random>

#include <neutrino/utils/builtin.h>
#include <neutrino/utils/exception.hh>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <neutrino/hal/video/fonts/rom_fonts.hh>

namespace detail {
  template <int BitsInWord>
  struct bitmask_traits;

  template <>
  struct bitmask_traits<64> {
    using word_t = uint64_t;
    static constexpr auto bits_in_word = 64;
    static constexpr auto exp = 6;
    static constexpr word_t unit = 1;
  };

  template <>
  struct bitmask_traits<32> {
    using word_t = uint32_t;
    static constexpr auto bits_in_word = 32;
    static constexpr auto exp = 5;
    static constexpr word_t unit = 1;

  };

  template <>
  struct bitmask_traits<16> {
    using word_t = uint16_t;
    static constexpr auto bits_in_word = 16;
    static constexpr auto exp = 4;
    static constexpr word_t unit = 1;
  };

  template <>
  struct bitmask_traits<8> {
    using word_t = uint8_t;
    static constexpr auto bits_in_word = 8;
    static constexpr auto exp = 3;
    static constexpr word_t unit = 1;
    static constexpr std::array<word_t, 8> mask = {
        0b11111111,
        0b11111110,
        0b11111100,
        0b11111000,
        0b11110000,
        0b11100000,
        0b11000000,
        0b10000000,
    };

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

template <int BitsInWord> class bitmask;

template <int BitsInWord>
[[nodiscard]] int overlap_area (const bitmask<BitsInWord>& a, const bitmask<BitsInWord>& b, int xoffset, int yoffset);

template <int BitsInWord>
class bitmask {
    template <int N>
    friend int overlap_area (const bitmask<N>& a, const bitmask<N>& b, int xoffset, int yoffset);
  public:
    using bitmask_traits_t = detail::bitmask_traits<BitsInWord>;
    using word_t = typename bitmask_traits_t::word_t;
  public:

    static bitmask<BitsInWord> create_random (int w, int h) {
      bitmask<BitsInWord> bm (w, h);
      std::random_device random_device;
      std::mt19937 random_engine (random_device ());
      std::uniform_int_distribution<int> d (1, 100);
      for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
          if (d (random_engine) < 50) {
            bm.set (x, y);
          }
        }
      }
      return bm;
    }

    void debug (const char* name) {
      std::cout << "bitmask<" << BitsInWord << "> " << name << "(" << m_w << ", " << m_h << ");" << std::endl;
      for (int y = 0; y < m_h; y++) {
        for (int x = 0; x < m_w; x++) {
          if (get (x, y)) {
            std::cout << name << ".set(" << x << "," << y << ");" << std::endl;
          }
        }
      }
    }

    const word_t* data () const {
      return m_data.data ();
    }

    void print_words () {
      int w = 0;
      int tx = 0;
      for (const auto& x: m_data) {
        std::cout << " w(" << w << ") ";
        w++;
        for (auto i = BitsInWord - 1; i >= 0; i--) {
          word_t mask = 1 << i;
          if (x & mask) {
            std::cout << 1;
          }
          else {
            std::cout << 0;
          }
          tx++;
          if (tx == m_w * m_h) {
            std::cout << "|";
          }
        }
      }
    }

    bitmask (int w, int h)
        : m_w (w), m_h (h), m_data (get_size (w, h), 0) {
    }

    bitmask& operator = (const bitmask& rhs) = default;
    bitmask (const bitmask& rhs) = default;

    bitmask& operator = (bitmask&& rhs) noexcept = default;
    bitmask (bitmask&& rhs) noexcept = default;

    [[nodiscard]] int width () const noexcept {
      return m_w;
    }

    [[nodiscard]] int height () const noexcept {
      return m_h;
    }

    void fill () {
      std::fill (m_data.begin (), m_data.end (), std::numeric_limits<word_t>::max ());
    }

    void clear () {
      std::fill (m_data.begin (), m_data.end (), (word_t) 0);
    }

    void set (int x, int y) {
      auto [word_num, bit_pos] = coords (x, y, m_w);
      m_data[word_num] |= (bitmask_traits_t::unit << bit_pos);
    }

    void set (int y, const char* line) {
      for (int i = 0; i < bitmask_traits_t::bits_in_word; i++) {
        if (line[i] != '0' && line[i] != ' ') {
          set (i, y);
        }
      }
    }

    void clear (int x, int y) {
      auto [word_num, bit_pos] = coords (x, y, m_w);
      m_data[word_num] &= ~(bitmask_traits_t::unit << bit_pos);
    }

    [[nodiscard]] bool get (int x, int y) const {
      auto [word_num, bit_pos] = coords (x, y, m_w);
      return (m_data[word_num] & (bitmask_traits_t::unit << bit_pos)) != 0;
    }

  private:
    static std::size_t get_size (int w, int h) {
      auto n = w * h;
      if (n % bitmask_traits_t::bits_in_word) {
        return 1 + n / bitmask_traits_t::bits_in_word;
      }
      return n / bitmask_traits_t::bits_in_word;
    }

    static std::tuple<int, int> coords (int x, int y, int w) {
      auto bit = w * y + x;
      constexpr auto n = bitmask_traits_t::bits_in_word - 1;
      return {
          bit >> bitmask_traits_t::exp,
          bit & n
      };
    }

  private:
    int m_w;
    int m_h;
    std::vector<word_t> m_data;

};

/*
   +----+----------..
   |A   | yoffset
   |  +-+----------..
   +--|B
   |xoffset
   |  |
   :  :
*/

template <int BitsInWord>
class shift_register {
    using word_t = typename detail::bitmask_traits<BitsInWord>::word_t;
  public:
    shift_register (const word_t* start, int bit_pos, int xpos, int row_width)
        : m_data (start),
          m_bit_pos (bit_pos),
          m_xpos (xpos),
          m_row_width (row_width) {
    }

    word_t shift () {
      ENFORCE(m_xpos < m_row_width);
      constexpr auto bits = detail::bitmask_traits<BitsInWord>::bits_in_word;
      constexpr auto& mask = detail::bitmask_traits<BitsInWord>::mask;
      int need_bits = std::min (m_row_width - m_xpos, bits);

      int bits_in_this_word = bits - m_bit_pos;
      word_t xa = *m_data;

      if (bits_in_this_word == need_bits) {
        m_data++;
        m_xpos += need_bits;
        return xa;
      }
      else {
        if (bits_in_this_word > need_bits) {
        //  print_bits<BitsInWord>(xa);
        //  print_bits<BitsInWord>(mask[bits - (m_bit_pos + need_bits)]);

          xa = (xa & mask[bits - (m_bit_pos + need_bits)]) << m_bit_pos;
          m_xpos += need_bits;
          return xa;
        }
        else {
          xa = xa << m_bit_pos;
          m_xpos += (bits - m_bit_pos);
          m_data++;
          if (m_xpos + m_bit_pos <= m_row_width) {
            word_t ya = (*m_data & mask[m_bit_pos]) >> (bits - m_bit_pos);
            m_xpos += m_bit_pos;
            return xa | ya;
          }
          else {
            int take = m_row_width - m_xpos;
            word_t ya = (*m_data & mask[m_bit_pos]) >> (take - m_bit_pos);
            m_xpos = m_row_width;
            return xa | ya;
          }
        }
      }
    }

    [[nodiscard]] int get_x_pos () const {
      return m_xpos;
    }

  private:
    const word_t* m_data;
    int m_bit_pos;
    int m_xpos;
    int m_row_width;

};

template <int BitsInWord>
[[nodiscard]] int overlap_area (const bitmask<BitsInWord>& a, const bitmask<BitsInWord>& b, int xoffset, int yoffset) {

  auto w_bits = std::min (b.m_w, a.m_w - xoffset);

  auto h = std::min (b.m_h, a.m_h - yoffset);

  const auto* a_data = a.m_data.data ();
  const auto* b_data = b.m_data.data ();

  int rc = 0;
  for (int r = 0; r < h; r++) {
    auto [a_word_num, a_word_bit] = bitmask<BitsInWord>::coords (xoffset, yoffset + r, a.m_w);
    auto [b_word_num, b_word_bit] = bitmask<BitsInWord>::coords (0, r, b.m_w);
    shift_register<BitsInWord> sra (a_data + a_word_num, a_word_bit, xoffset, a.m_w);
    shift_register<BitsInWord> srb (b_data + b_word_num, b_word_bit, 0, b.m_w);
    std::cout << "(" << xoffset << "," << yoffset + r << ") " << a_word_num << "," << a_word_bit << std::endl
              << "(" << 0 << "," << r << ") " << b_word_num << "," << b_word_bit << std::endl;
    int x_pos = 0;
    while (x_pos < w_bits) {
      auto x = sra.shift ();
      auto y = srb.shift ();
      std::cout << "a "; print_bits<BitsInWord>(x);
      std::cout << "b "; print_bits<BitsInWord>(y);
      auto n = psnip_builtin_popcount(x & y);
      x_pos = srb.get_x_pos ();
      rc += n;
    }
    //  std::cout << "R " << (yoffset + r) << " = " << row << std::endl;
  }
  return rc;
}

template <int BitsInWord>
[[nodiscard]] int
naive_overlap_area (const bitmask<BitsInWord>& a, const bitmask<BitsInWord>& b, int xoffset, int yoffset) {
  int rc = 0;
  for (int y = yoffset; y < a.height (); y++) {
    int row = 0;
    for (int x = xoffset; x < a.width (); x++) {
      int xb = x - xoffset;
      int yb = y - yoffset;
      if (xb < b.width () && yb < b.height ()) {
        if (a.get (x, y) && b.get (xb, yb)) {
          row++;
          rc++;
        }
      }
    }
    // std::cout << "r " << (yoffset + y) << " = " << row << std::endl;
  }
  return rc;
}

TEST_SUITE("bitmap shift register") {
  TEST_CASE("test shift register #1") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);

    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);

    shift_register<8> sa (a.data (), 0, 0, 9);
    shift_register<8> sb (b.data (), 0, 0, 9);

    REQUIRE (sa.shift () == 0b00010010);
    REQUIRE (sb.shift () == 0b00010110);
    REQUIRE (sa.shift () == 0b10000000);
    REQUIRE (sb.shift () == 0b10000000);

    sa = shift_register<8> (a.data () + 1, 1, 0, 9);
    sb = shift_register<8> (b.data () + 1, 1, 0, 9);

    REQUIRE(sa.shift () == 0b00001000);
    REQUIRE(sb.shift () == 0b10000001);
    REQUIRE(sa.shift () == 0b00000000);
    REQUIRE(sb.shift () == 0b00000000);

    sa = shift_register<8> (a.data () + 2, 1, 0, 9);
    sb = shift_register<8> (b.data () + 2, 1, 0, 9);

    REQUIRE(sa.shift () == 0b00110101);
    REQUIRE(sb.shift () == 0b01110000);
    REQUIRE(sa.shift () == 0b10000000);
    REQUIRE(sb.shift () == 0b10000000);

    sa = shift_register<8> (a.data () + 3, 1, 0, 9);
    sb = shift_register<8> (b.data () + 3, 1, 0, 9);

    REQUIRE(sa.shift () == 0b11000001);
    REQUIRE(sb.shift () == 0b10000100);
    REQUIRE(sa.shift () == 0b00000000);
    REQUIRE(sb.shift () == 0b10000000);

    sa = shift_register<8> (a.data () + 4, 1, 0, 9);
    sb = shift_register<8> (b.data () + 4, 1, 0, 9);

    REQUIRE(sa.shift () == 0b01001001);
    REQUIRE(sb.shift () == 0b11011000);
    REQUIRE(sa.shift () == 0b00000000);
    REQUIRE(sb.shift () == 0b10000000);

    sa = shift_register<8> (a.data () + 5, 1, 0, 9);
    sb = shift_register<8> (b.data () + 5, 1, 0, 9);

    REQUIRE(sa.shift () == 0b01001001);
    REQUIRE(sb.shift () == 0b10010101);
    REQUIRE(sa.shift () == 0b00000000);
    REQUIRE(sb.shift () == 0b00000000);

    sa = shift_register<8> (a.data () + 6, 1, 0, 9);
    sb = shift_register<8> (b.data () + 6, 1, 0, 9);

    REQUIRE(sa.shift () == 0b01110000);
    REQUIRE(sb.shift () == 0b01011101);
    REQUIRE(sa.shift () == 0b10000000);
    REQUIRE(sb.shift () == 0b00000000);

    sa = shift_register<8> (a.data () + 7, 1, 0, 9);
    sb = shift_register<8> (b.data () + 7, 1, 0, 9);

    REQUIRE(sa.shift () == 0b10001000);
    REQUIRE(sb.shift () == 0b00011100);
    REQUIRE(sa.shift () == 0b10000000);
    REQUIRE(sb.shift () == 0b10000000);

    sa = shift_register<8> (a.data () + 8, 1, 0, 9);
    sb = shift_register<8> (b.data () + 8, 1, 0, 9);

    REQUIRE(sa.shift () == 0b11101001);
    REQUIRE(sb.shift () == 0b11001001);
    REQUIRE(sa.shift () == 0b00000000);
    REQUIRE(sb.shift () == 0b10000000);

    sa = shift_register<8> (a.data () + 9, 1, 0, 9);
    sb = shift_register<8> (b.data () + 9, 1, 0, 9);

    REQUIRE(sa.shift () == 0b00101100);
    REQUIRE(sb.shift () == 0b11100000);
    REQUIRE(sa.shift () == 0b00000000);
    REQUIRE(sb.shift () == 0b00000000);

  }

  TEST_CASE("test shift register #2") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);

    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);

    shift_register<8> sa (a.data (), 0, 0, 9);
    shift_register<8> sb (b.data (), 0, 0, 9);

    REQUIRE (sa.shift () == 0b00010010);
    REQUIRE (sb.shift () == 0b00010110);

    REQUIRE (sa.shift () == 0b10000000);
    REQUIRE (sb.shift () == 0b10000000);

    sa = shift_register<8> (a.data () + 1, 1, 0, 9);
    sb = shift_register<8> (b.data () + 1, 1, 0, 9);

    REQUIRE(sa.shift () == 0b00001000);
    REQUIRE(sb.shift () == 0b10000001);

    sa = shift_register<8> (a.data () + 2, 1, 0, 9);
    sb = shift_register<8> (b.data () + 2, 1, 0, 9);

    REQUIRE(sa.shift () == 0b00110101);
    REQUIRE(sb.shift () == 0b01110000);

    sa = shift_register<8> (a.data () + 3, 1, 0, 9);
    sb = shift_register<8> (b.data () + 3, 1, 0, 9);

    REQUIRE(sa.shift () == 0b11000001);
    REQUIRE(sb.shift () == 0b10000100);

    sa = shift_register<8> (a.data () + 4, 1, 0, 9);
    sb = shift_register<8> (b.data () + 4, 1, 0, 9);

    REQUIRE(sa.shift () == 0b01001001);
    REQUIRE(sb.shift () == 0b11011000);

    sa = shift_register<8> (a.data () + 5, 1, 0, 9);
    sb = shift_register<8> (b.data () + 5, 1, 0, 9);
    REQUIRE(sa.shift () == 0b01001001);
    REQUIRE(sb.shift () == 0b10010101);

    sa = shift_register<8> (a.data () + 6, 1, 0, 9);
    sb = shift_register<8> (b.data () + 6, 1, 0, 9);
    REQUIRE(sa.shift () == 0b01110000);
    REQUIRE(sb.shift () == 0b01011101);

    sa = shift_register<8> (a.data () + 7, 1, 0, 9);
    sb = shift_register<8> (b.data () + 7, 1, 0, 9);
    REQUIRE(sa.shift () == 0b10001000);
    REQUIRE(sb.shift () == 0b00011100);

    sa = shift_register<8> (a.data () + 8, 1, 0, 9);
    sb = shift_register<8> (b.data () + 8, 1, 0, 9);
    REQUIRE(sa.shift () == 0b11101001);
    REQUIRE(sb.shift () == 0b11001001);

    sa = shift_register<8> (a.data () + 9, 1, 0, 9);
    sb = shift_register<8> (b.data () + 9, 1, 0, 9);

    REQUIRE(sa.shift () == 0b00101100);
    REQUIRE(sb.shift () == 0b11100000);
  }
}


TEST_SUITE("bitmap") {

  TEST_CASE("test overlap #1") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    a.print_words();
    std::cout << std::endl;
    b.print_words();
    std::cout << std::endl;
    int rc_n = naive_overlap_area (a, b, 0, 0);
    int rc_o = overlap_area (a, b, 0, 0);
    REQUIRE(rc_n == rc_o);
  }
#if 0
  TEST_CASE("test overlap #2") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 1, 0);
    int rc_o = overlap_area (a, b, 1, 0);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #3") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 2, 0);
    int rc_o = overlap_area (a, b, 2, 0);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #4") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 3, 0);
    int rc_o = overlap_area (a, b, 3, 0);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #5") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 4, 0);
    int rc_o = overlap_area (a, b, 4, 0);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #6") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 5, 0);
    int rc_o = overlap_area (a, b, 5, 0);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #7") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 6, 0);
    int rc_o = overlap_area (a, b, 6, 0);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #8") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 7, 0);
    int rc_o = overlap_area (a, b, 7, 0);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #9") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 0, 1);
    int rc_o = overlap_area (a, b, 0, 1);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #10") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 1, 1);
    int rc_o = overlap_area (a, b, 1, 1);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #11") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 2, 1);
    int rc_o = overlap_area (a, b, 2, 1);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #12") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 3, 1);
    int rc_o = overlap_area (a, b, 3, 1);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #13") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 4, 1);
    int rc_o = overlap_area (a, b, 4, 1);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #14") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 5, 1);
    int rc_o = overlap_area (a, b, 5, 1);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #15") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 7, 1);
    int rc_o = overlap_area (a, b, 7, 1);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #16") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 0, 2);
    int rc_o = overlap_area (a, b, 0, 2);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #17") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 1, 2);
    int rc_o = overlap_area (a, b, 1, 2);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #18") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 2, 2);
    int rc_o = overlap_area (a, b, 2, 2);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #19") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 3, 2);
    int rc_o = overlap_area (a, b, 3, 2);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #20") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 4, 2);
    int rc_o = overlap_area (a, b, 4, 2);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #21") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 5, 2);
    int rc_o = overlap_area (a, b, 5, 2);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #22") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 7, 2);
    int rc_o = overlap_area (a, b, 7, 2);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #23") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 0, 3);
    int rc_o = overlap_area (a, b, 0, 3);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #24") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 1, 3);
    int rc_o = overlap_area (a, b, 1, 3);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #25") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 2, 3);
    int rc_o = overlap_area (a, b, 2, 3);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #26") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 3, 3);
    int rc_o = overlap_area (a, b, 3, 3);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #27") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 4, 3);
    int rc_o = overlap_area (a, b, 4, 3);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #28") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 5, 3);
    int rc_o = overlap_area (a, b, 5, 3);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #29") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 7, 3);
    int rc_o = overlap_area (a, b, 7, 3);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #30") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 0, 4);
    int rc_o = overlap_area (a, b, 0, 4);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #31") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 1, 4);
    int rc_o = overlap_area (a, b, 1, 4);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #32") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 2, 4);
    int rc_o = overlap_area (a, b, 2, 4);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #33") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 3, 4);
    int rc_o = overlap_area (a, b, 3, 4);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #34") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 6, 4);
    int rc_o = overlap_area (a, b, 6, 4);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #35") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 7, 4);
    int rc_o = overlap_area (a, b, 7, 4);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #36") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 0, 5);
    int rc_o = overlap_area (a, b, 0, 5);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #37") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 1, 5);
    int rc_o = overlap_area (a, b, 1, 5);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #38") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 2, 5);
    int rc_o = overlap_area (a, b, 2, 5);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #39") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 3, 5);
    int rc_o = overlap_area (a, b, 3, 5);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #40") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 4, 5);
    int rc_o = overlap_area (a, b, 4, 5);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #41") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 5, 5);
    int rc_o = overlap_area (a, b, 5, 5);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #42") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 6, 5);
    int rc_o = overlap_area (a, b, 6, 5);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #43") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 7, 5);
    int rc_o = overlap_area (a, b, 7, 5);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #44") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 0, 6);
    int rc_o = overlap_area (a, b, 0, 6);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #45") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 1, 6);
    int rc_o = overlap_area (a, b, 1, 6);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #46") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 2, 6);
    int rc_o = overlap_area (a, b, 2, 6);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #47") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 3, 6);
    int rc_o = overlap_area (a, b, 3, 6);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #48") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 4, 6);
    int rc_o = overlap_area (a, b, 4, 6);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #49") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 5, 6);
    int rc_o = overlap_area (a, b, 5, 6);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #50") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 6, 6);
    int rc_o = overlap_area (a, b, 6, 6);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #51") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 7, 6);
    int rc_o = overlap_area (a, b, 7, 6);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #52") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 1, 7);
    int rc_o = overlap_area (a, b, 1, 7);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #53") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 2, 7);
    int rc_o = overlap_area (a, b, 2, 7);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #54") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 3, 7);
    int rc_o = overlap_area (a, b, 3, 7);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #55") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 4, 7);
    int rc_o = overlap_area (a, b, 4, 7);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #56") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 5, 7);
    int rc_o = overlap_area (a, b, 5, 7);
    REQUIRE(rc_n == rc_o);
  }

  TEST_CASE("test overlap #57") {
    bitmask<8> a (9, 9);
    a.set (1, 0);
    a.set (4, 0);
    a.set (1, 1);
    a.set (6, 1);
    a.set (8, 1);
    a.set (1, 2);
    a.set (2, 2);
    a.set (2, 3);
    a.set (3, 3);
    a.set (4, 3);
    a.set (7, 3);
    a.set (1, 4);
    a.set (3, 4);
    a.set (6, 4);
    a.set (0, 5);
    a.set (2, 5);
    a.set (6, 5);
    a.set (7, 5);
    a.set (8, 5);
    a.set (1, 6);
    a.set (4, 6);
    a.set (8, 6);
    a.set (3, 7);
    a.set (5, 7);
    a.set (6, 7);
    a.set (7, 7);
    a.set (1, 8);
    a.set (2, 8);
    a.set (4, 8);
    a.set (7, 8);
    bitmask<8> b (9, 9);
    b.set (1, 0);
    b.set (2, 0);
    b.set (4, 0);
    b.set (5, 1);
    b.set (6, 1);
    b.set (1, 2);
    b.set (2, 2);
    b.set (3, 2);
    b.set (5, 2);
    b.set (7, 2);
    b.set (3, 3);
    b.set (7, 3);
    b.set (8, 3);
    b.set (1, 4);
    b.set (2, 4);
    b.set (5, 4);
    b.set (7, 4);
    b.set (1, 5);
    b.set (4, 5);
    b.set (5, 5);
    b.set (6, 5);
    b.set (8, 5);
    b.set (1, 6);
    b.set (3, 6);
    b.set (4, 6);
    b.set (5, 6);
    b.set (0, 7);
    b.set (3, 7);
    b.set (6, 7);
    b.set (7, 7);
    b.set (4, 8);
    b.set (5, 8);
    b.set (6, 8);
    b.set (7, 8);
    int rc_n = naive_overlap_area (a, b, 6, 7);
    int rc_o = overlap_area (a, b, 6, 7);
    REQUIRE(rc_n == rc_o);
  }
#endif
/*
  TEST_CASE("bitmap overlap 1") {
    using bm_t = bitmask<8>;
    auto a = bm_t::create_random(9, 9);
    auto b = bm_t::create_random(9, 9);

    int k = 0;
    for (int y = 0; y<a.height() - 1; y++) {
      for (int x = 0; x<a.width() - 1; x++) {
        int rc_n = naive_overlap_area (a, b, x, y);
        int rc = overlap_area (a, b, x, y);
        if (rc != rc_n) {
          k++;
          std::cout << "TEST_CASE(\"test overlap #" << k << "\") {" << std::endl;

          a.debug ("a");
          b.debug ("b");

          std::cout << "int rc_n = naive_overlap_area (a, b, " << x << ", " << y << ");" << std::endl;
          std::cout << "int rc_o = overlap_area (a, b, " << x << ", " << y << ");" << std::endl;

          std::cout << "REQUIRE(rc_n == rc_o);" << std::endl;
          std::cout << "}" << std::endl;
        } else {
          REQUIRE(rc == rc_n);
        }
      }
    }
  }
*/
}

#if 0
                                                                                                                        int main(int argc, char* argv[]) {
  /*
  //neutrino::assets::load_fon ("/home/igor/proj/ares/fon/120TMS18.FON");
  try {
    auto s = neutrino::assets::load_image ("/home/igor/tmp/1/crater.ham");
    neutrino::assets::save ("/home/igor/tmp/1/crater.png", s);
  } catch (std::exception& e) {
    std::cout << "Error: " << e.what() << std::endl;
  }
   */
  auto v = neutrino::hal::get_glyph (neutrino::hal::font::IBM_8x16, 'A');
  using bm8_t = bitmask<8>;
  bm8_t bm(8, 16);
  for (int y=0; y<16; y++) {
    for (int x=0; x<8; x++) {
      if (v[y][x]) {
        bm.set (x, y);
      }
    }
  }

  for (int y=0; y<16; y++) {
    for (int x = 0; x < 8; x++) {
      if (bm.get (x, y)) {
        std::cout << "+";
      } else {
        std::cout << " ";
      }
    }
    std::cout << std::endl;
  }
  return 0;
}

#endif