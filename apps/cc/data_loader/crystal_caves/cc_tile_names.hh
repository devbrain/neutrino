//
// Created by igor on 8/27/24.
//

#ifndef CC_TILE_NAMES_HH
#define CC_TILE_NAMES_HH

#include <tuple>

struct cc_tile_names {
    std::tuple<int, int> get_range(int id);
};

#endif
