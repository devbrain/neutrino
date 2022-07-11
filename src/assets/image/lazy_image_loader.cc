//
// Created by igor on 26/05/2022.
//

#include <neutrino/assets/image/lazy_image_loader.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino::assets {
  lazy_image_loader::lazy_image_loader (loader_t ldr)
  : m_loader(std::move(ldr)) {
  }

  lazy_image_loader::lazy_image_loader (loader_t ldr, math::dimension2di_t dims)
  : m_loader(std::move(ldr)), m_dims(dims) {
  }

  hal::surface lazy_image_loader::load() const {
      auto s = m_loader ();
      if (m_dims) {
        auto [w, h] = s.dimensions ();
        ENFORCE((int) w == m_dims->x && (int) h == m_dims->y);
      }
      return s;

  }

  std::optional<math::dimension2di_t> lazy_image_loader::dimensions () const {
    return m_dims;
  }

  void lazy_image_loader::dimensions(math::dimension2di_t d) {
    m_dims = d;
  }

  lazy_image_loader::operator bool () const {
    return !!m_loader;
  }
}