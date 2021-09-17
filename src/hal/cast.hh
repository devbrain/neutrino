//
// Created by igor on 27/06/2021.
//

#ifndef NEUTRINO_HAL_MATH_CAST_HH
#define NEUTRINO_HAL_MATH_CAST_HH

#include <optional>
#include <hal/sdl/geometry.hh>
#include <hal/sdl/color.hh>
#include <hal/sdl/surface.hh>

#include <neutrino/math/point.hh>
#include <neutrino/math/rect.hh>
#include <neutrino/hal/video/color.hh>
#include <neutrino/hal/video/blend_mode.hh>
#include <neutrino/utils/exception.hh>

namespace neutrino {
  inline
  sdl::point cast (const math::point2d& p) {
    return {p[0], p[1]};
  }

  inline
  math::point2d cast (const sdl::point& p) {
    return {p.x, p.y};
  }

  inline
  sdl::rect cast (const math::rect& r) {
    return {r.point[0], r.point[1], r.dims[0], r.dims[1]};
  }

  inline
  math::rect cast (const sdl::rect& r) {
    return {r.x, r.y, r.w, r.h};
  }

  inline
  sdl::color cast (const neutrino::hal::color& c) {
    return {c.r, c.g, c.b, c.a};
  }

  inline
  neutrino::hal::color cast (const sdl::color& c) {
    return {c.r, c.g, c.b, c.a};
  }

  template <typename T1, typename T2>
  std::optional<T1> cast (const std::optional<T2>& x) {
    if (!x) {
      return std::nullopt;
    }
    return cast (*x);
  }

  inline
  sdl::blend_mode cast (neutrino::hal::blend_mode v) {
    switch (v) {
      case neutrino::hal::blend_mode::NONE:
        return sdl::blend_mode::NONE;
      case neutrino::hal::blend_mode::ADD:
        return sdl::blend_mode::ADD;
      case neutrino::hal::blend_mode::BLEND:
        return sdl::blend_mode::BLEND;
      case neutrino::hal::blend_mode::MOD:
        return sdl::blend_mode::MOD;
      default:
        RAISE_EX("Should not be here");
    }
  }

  inline
  neutrino::hal::blend_mode cast (sdl::blend_mode v) {
    switch (v) {
      case sdl::blend_mode::NONE:
        return neutrino::hal::blend_mode::NONE;
      case sdl::blend_mode::ADD:
        return neutrino::hal::blend_mode::ADD;
      case sdl::blend_mode::BLEND:
        return neutrino::hal::blend_mode::BLEND;
      case sdl::blend_mode::MOD:
        return neutrino::hal::blend_mode::MOD;
      default:
        RAISE_EX("Should not be here");
    }
  }
}

#endif
