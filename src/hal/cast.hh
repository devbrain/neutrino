//
// Created by igor on 27/06/2021.
//

#ifndef NEUTRINO_HAL_MATH_CAST_HH
#define NEUTRINO_HAL_MATH_CAST_HH

#include <optional>
#include <neutrino/sdl/geometry.hh>
#include <neutrino/sdl/color.hh>
#include <neutrino/math/point.hh>
#include <neutrino/math/rect.hh>
#include <neutrino/hal/color.hh>

namespace neutrino {
    inline
    sdl::point cast(const math::point2d& p) {
        return {p[0], p[1]};
    }

    inline
    math::point2d cast (const sdl::point& p) {
        return {p.x, p.y};
    }

    inline
    sdl::rect cast(const math::rect& r) {
        return {r.point[0], r.point[1], r.dims[0], r.dims[1]};
    }

    inline
    math::rect cast (const sdl::rect& r) {
        return {r.x, r.y, r.w, r.h};
    }

    inline
    sdl::color cast(const neutrino::hal::color& c) {
        return {c.r, c.g, c.b, c.a};
    }

    inline
    neutrino::hal::color cast(const sdl::color& c) {
        return {c.r, c.g, c.b, c.a};
    }

    template <typename T1, typename T2>
    std::optional<T1> cast (const std::optional<T2>& x) {
        if (!x) {
            return std::nullopt;
        }
        return cast(*x);
    }

}

#endif
