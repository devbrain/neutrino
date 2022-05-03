//
// Created by igor on 27/06/2021.
//

#ifndef NEUTRINO_MATH_POINT_HH
#define NEUTRINO_MATH_POINT_HH

#include <neutrino/math/traits.hh>

namespace neutrino::math {
  using point2d = detail::vector_traits<int>::vector2d;
  using point2f = detail::vector_traits<float>::vector2d;

  template <typename TO, typename FROM>
  typename detail::vector_traits<TO>::vector2d convert(const typename detail::vector_traits<FROM>::vector2d& v) {
    return {static_cast<TO>(v[0]), static_cast<TO>(v[1])};
  }
}

#endif
