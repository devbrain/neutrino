//
// Created by igor on 27/06/2021.
//

#ifndef NEUTRINO_HAL_COLOR_HH
#define NEUTRINO_HAL_COLOR_HH

#include <cstdint>

namespace neutrino::hal {
    struct color {
        color()
        : r(0), g(0), b(0), a(0) {}
        color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 0)
        : r(r_), g(g_), b(b_), a(a_) {}

        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint8_t a;
    };
}

#endif

