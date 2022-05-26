//
// Created by igor on 26/05/2022.
//

#include <neutrino/assets/image/image.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::assets {
  image::image (loader_t ldr)
  : m_loader(std::move(ldr)) {
  }

  image::image (loader_t ldr, math::dimension2di_t dims)
  : m_loader(std::move(ldr)), m_dims(dims) {
  }

  hal::surface image::load() const {
      auto s = m_loader ();
      if (m_dims) {
        auto [w, h] = s.dimensions ();
        ENFORCE((int) w == m_dims->x && (int) h == m_dims->y);
      }
      return s;

  }

  std::optional<math::dimension2di_t> image::dimensions () const {
    return m_dims;
  }

  void image::dimensions(math::dimension2di_t d) {
    m_dims = d;
  }

  image::operator bool () const {
    return !!m_loader;
  }
}