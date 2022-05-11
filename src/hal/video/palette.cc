//
// Created by igor on 03/05/2022.
//

#include <neutrino/hal/video/palette.hh>
#include "palette_impl.hh"

namespace neutrino::hal {
  palette::palette() {
    m_pimpl = spimpl::make_unique_impl<detail::palette_impl>();
  }

  palette::palette (std::size_t num_colors) {
    m_pimpl = spimpl::make_unique_impl<detail::palette_impl>(num_colors);
  }

  [[nodiscard]] std::size_t palette::size () const noexcept {
    return m_pimpl->palette.size();
  }
  [[nodiscard]] bool palette::empty () const noexcept {
    return m_pimpl->palette.empty();
  }

  [[nodiscard]] color palette::operator [] (std::size_t n) const {
    return get(n);
  }

  [[nodiscard]] color palette::get (std::size_t n) const {
    const auto c = m_pimpl->palette[n];
    return {c.r, c.g, c.b, c.a};
  }

  void palette::set(std::size_t n, color c) {
    m_pimpl->palette[n] = sdl::color {c.r, c.g, c.b, c.a};
  }

  void palette::set(std::size_t n, uint8_t r, uint8_t g, uint8_t b) {
    m_pimpl->palette[n] = sdl::color {r, g, b, 0xFF};
  }

  palette::palette (std::unique_ptr<detail::palette_impl>&& impl)
  : m_pimpl(std::move(impl)) {
  }

}