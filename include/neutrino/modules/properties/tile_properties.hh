//
// Created by igor on 9/12/24.
//

#ifndef TILE_PROPERTIES_HH
#define TILE_PROPERTIES_HH
namespace neutrino {
    enum tile_type {
        PLAYER_START_POINT,
        EXIT_FROM_LEVEL,
        BACKGROUND,
        PLATFROM
    };

    struct tile_properties {
        tile_type type;
    };
}
#endif
