//
// Created by igor on 26/06/2022.
//

#ifndef APPS_FON_BM_HH
#define APPS_FON_BM_HH

#include <vector>
#include <algorithm>
#include <random>
#include <iostream>
#include <neutrino/utils/builtin.h>
#include <neutrino/utils/exception.hh>
#include "traits.hh"


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
    static constexpr auto bits_in_word = BitsInWord;
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

    void print_words () const {
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
      auto m = bitmask_traits_t::unit << bit_pos;
      m_data[word_num] |= m;
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
      auto m = bitmask_traits_t::unit << bit_pos;
      m_data[word_num] &= ~m;
    }

    [[nodiscard]] bool get (int x, int y) const {
      auto [word_num, bit_pos] = coords (x, y, m_w);
      auto m = bitmask_traits_t::unit << bit_pos;
      return (m_data[word_num] & m) == m;
    }

    std::tuple<word_t, int> get_row_word(int x_start, int y) {
      word_t w = detail::bitmask_traits<BitsInWord>::zero;
      int num = 0;
      int n = bitmask_traits_t::bits_in_word;
      for (int x = x_start; x<m_w; x++) {
        int px = x - x_start;
        auto [wr, b] = coords (px, 0, n);
        if (wr == 0) {
          if (get (x, y)) {
            word_t m = bitmask_traits_t::unit << b;
            w |= m;
          }
          num++;
        }
      }
      return {w, num};
    }


#if defined(DOCTEST_LIBRARY_INCLUDED)
  public:
#else
  private:
#endif
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
          n - (bit & n)
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
          m_consumed(0),
          m_row_width (row_width) {
    }

    word_t shift () {

      constexpr auto bits = detail::bitmask_traits<BitsInWord>::bits_in_word;
      constexpr auto& mask = detail::bitmask_traits<BitsInWord>::mask;

      int need_bits = std::min (m_row_width, bits);
      int bits_in_this_word = 1 + m_bit_pos;

      m_consumed += need_bits;
      m_xpos += need_bits;
      m_row_width -= need_bits;

      word_t xa = *m_data;
      int shift = bits - 1 - m_bit_pos;

      if (bits_in_this_word == need_bits) {
        m_data++;
        xa = xa << shift;
        return xa;
      }
      else {
        if (bits_in_this_word > need_bits) {
          xa = (xa << shift) & mask[need_bits];
          return xa;
        }
        else {

          xa <<= shift;

          m_data++;
          int shift2 = (bits - shift);

          word_t ya = (*m_data  >> shift2);

          word_t w = xa | ya;

          word_t m = mask[need_bits];

          return w & m;
        }
      }
    }

    [[nodiscard]] int has_bits () const {
      return m_consumed;
    }

    [[nodiscard]] int get_x_pos() const {
      return m_xpos;
    }

    [[nodiscard]] int get_bit_pos() const {
      return m_bit_pos;
    }

  private:
    const word_t* m_data;
    int m_bit_pos;
    int m_xpos;
    int m_consumed;
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
    int row_n = 0;
    auto [a_word_num, a_word_bit] = bitmask<BitsInWord>::coords (xoffset, yoffset + r, a.m_w);
    auto [b_word_num, b_word_bit] = bitmask<BitsInWord>::coords (0, r, b.m_w);

    shift_register<BitsInWord> sra (a_data + a_word_num, a_word_bit, xoffset, w_bits);
    shift_register<BitsInWord> srb (b_data + b_word_num, b_word_bit, 0, w_bits);

    int has_bits = 0;

    while (has_bits < w_bits) {
      auto x = sra.shift ();
      auto y = srb.shift ();

      typename detail::bitmask_traits<BitsInWord>::word_t w = x & y;
      auto n = psnip_builtin_popcount(w);
      has_bits = srb.has_bits ();
      rc += n;
      row_n += n;
    }

  }
  return rc;
}

template <int BitsInWord>
[[nodiscard]] int
naive_overlap_area (const bitmask<BitsInWord>& a, const bitmask<BitsInWord>& b, int xoffset, int yoffset) {
  int rc = 0;

  auto w = std::min (b.width(), a.width() - xoffset);
  auto h = std::min (b.height(), a.height() - yoffset);


  for (int y = 0; y < h; y++) {
    int row = 0;
    std::string arow = "";
    std::string brow = "";
    for (int x = 0; x < w; x++) {
      int xa = x + xoffset;
      int ya = y + yoffset;
      if (a.get (xa, ya)) {
        arow += "1";
      } else {
        arow += "0";
      }
      if (b.get (x, y)) {
        brow += "1";
      } else {
        brow += "0";
      }
    }

    for (int x = 0; x < w; x++) {
      int xa = x + xoffset;
      int ya = y + yoffset;
      if (a.get (xa, ya) && b.get (x, y)) {
        row++;
        rc++;
      }
    }
  }
  return rc;
}

#endif //APPS_FON_BM_HH
