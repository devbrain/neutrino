//
// Created by igor on 08/05/2022.
//

#ifndef INCLUDE_NEUTRINO_KERNEL_RC_TYPES_HH
#define INCLUDE_NEUTRINO_KERNEL_RC_TYPES_HH

#include <limits>
#include <neutrino/utils/strong_type.hpp>

namespace neutrino::kernel {
  using atlas_id_t = strong::type<std::size_t, struct atlas_id_tag, strong::ordered, strong::equality>;
  using cell_id_t  = strong::type<std::size_t, struct tile_id_tag, strong::ordered, strong::equality>;



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

  struct tile_id_t {
    tile_id_t()
    : atlas_id(make_invalid<atlas_id_t>()),
      cell_id(make_invalid<cell_id_t>()) {}

    tile_id_t(atlas_id_t a, const cell_id_t c)
    : atlas_id(a),
    cell_id(c) {}

    atlas_id_t atlas_id;
    cell_id_t  cell_id;

    [[nodiscard]] bool valid() const noexcept {
      return !is_invalid (atlas_id) && !is_invalid (cell_id);
    }

  };
}

#endif //INCLUDE_NEUTRINO_KERNEL_RC_TYPES_HH
