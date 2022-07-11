//
// Created by igor on 08/05/2022.
//

#ifndef INCLUDE_NEUTRINO_ASSETS_TILES_TYPES_HH
#define INCLUDE_NEUTRINO_ASSETS_TILES_TYPES_HH

#include <limits>
#include <cstdint>
#include <neutrino/utils/strong_type.hpp>
#include <neutrino/assets/image/color.hh>

namespace neutrino::assets {

  using atlas_id_t = strong::type<std::size_t, struct atlas_id_tag, strong::ordered, strong::equality>;
  using cell_id_t  = strong::type<std::size_t, struct tile_id_tag, strong::ordered, strong::equality>;
  using animation_seq_id_t = strong::type<uint16_t, struct animation_seq_tag, strong::ordered, strong::equality>;
  using animation_state_id_t = strong::type<uint16_t, struct animation_state_tag, strong::ordered, strong::equality>;

  namespace detail {
    template <typename T>
    struct strong_traits {
      static constexpr bool is_strong = false;
    };

    template <typename T, typename Tag, typename ... Args>
    struct strong_traits<strong::type <T, Tag, Args...>> {
      static constexpr bool is_strong = true;
      using type = T;
    };
  }

  template <typename T>
  constexpr T make_invalid() {
    if constexpr(detail::strong_traits<T>::is_strong) {
      using type = typename detail::strong_traits<T>::type;
      return T{std::numeric_limits<type>::max()};
    } else {
      return std::numeric_limits<T>::max ();
    }
  }

  template <typename T>
  [[nodiscard]] bool is_invalid(const T& x) {
      return x == make_invalid<T> ();
  }

  struct rotation_info {
    rotation_info ()
    : hflip(false), vflip(false), degree(0) {

    }

    rotation_info(bool h, bool v, int d)
        : hflip(h), vflip(v), degree(d) {
    }

    [[nodiscard]] bool empty() const {
      return !hflip && !vflip && (degree == 0);
    }

    bool hflip;
    bool vflip;
    int  degree; // clockwise
  };
}

#endif //INCLUDE_NEUTRINO_ASSETS_TILES_TYPES_HH
