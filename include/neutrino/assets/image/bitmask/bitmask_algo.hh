//
// Created by igor on 29/06/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_IMAGE_BITMASK_BITMASK_ALGO_HH
#define INCLUDE_NEUTRINO_ASSETS_IMAGE_BITMASK_BITMASK_ALGO_HH

#include <neutrino/assets/image/bitmask/bitmask_impl.hh>
#include <neutrino/utils/builtin.h>

namespace neutrino::assets {

  namespace detail {
    template <int BitsInWord>
    class shift_register {
        using word_t = typename bitmask_traits<BitsInWord>::word_t;
      public:
        shift_register (const word_t* start, int bit_pos, int xpos, int row_width)
            : m_data (start),
              m_bit_pos (bit_pos),
              m_xpos (xpos),
              m_consumed (0),
              m_row_width (row_width) {
        }

        word_t shift () {

          constexpr auto bits = bitmask_traits<BitsInWord>::bits_in_word;
          constexpr auto& mask = bitmask_traits<BitsInWord>::mask;

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

              word_t ya = (*m_data >> shift2);

              word_t w = xa | ya;

              word_t m = mask[need_bits];

              return w & m;
            }
          }
        }

        [[nodiscard]] int has_bits () const {
          return m_consumed;
        }

      private:
        const word_t* m_data;
        int m_bit_pos;
        int m_xpos;
        int m_consumed;
        int m_row_width;
    };
  }
  namespace impl {
    template <int BitsInWord>
    [[nodiscard]] int
    overlap_area (const bitmask<BitsInWord>& a, const bitmask<BitsInWord>& b, int xoffset, int yoffset) {

      auto w_bits = std::min (b.m_w, a.m_w - xoffset);

      auto h = std::min (b.m_h, a.m_h - yoffset);

      const auto* a_data = a.m_data.data ();
      const auto* b_data = b.m_data.data ();

      int rc = 0;
      for (int r = 0; r < h; r++) {

        auto [a_word_num, a_word_bit] = bitmask<BitsInWord>::coords (xoffset, yoffset + r, a.m_w);
        auto [b_word_num, b_word_bit] = bitmask<BitsInWord>::coords (0, r, b.m_w);

        detail::shift_register<BitsInWord> sra (a_data + a_word_num, a_word_bit, xoffset, w_bits);
        detail::shift_register<BitsInWord> srb (b_data + b_word_num, b_word_bit, 0, w_bits);

        int has_bits = 0;

        while (has_bits < w_bits) {
          auto x = sra.shift ();
          auto y = srb.shift ();

          typename detail::bitmask_traits<BitsInWord>::word_t w = x & y;
          auto n = psnip_builtin_popcount(w);
          has_bits = srb.has_bits ();
          rc += n;
        }

      }
      return rc;
    }

    template <int BitsInWord>
    [[nodiscard]] bool overlaps (const bitmask<BitsInWord>& a, const bitmask<BitsInWord>& b, int xoffset, int yoffset) {

      auto w_bits = std::min (b.m_w, a.m_w - xoffset);

      auto h = std::min (b.m_h, a.m_h - yoffset);

      const auto* a_data = a.m_data.data ();
      const auto* b_data = b.m_data.data ();

      for (int r = 0; r < h; r++) {
        auto [a_word_num, a_word_bit] = bitmask<BitsInWord>::coords (xoffset, yoffset + r, a.m_w);
        auto [b_word_num, b_word_bit] = bitmask<BitsInWord>::coords (0, r, b.m_w);

        detail::shift_register<BitsInWord> sra (a_data + a_word_num, a_word_bit, xoffset, w_bits);
        detail::shift_register<BitsInWord> srb (b_data + b_word_num, b_word_bit, 0, w_bits);

        int has_bits = 0;

        while (has_bits < w_bits) {
          auto x = sra.shift ();
          auto y = srb.shift ();

          typename detail::bitmask_traits<BitsInWord>::word_t w = x & y;
          if (w) {
            return true;
          }
          has_bits = srb.has_bits ();
        }

      }
      return false;
    }
  }

}

#endif //INCLUDE_NEUTRINO_ASSETS_IMAGE_BITMASK_BITMASK_ALGO_HH
