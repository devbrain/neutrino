//
// Created by igor on 7/12/24.
//

#ifndef  SA_DECODE_HH
#define  SA_DECODE_HH

#include <tuple>
#include <vector>
#include "raw_map.hh"
#include "map_tile.hh"

std::tuple<bg_map_t, fg_map_t> sa_decode(raw_map m, bool is_world);

#endif
