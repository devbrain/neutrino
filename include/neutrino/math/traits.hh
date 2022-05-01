//
// Created by igor on 28/04/2022.
//

#ifndef INCLUDE_NEUTRINO_MATH_TRAITS_HH
#define INCLUDE_NEUTRINO_MATH_TRAITS_HH

#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_float2.hpp>

namespace neutrino::math::detail {
  template <typename T>
  struct vector_traits;

  template <>
  struct vector_traits<float> {
    using vector = glm::vec2;
    using scalar = float;
  };

  template <>
  struct vector_traits<int> {
    using vector = glm::ivec2;
    using scalar = int;
  };
}

#endif //INCLUDE_NEUTRINO_MATH_TRAITS_HH
