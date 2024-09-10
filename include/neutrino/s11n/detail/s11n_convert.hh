//
// Created by igor on 9/9/24.
//

#ifndef NEUTRINO_S11N_CONVERT_HH
#define NEUTRINO_S11N_CONVERT_HH

#include <type_traits>

namespace neutrino::s11n {
    namespace detail {
        template <class T, std::size_t = sizeof(T)>
        std::true_type is_complete_impl(T *);

        std::false_type is_complete_impl(...);

        template <class T>
        constexpr bool is_complete_v = std::is_same_v<std::true_type, decltype(is_complete_impl(std::declval<T*>()))>;

    }

    template <class T>
    struct s11n_converter;

}
#endif
