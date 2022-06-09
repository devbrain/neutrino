//
// Created by igor on 07/06/2022.
//

#include <neutrino/assets/image/bitmask.hh>

namespace neutrino::assets {

  static std::size_t get_size(int w, int h) {
    auto n = w*h;
    if (n % detail::bitmask_traits_t::bits_in_word) {
      return 1 + n / detail::bitmask_traits_t::bits_in_word;
    }
    return n / detail::bitmask_traits_t::bits_in_word;
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
    auto pos = (m_w*y + x) >> detail::bitmask_traits_t::exp;
    auto bit_pos = (m_w*y + x) & (detail::bitmask_traits_t::bits_in_word - 1);
    m_data[pos] |= (detail::bitmask_traits_t ::unit << bit_pos);
  }

  void bitmask::clear(int x, int y) {
    auto pos = (m_w*y + x) >> detail::bitmask_traits_t::exp;
    auto bit_pos = (m_w*y + x) & (detail::bitmask_traits_t::bits_in_word - 1);
    m_data[pos] &= ~(detail::bitmask_traits_t ::unit << bit_pos);
  }

  bool bitmask::get(int x, int y) const {
    auto pos = (m_w*y + x) >> detail::bitmask_traits_t::exp;
    auto bit_pos = (m_w*y + x) & (detail::bitmask_traits_t::bits_in_word - 1);
    return (m_data[pos] & (detail::bitmask_traits_t ::unit << bit_pos)) != 0;
  }
}