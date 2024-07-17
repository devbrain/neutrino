//
// Created by igor on 7/11/24.
//

#ifndef MAP_TILE_HH
#define MAP_TILE_HH

#include <vector>

struct map_tile {
	map_tile(unsigned posx_, int posy_, int code_, int type_)
		: posx(posx_),
		  posy(posy_),
		  code(code_),
		  type(type_) {
	}

	unsigned posx;
	unsigned posy;
	int code;
	int type;
};

using bg_map_t = std::vector<map_tile>;
using fg_map_t = std::vector<map_tile>;

#endif
