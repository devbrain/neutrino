//
// Created by igor on 7/11/24.
//

#include "data_loader/crystal_caves/cc_decode.hh"
#include "data_loader/crystal_caves/cc_tiles_mapping.hh"

/// This is the largest valid tile code in the background layer.
#define CC_MAX_VALID_TILECODE   0xFE

/// Create a tile number from a tileset number and an index into the tileset.
// #define MAKE_TILE(tileset, tile) (((tileset) << 8) | (tile))
#define MAKE_TILE(tileset, tile) (50*tileset + tile)

/// Tile code that means "next tile"
#define CCT_NEXT  0x6E

/// Tile code that means "no tile here"
#define CCT_EMPTY 0x20

static constexpr unsigned int block_tile = MAKE_TILE(21, 32); // solid brown
//unsigned int ibeam_tile = MAKE_TILE(19,  3); // blue
static constexpr unsigned int ibeam_tile = MAKE_TILE(19, 6); // red
static constexpr unsigned int underscore_tile = MAKE_TILE(19, 0); // blue

static void insert_tile(bg_map_t& m, int x, int y, int dx, int dy, int val, int flags) {
	auto posx = x + dx;
	auto posy = y + dy;
	int code = val;
	if (IS_IBEAM(val)) {
		code = CCT_IBEAM(ibeam_tile, (val));
	} else if (IS_BLOCK(val)) {
		code = CCT_BLOCK(block_tile, (val));
	} else if ((val) == CCT_USCORE) {
		code = underscore_tile;
	}
	m.emplace_back(posx, posy, code, flags);
}

#define SET_NEXT_TILE(dx, dy, val)									\
if (																\
	(val != ___________)											\
	&& (x + (dx) < w)												\
	&& (y + (dy) < h)												\
	&& (map.get_relative(bg, (dx), (dy)) == CCT_NEXT)				\
	) {																\
		insert_tile(bg_map, x, y, (dx), (dy), val, CCTF_MV_NONE);	\
		map.get_relative(bg, (dx), (dy)) = CCT_EMPTY;				\
	}

std::tuple <bg_map_t, fg_map_t> cc_decode(raw_map map) {
	bg_map_t bg_map;
	fg_map_t fg_map;

	auto bg = map.begin();
	const auto h = map.get_height();
	const auto w = map.get_width();
	for (unsigned int y = 0; y < h; y++) {
		for (unsigned int x = 0; x < w; x++, bg++) {
			if (*bg == CCT_EMPTY) {
				continue;
			}
			bool matched = false;
			// Check vines first
			for (auto& m : tileMapVine) {
				if (map.get_relative(bg, 0, 0) == m.code) {
					matched = true;
					if (map.get_relative(bg, 0, 1) == m.code) {
						// The vine continue on the row below, use a mid-tile
						insert_tile(bg_map, x, y, 0, 0, m.tileIndexMid, m.flags);
					} else {
						// The vine stops here, use an end-tile
						insert_tile(bg_map, x, y, 0, 0, m.tileIndexEnd, m.flags);
					}
					// Follow the vine up if need be
					for (int y2 = 1; y2 <= (signed)y; y2++) {
						if (map.get_relative(bg, 0, -y2) == CCT_NEXT) {
							insert_tile(bg_map, x, y, 0, -y2, m.tileIndexMid, CCTF_MV_NONE);
						} else {
							break; // vine stopped
						}
					}
					break;
				}
			}
			if (matched) continue;
			// Then check signs
			for (auto& m : tileMapSign) {
				if ((map.get_relative(bg, 0, 0) == m.code1) && (map.get_relative(bg, 1, 0) == m.code2)) {
					matched = true;
					insert_tile(bg_map, x, y, 0, 0, m.tileIndexBG[0], m.flags);
					insert_tile(bg_map, x, y, 1, 0, m.tileIndexBG[1], CCTF_MV_NONE);
					SET_NEXT_TILE(2, 0, m.tileIndexBG[2]);
					SET_NEXT_TILE(3, 0, m.tileIndexBG[3]);
					SET_NEXT_TILE(0, 1, m.tileIndexBG[4]);
					SET_NEXT_TILE(1, 1, m.tileIndexBG[5]);
					SET_NEXT_TILE(2, 1, m.tileIndexBG[6]);
					SET_NEXT_TILE(3, 1, m.tileIndexBG[7]);
					SET_NEXT_TILE(0, 2, m.tileIndexBG[8]);
					SET_NEXT_TILE(1, 2, m.tileIndexBG[9]);
					SET_NEXT_TILE(2, 2, m.tileIndexBG[10]);
					SET_NEXT_TILE(3, 2, m.tileIndexBG[11]);
					SET_NEXT_TILE(0, 3, m.tileIndexBG[12]);
					SET_NEXT_TILE(1, 3, m.tileIndexBG[13]);
					SET_NEXT_TILE(2, 3, m.tileIndexBG[14]);
					SET_NEXT_TILE(3, 3, m.tileIndexBG[15]);
					// All signs are at least two cells wide
					bg++;
					x++;
					break;
				}
			}
			if (matched) continue;
			// Lastly check the normal tiles
			for (auto & m : tileMap) {
					if (*bg == m.code) {
					matched = true;
					insert_tile(bg_map, x, y, 0, 0, m.tileIndexBG[0], m.flags);
					SET_NEXT_TILE(1, 0, m.tileIndexBG[1]);
					SET_NEXT_TILE(0, 1, m.tileIndexBG[2]);
					SET_NEXT_TILE(1, 1, m.tileIndexBG[3]);
					if (m.tileIndexFG != ___________) {
						fg_map.emplace_back(x, y, m.tileIndexFG, 0);
					}
					break;
				}
			}
			if (matched) continue;
			for (auto & m : tileMap4x1) {
					if (*bg == m.code) {
					matched = true;
					insert_tile(bg_map, x, y, 0, 0, m.tileIndexBG[0], m.flags);
					SET_NEXT_TILE(1, 0, m.tileIndexBG[1]);
					SET_NEXT_TILE(2, 0, m.tileIndexBG[2]);
					SET_NEXT_TILE(3, 0, m.tileIndexBG[3]);
					break;
				}
			}
			if (matched) continue;
		}
	}

	return {bg_map, fg_map};
}
