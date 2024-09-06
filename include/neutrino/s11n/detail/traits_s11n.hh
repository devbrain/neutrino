//
// Created by igor on 9/6/24.
//

#ifndef NEUTRINO_S11N_DETAIL_TRAITS_S11N_HH
#define NEUTRINO_S11N_DETAIL_TRAITS_S11N_HH

#include <map>
#include <list>
#include <vector>
#include <string>
#include <type_traits>

namespace neutrino::detail {

    template<typename T>
    struct is_simple {
        static constexpr bool value = std::is_arithmetic_v <T>;
    };

    template<>
    struct is_simple <std::string> {
        static constexpr bool value = true;
    };

    template<typename T>
    struct is_seq {
        static constexpr bool value = false;
    };

    template<typename T>
    struct is_seq <std::list <T>> {
        static constexpr bool value = true;
        static constexpr bool simple_type = is_simple<T>::value;
        using inner = T;
    };

    template<typename T>
    struct is_seq <std::vector <T>> {
        static constexpr bool value = true;
        static constexpr bool simple_type = is_simple<T>::value;
        using inner = T;
    };

    template<typename T, std::size_t N>
    struct is_seq <std::array <T, N>> {
        static constexpr bool value = true;
        static constexpr bool simple_type = is_simple<T>::value;
        using inner = T;
    };

    template<typename T>
    struct is_map {
        static constexpr bool value = false;
    };

    template<typename K, typename V>
    struct is_map <std::map <K, V>> {
        static constexpr bool value = true;
        using inner = V;
    };


}

#endif
