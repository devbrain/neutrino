//
// Created by igor on 02/06/2021.
//

#ifndef NEUTRINO_UTILS_MP_CONSTEXPR_FOR_HH
#define NEUTRINO_UTILS_MP_CONSTEXPR_FOR_HH

#include <type_traits>

namespace neutrino::mp {
  template <auto Start, auto End, auto Inc, class F>
  constexpr void constexpr_for (F&& f) {
    if constexpr (Start < End) {
      f (std::integral_constant<decltype (Start), Start> ());
      constexpr_for<Start + Inc, End, Inc> (f);
    }
  }
}

#endif
