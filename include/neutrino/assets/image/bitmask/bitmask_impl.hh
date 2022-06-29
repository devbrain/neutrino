//
// Created by igor on 29/06/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_IMAGE_BITMASK_BITMASK_IMPL_HH
#define INCLUDE_NEUTRINO_ASSETS_IMAGE_BITMASK_BITMASK_IMPL_HH

#if defined(DOCTEST_LIBRARY_INCLUDED)
#include <random>
#endif

#include <tuple>
#include <vector>
#include <limits>
#include <neutrino/assets/image/bitmask/bitmask_traits.hh>

namespace neutrino::assets::impl {

  template <int BitsInWord> class bitmask;

  template <int BitsInWord>
  [[nodiscard]] int overlap_area (const bitmask<BitsInWord>& a, const bitmask<BitsInWord>& b, int xoffset, int yoffset);

  template <int BitsInWord>
  [[nodiscard]] bool overlaps (const bitmask<BitsInWord>& a, const bitmask<BitsInWord>& b, int xoffset, int yoffset);

  template <int BitsInWord>
  class bitmask {
      template <int N>
      friend int overlap_area (const bitmask<N>& a, const bitmask<N>& b, int xoffset, int yoffset);

      template <int N>
      friend bool overlaps (const bitmask<N>& a, const bitmask<N>& b, int xoffset, int yoffset);
    public:
      using bitmask_traits_t = detail::bitmask_traits<BitsInWord>;
      using word_t = typename bitmask_traits_t::word_t;
      static constexpr auto bits_in_word = BitsInWord;
    public:

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
#if defined(DOCTEST_LIBRARY_INCLUDED)
    public:
      static bitmask<BitsInWord> create_random (int w, int h) {
        bitmask<BitsInWord> bm (w, h);
        std::random_device random_device;
        std::mt19937 random_engine (random_device ());
        std::uniform_int_distribution<word_t> d (0, std::numeric_limits<word_t>::max());
        for (auto& w : bm.m_data) {
          w = d(random_engine);
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
#endif

  };
}

#endif //INCLUDE_NEUTRINO_ASSETS_IMAGE_BITMASK_BITMASK_IMPL_HH
