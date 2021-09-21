//
// Created by igor on 17/07/2021.
//

#ifndef NEUTRINO_TILED_TILE_TYPES_HH
#define NEUTRINO_TILED_TILE_TYPES_HH

#include <cstddef>
#include <neutrino/utils/strong_type.hpp>
#include <neutrino/utils/macros.hh>

#define d_DECLARE_TILED_TYPE(TYPE, NAME)                    \
    namespace detail    {                                   \
        struct PPCAT(NAME,_){};                             \
    }                                                       \
    using NAME = strong::type<TYPE, detail::PPCAT(NAME,_)>

#define d_DECLARE_TILED_TYPE_ORDERED(TYPE, NAME)            \
    namespace detail    {                                   \
        struct PPCAT(NAME,_){};                             \
    }                                                       \
    using NAME = strong::type<TYPE, detail::PPCAT(NAME,_), strong::ordered>

namespace neutrino::tiled {
  d_DECLARE_TILED_TYPE_ORDERED(std::size_t, texture_id_t);
  d_DECLARE_TILED_TYPE_ORDERED(std::size_t, layer_id_t);
  d_DECLARE_TILED_TYPE(std::size_t, tile_id_t);

}

#endif
