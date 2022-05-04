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

    private:
      palette (std::unique_ptr<detail::palette_impl>&& impl);
    private:
      spimpl::unique_impl_ptr<detail::palette_impl> m_pimpl;
  };
}

#endif //INCLUDE_NEUTRINO_HAL_VIDEO_PALETTE_HH
