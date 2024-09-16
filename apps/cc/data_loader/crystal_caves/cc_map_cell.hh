//
// Created by igor on 9/16/24.
//

#ifndef CC_MAP_CELL_HH
#define CC_MAP_CELL_HH

enum cc_tile_flags : unsigned int {
    CC_TILE_FLAG_NONE            = 0,
    CC_TILE_FLAG_SOLID           = 1 << 0,
    CC_TILE_FLAG_SOLID_TOP       = 1 << 1,
    CC_TILE_FLAG_DAMAGE          = 1 << 2,
    CC_TILE_FLAG_DEATH           = 1 << 3,
    CC_TILE_FLAG_ANIMATED        = 1 << 4,
    CC_TILE_FLAG_RENDER_IN_FRONT = 1 << 5
};

enum cc_cell_props : int {
    CC_CELL_TYPE,
    CC_HAS_HIDDEN_CRYSTAL,
    CC_ENTRANCE_TO,
    CC_IS_INVALID,
    CC_INVALID_POS_X,
    CC_INVALID_POS_Y,
    CC_SPRITES_COUNT,

};

enum cc_cell_type {
    CC_CELL_PLAYER_START,
    CC_CELL_PLATFORM_H,
    CC_CELL_PLATFORM_V,
    CC_CELL_DOOR,
    CC_CELL_MOON,
    CC_CELL_EARTH,
    CC_VULCANO_SPAWN,
    CC_CELL_TORCH
};

#endif
