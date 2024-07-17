//
// Created by igor on 7/12/24.
//

#include <cstdlib>
#include "sa_decode.hh"
#include "sa_tiles_mapping.hh"
#define MAKE_TILE(tileset, tile) (50*tileset + tile)

std::tuple<bg_map_t, fg_map_t> sa_decode(raw_map map, bool is_world) {
	bg_map_t bg_map;
	fg_map_t fg_map;

	auto bg = map.data();
	const auto h = map.get_height();
	const auto w = map.get_width();

	auto mapHeight = h;
	unsigned int bgcode = strtod((char*)bg, nullptr);
	bg += w; // skip full first line (it only has the BG code)
	bg += w; // skip full second line (not displayed in the game)
	unsigned int bgtile;
	switch (bgcode) {
		case 667: bgtile = MAKE_TILE( 6, 16); break;
		case 695: bgtile = MAKE_TILE( 6, 44);  break;
		case 767: bgtile = MAKE_TILE( 8, 16);  break;
		case 771: bgtile = MAKE_TILE( 8, 20);  break;
		case 325: bgtile = MAKE_TILE( 9, 24); break;
		case 329: bgtile = MAKE_TILE( 9, 28);  break;
		case 333: bgtile = MAKE_TILE( 9, 32);  break;
		case 337: bgtile = MAKE_TILE( 9, 36);  break;
		case 341: bgtile = MAKE_TILE( 9, 40);  break;
		case 209: bgtile = MAKE_TILE(11,  8);  break;
		case 213: bgtile = MAKE_TILE(11, 12);  break;
		case 217: bgtile = MAKE_TILE(11, 16);  break;
		case 233: bgtile = MAKE_TILE(11, 32);  break;
		case 237: bgtile = MAKE_TILE(11, 36);  break;
		case 241: bgtile = MAKE_TILE(11, 40);  break;
		case 245: bgtile = MAKE_TILE(11, 44);  break;
		case 501: bgtile = MAKE_TILE( 1,  0);  break;
		default: bgtile = MAKE_TILE( 6, 16);  break;
	}

	const TILE_MAP *tm = is_world ? worldMap : tileMap;

	return {bg_map, fg_map};
}
