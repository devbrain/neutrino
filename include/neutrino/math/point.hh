//
// Created by igor on 27/06/2021.
//

#ifndef NEUTRINO_MATH_POINT_HH
#define NEUTRINO_MATH_POINT_HH

#include <neutrino/math/traits.hh>

namespace neutrino::math {
  using point2d = detail::vector_traits<int>::vector;
  using point2f = detail::vector_traits<float>::vector;
}

#endif
