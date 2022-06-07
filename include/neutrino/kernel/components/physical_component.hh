//
// Created by igor on 06/06/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_COMPONENTS_PHYSICAL_COMPONENT_HH
#define INCLUDE_NEUTRINO_KERNEL_COMPONENTS_PHYSICAL_COMPONENT_HH

#include <neutrino/math/point.hh>

namespace neutrino::kernel {
  struct physical_component {
    physical_component()
    : speed(0,0), acceleration(0), mass(1.0) {}

    math::point2f speed;
    float acceleration;
    float mass;
  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_COMPONENTS_PHYSICAL_COMPONENT_HH
