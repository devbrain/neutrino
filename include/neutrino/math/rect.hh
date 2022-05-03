//
// Created by igor on 27/06/2021.
//

#ifndef NEUTRINO_MATH_RECT_HH
#define NEUTRINO_MATH_RECT_HH

#include <neutrino/math/point.hh>

namespace neutrino::math {
  using dimension2di_t = detail::vector_traits<int>::vector2d;
  using dimension2df_t = detail::vector_traits<float>::vector2d;

  template <typename T>
  struct box2d {
    using scalar_t = typename detail::vector_traits<T>::scalar;
    using point_t = typename detail::vector_traits<T>::vector2d;

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

    [[nodiscard]] constexpr T left() const noexcept
    {
      return point.x;
    }

    [[nodiscard]] constexpr T right() const noexcept
    {
      return point.x + dims.x;
    }

    [[nodiscard]] constexpr T bottom() const noexcept
    {
      return point.y + dims.y;
    }

    [[nodiscard]] constexpr T top() const noexcept
    {
      return point.y;
    }


    [[nodiscard]] constexpr point_t top_left() const noexcept
    {
      return point;
    }

    [[nodiscard]] constexpr point_t center() const noexcept
    {
      return point + (dims / static_cast<T>(2));
    }

    [[nodiscard]] constexpr point_t size() const noexcept
    {
      return dims;
    }

    [[nodiscard]] constexpr bool contains(const box2d<T>& box) const noexcept
    {
      return left() <= box.left() && box.right() <= right() &&
             top() <= box.top() && box.bottom() <= bottom();
    }

    [[nodiscard]] constexpr bool intersects(const box2d<T>& box) const noexcept
    {
      return !(left() >= box.right() || right() <= box.left() ||
               top() >= box.bottom() || bottom() <= box.top());
    }

    point_t point;
    point_t dims;
  };


  template  <typename TO, typename FROM>
  box2d<TO> convert(const box2d<FROM>& v) {
    return box2d<TO>(static_cast<TO>(v.point[0]), static_cast<TO>(v.point[1]),
                     static_cast<TO>(v.dims[0]), static_cast<TO>(v.dims[1]));
  }

  using rect = box2d<int>;
}

#endif
