//
// Created by igor on 27/06/2021.
//

#ifndef NEUTRINO_MATH_RECT_HH
#define NEUTRINO_MATH_RECT_HH

#include <neutrino/math/point.hh>

namespace neutrino::math {
  using dimension_t = detail::vector_traits<int>::vector;

  template <typename T>
  struct box2d {
    using scalar_t = typename detail::vector_traits<T>::scalar;
    using point_t = typename detail::vector_traits<T>::vector;

    box2d (scalar_t x, scalar_t y, scalar_t w, scalar_t h)
        : point{x, y}, dims{w, h} {
    }

    box2d () = default;

    box2d (const point_t& corner, scalar_t w, scalar_t h)
        : point{corner}, dims{w, h} {
    }

    box2d (const point_t& corner, const point_t& wh )
        : point{corner}, dims{wh} {
    }

    constexpr T left() const noexcept
    {
      return point.x;
    }

    constexpr T right() const noexcept
    {
      return point.x + dims.x;
    }

    constexpr T bottom() const noexcept
    {
      return point.y + dims.y;
    }

    constexpr T top() const noexcept
    {
      return point.y;
    }


    constexpr point_t top_left() const noexcept
    {
      return point;
    }

    constexpr point_t center() const noexcept
    {
      return point + (dims / static_cast<T>(2));
    }

    constexpr point_t size() const noexcept
    {
      return dims;
    }

    constexpr bool contains(const box2d<T>& box) const noexcept
    {
      return left() <= box.left() && box.right() <= right() &&
             top() <= box.top() && box.bottom() <= bottom();
    }

    constexpr bool intersects(const box2d<T>& box) const noexcept
    {
      return !(left() >= box.right() || right() <= box.left() ||
               top() >= box.bottom() || bottom() <= box.top());
    }

    point_t point;
    point_t dims;
  };

  using rect = box2d<int>;
}

#endif
