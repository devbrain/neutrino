//
// Created by igor on 26/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_IMAGE_IMAGE_HH
#define INCLUDE_NEUTRINO_ASSETS_IMAGE_IMAGE_HH

#include <functional>
#include <optional>

#include <neutrino/hal/video/surface.hh>
#include <neutrino/math/rect.hh>

namespace neutrino::assets {
  class image {
    public:
      using loader_t = std::function<hal::surface()>;
    public:
      image() = default;
      explicit image (loader_t ldr);
      image (loader_t ldr, math::dimension2di_t dims);

      [[nodiscard]] hal::surface load() const;
      [[nodiscard]] std::optional<math::dimension2di_t> dimensions () const;
      void dimensions(math::dimension2di_t d);

      explicit operator bool () const;
    private:
      loader_t m_loader;
      std::optional<math::dimension2di_t> m_dims;
  };
}

#endif //INCLUDE_NEUTRINO_ASSETS_IMAGE_IMAGE_HH
