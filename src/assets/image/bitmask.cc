//
// Created by igor on 07/06/2022.
//

#include <tuple>
#include <neutrino/assets/image/bitmask.hh>
#include <algorithm>

namespace neutrino::assets {

  static std::size_t get_size(int w, int h) {
    auto n = w*h;
    if (n % detail::bitmask_traits_t::bits_in_word) {
      return 1 + n / detail::bitmask_traits_t::bits_in_word;
    }
    return n / detail::bitmask_traits_t::bits_in_word;
  }

  static std::tuple<int, int> coords(int x, int y, int w) {
     auto bit = w * y + x;
     return {
         bit >> detail::bitmask_traits_t::exp,
         bit & (detail::bitmask_traits_t::bits_in_word - 1)
     };
  }

  bitmask::bitmask(int w, int h)
  : m_w(w), m_h(h), m_data(get_size (w, h), 0) {
  }

  int bitmask::width() const noexcept {
    return m_w;
  }

  int bitmask::height() const noexcept {
    return m_h;
  }

  void bitmask::set(int x, int y) {
    auto [word_num, bit_pos]  = coords (x, y, m_w);
    m_data[word_num] |= (detail::bitmask_traits_t ::unit << bit_pos);
  }

  void bitmask::clear(int x, int y) {
    auto [word_num, bit_pos]  = coords (x, y, m_w);
    m_data[word_num] &= ~(detail::bitmask_traits_t ::unit << bit_pos);
  }

  bool bitmask::get(int x, int y) const {
    auto [word_num, bit_pos]  = coords (x, y, m_w);
    return (m_data[word_num] & (detail::bitmask_traits_t ::unit << bit_pos)) != 0;
  }

  /*
   +----+----------..
   |A   | yoffset
   |  +-+----------..
   +--|B
   |xoffset
   |  |
   :  :
*/

  bool bitmask::overlap(const bitmask& a, const bitmask& b, int xoffs, int yoffs) {
    auto [word_num, bit_pos]  = coords (xoffs, yoffs, a.m_w);
    const auto* a_data = a.m_data.data() + word_num;
    const auto* b_data = b.m_data.data();
    auto shift = detail::bitmask_traits_t ::bits_in_word - bit_pos;
    auto b_row_len_words = std::get<0>(coords(0, 1, b.m_w));
    auto a_row_len_words = std::get<0>(coords(xoffs, yoffs+1, a.m_w));
    auto test_area_w = std::min(a.m_w - xoffs, b.m_w);
    auto test_area_h = std::min(a.m_h - yoffs, b.m_h);

    for (int test_y = 0; test_y < test_area_h; test_y++) {
      int test_x = 0;
      // in test first word
      // todo
      //
      test_x += detail::bitmask_traits_t ::bits_in_word;
      for (; test_x < test_area_w - 1; test_x += detail::bitmask_traits_t ::bits_in_word) {

      }
      // test last word

      a_data += a_row_len_words;
      b_data += b_row_len_words;
    }

    return false;
  }
}