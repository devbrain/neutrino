//
// Created by igor on 26/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_IMAGE_LAZY_IMAGE_LOADER_HH
#define INCLUDE_NEUTRINO_ASSETS_IMAGE_LAZY_IMAGE_LOADER_HH

#include <functional>
#include <optional>

#include <neutrino/hal/video/surface.hh>
#include <neutrino/math/rect.hh>

namespace neutrino::assets {
  class lazy_image_loader {
    public:
      using loader_t = std::function<hal::surface()>;
    public:
      lazy_image_loader() = default;
      explicit lazy_image_loader (loader_t ldr);
      lazy_image_loader (loader_t ldr, math::dimension2di_t dims);

      [[nodiscard]] hal::surface load() const;
      [[nodiscard]] std::optional<math::dimension2di_t> dimensions () const;
      void dimensions(math::dimension2di_t d);

      explicit operator bool () const;
    private:
      loader_t m_loader;
      std::optional<math::dimension2di_t> m_dims;
  };
}

#endif //INCLUDE_NEUTRINO_ASSETS_IMAGE_LAZY_IMAGE_LOADER_HH
