//
// Created by igor on 31/05/2021.
//

#ifndef NEUTRINO_UTILS_MP_ONE_OF_HH
#define NEUTRINO_UTILS_MP_ONE_OF_HH
#include <type_traits>

namespace neutrino::mp {
    /**
     * Check a parameter pack for at least one of type A
     * @tparam A Type to test
     * @tparam Ts Parameter pack
     */
    template <typename A, typename ...Ts>
    inline constexpr bool one_of_v = std::disjunction_v<std::is_same<A,Ts>...>;
}
#endif
