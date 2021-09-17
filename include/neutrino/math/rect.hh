//
// Created by igor on 27/06/2021.
//

#ifndef NEUTRINO_MATH_RECT_HH
#define NEUTRINO_MATH_RECT_HH

#include <neutrino/math/point.hh>

namespace neutrino::math {
  using dimension_t = glm::ivec2;

  struct rect {
    rect (int x, int y, int w, int h)
        : point{x, y}, dims{w, h} {

    }

    rect () = default;

    rect (const point2d& corner, int w, int h)
        : point{corner}, dims{w, h} {

    }

    point2d point;
    dimension_t dims;
  };
}

#endif
