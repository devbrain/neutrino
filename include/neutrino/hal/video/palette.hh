//
// Created by igor on 03/05/2022.
//

#ifndef INCLUDE_NEUTRINO_HAL_VIDEO_PALETTE_HH
#define INCLUDE_NEUTRINO_HAL_VIDEO_PALETTE_HH

#include <neutrino/hal/video/color.hh>
#include <neutrino/utils/spimpl.h>

namespace neutrino::hal {
  namespace detail {
    struct palette_impl;
  }

  class surface;

  class palette final {
      friend class surface;
    public:
      palette();
      explicit palette (std::size_t num_colors);

      [[nodiscard]] std::size_t size () const noexcept;
      [[nodiscard]] bool empty () const noexcept;

      [[nodiscard]] color operator [] (std::size_t n) const;
      [[nodiscard]] color get (std::size_t n) const;
      void set(std::size_t n, color c);
      void set(std::size_t n, uint8_t r, uint8_t g, uint8_t b);

      friend void swap(palette& lhs, palette& rhs) {
        std::swap(lhs.m_pimpl, rhs.m_pimpl);
      }

      explicit operator bool() const
      {
        return !(m_pimpl == nullptr);
      }
    private:
      palette (std::unique_ptr<detail::palette_impl>&& impl);
    private:
      spimpl::unique_impl_ptr<detail::palette_impl> m_pimpl;
  };
}

#endif //INCLUDE_NEUTRINO_HAL_VIDEO_PALETTE_HH
