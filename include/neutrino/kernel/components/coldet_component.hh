//
// Created by igor on 06/06/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_COMPONENTS_COLDET_COMPONENT_HH
#define INCLUDE_NEUTRINO_KERNEL_COMPONENTS_COLDET_COMPONENT_HH

#include <neutrino/math/rect.hh>

namespace neutrino::kernel {
  struct coldet_component {
    neutrino::math::box2d<float> aabb;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_COMPONENTS_COLDET_COMPONENT_HH
