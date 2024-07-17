//
// Created by igor on 7/11/24.
//

#ifndef  CC_DECODE_HH
#define  CC_DECODE_HH

#include <tuple>
#include <vector>
#include "data_loader/raw_map.hh"
#include "data_loader/map_tile.hh"

std::tuple<bg_map_t, fg_map_t> cc_decode(raw_map m);

#endif
