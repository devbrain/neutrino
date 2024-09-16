//
// Created by igor on 9/15/24.
//

#include <vector>
#include <array>
#include <iostream>
#include "cc_level_loader.hh"
#include "cc_map_cell.hh"
#include <neutrino/utils/random.hh>

#include "neutrino/ini/config_parser.hh"

static const std::pair<cc_tile_name_t, neutrino::sdl::area_type> BACKGROUNDS[] = {
    // intro
    {CC_TILE_NAME_STARS_1, {6, 1}},
    // finale
    {CC_TILE_NAME_STARS_1, {6, 1}},
    // main
    {CC_TILE_NAME_ROCKS_1, {2, 2}},
    {CC_TILE_NAME_NONE, {0, 0}},
    {CC_TILE_NAME_NONE, {0, 0}},
    {CC_TILE_NAME_NONE, {0, 0}},
    {CC_TILE_NAME_RED_PANEL_1, {2, 2}},
    {CC_TILE_NAME_NONE, {0, 0}},
    {CC_TILE_NAME_NONE, {0, 0}},
    {CC_TILE_NAME_NONE, {0, 0}},
    {CC_TILE_NAME_NONE, {0, 0}},
    {CC_TILE_NAME_NONE, {0, 0}},
    {CC_TILE_NAME_NONE, {0, 0}},
    {CC_TILE_NAME_NONE, {0, 0}},
    {CC_TILE_NAME_NONE, {0, 0}},
    {CC_TILE_NAME_NONE, {0, 0}},
    {CC_TILE_NAME_NONE, {0, 0}},
    {CC_TILE_NAME_NONE, {0, 0}},
    {CC_TILE_NAME_NONE, {0, 0}},
  };




static constexpr std::array <cc_tile_name_t, 21> STARS = {
    // The sprite with the bright star (3) seems to be less common...
    CC_TILE_NAME_STARS_1, CC_TILE_NAME_STARS_1, CC_TILE_NAME_STARS_1, CC_TILE_NAME_STARS_1,
    CC_TILE_NAME_STARS_2,
    CC_TILE_NAME_STARS_2, CC_TILE_NAME_STARS_2, CC_TILE_NAME_STARS_2, CC_TILE_NAME_STARS_3,
    CC_TILE_NAME_STARS_4,
    CC_TILE_NAME_STARS_4, CC_TILE_NAME_STARS_4, CC_TILE_NAME_STARS_4, CC_TILE_NAME_STARS_5,
    CC_TILE_NAME_STARS_5,
    CC_TILE_NAME_STARS_5, CC_TILE_NAME_STARS_5, CC_TILE_NAME_STARS_6, CC_TILE_NAME_STARS_6,
    CC_TILE_NAME_STARS_6,
    CC_TILE_NAME_STARS_6,
};

static constexpr std::array <cc_tile_name_t, 5> HORIZON = {
    CC_TILE_NAME_HORIZON_LAMP,
    CC_TILE_NAME_HORIZON_1,
    CC_TILE_NAME_HORIZON_2,
    CC_TILE_NAME_HORIZON_3,
    CC_TILE_NAME_HORIZON_4,
};


// Different levels use different block colours
static constexpr std::array <cc_tile_name_t,18> BLOCK_COLORS = {
    // Intro 1-2
    CC_TILE_NAME_BLOCK_PEBBLE_NW,
    CC_TILE_NAME_BLOCK_PEBBLE_NW,
    // Main
    CC_TILE_NAME_BLOCK_PEBBLE_NW,
    // 1-8
    CC_TILE_NAME_BLOCK_BROWN_NW,
    CC_TILE_NAME_BLOCK_CYAN_NW,
    CC_TILE_NAME_BLOCK_PEBBLE_NW,
    CC_TILE_NAME_BLOCK_CYAN_NW,
    CC_TILE_NAME_BLOCK_CYAN_NW,
    CC_TILE_NAME_BLOCK_PEBBLE_NW,
    CC_TILE_NAME_BLOCK_PEBBLE_NW,
    CC_TILE_NAME_BLOCK_BLUE_NW,
    // 9-16
    CC_TILE_NAME_BLOCK_PEBBLE_NW,
    CC_TILE_NAME_BLOCK_PEBBLE_NW,
    CC_TILE_NAME_BLOCK_BLUE_NW,
    CC_TILE_NAME_BLOCK_BLUE_NW,
    CC_TILE_NAME_BLOCK_GREEN_NW,
    CC_TILE_NAME_BLOCK_PEBBLE_NW,
    CC_TILE_NAME_BLOCK_METAL_NW,
};

template <std::size_t N>
static cc_tile_name_t choose_random_of(const std::array<cc_tile_name_t, N>& arr) {
    return arr[neutrino::random::get_int<int>(0, N-1)];
}

raw_level_map parse_level(const cc_level_description& descr) {
    raw_level_map out(descr.level, descr.level_dims);
    std::cout << "LEVEL " << descr.level << std::endl;
    auto [level_bg, level_bg_size] = BACKGROUNDS[descr.level];

    bool is_stars_row = false;
    bool is_horizon_row = false;
    bool is_bumpable_platform = false;
    bool is_sign = false;
    bool is_wood_strut = false;
    bool is_wood_pillar = false;
    bool is_volcano = false;
    int volcano_sprite = -1;
    int entrance_level = 3;
    const auto& tile_ids = descr.tiles;
    auto width = descr.level_dims.w;
    const int first_block = BLOCK_COLORS[descr.level];
    for (std::size_t i = 0; i < tile_ids.size(); i++) {
        const auto x = i % width;
        if (x == 0) {
            is_stars_row = false;
            is_horizon_row = false;
            is_bumpable_platform = false;
            is_sign = false;
            is_wood_strut = false;
            is_wood_pillar = false;
            is_volcano = false;
            volcano_sprite = -1;
        }
        const auto y = i / width;
        const auto tile_id = tile_ids[i];
        cell tile;
        int bg = level_bg;
        if (is_stars_row) {
            bg = choose_random_of(STARS);
        } else if (is_horizon_row) {
            bg = choose_random_of(HORIZON);
        } else if (level_bg != CC_TILE_NAME_NONE) {
            // Normal background
            bg = level_bg + (((y + 1) % level_bg_size.h) * 4) + (x % level_bg_size.w);
        }
        // Decode tile ids from exe data
        int sprite = -1;
        int sprites_count = 1;
        unsigned int flags = 0;
        int tile_type = -1;

        std::cout.width(4);
        if (tile_id >= ' ' && tile_id < 128) {
            std::cout << (char)tile_id;
        } else {
            std::cout << tile_id;
        }
        if (x == width - 1) {
            std::cout << std::endl;
        }
        if (is_bumpable_platform) {
            switch (tile_id) {
                case 'd':
                case -103:
                    sprite = CC_TILE_NAME_BUMP_PLATFORM_RED_MID;
                    flags |= CC_TILE_FLAG_SOLID;
                    if (tile_ids[i] == -103) {
                        // TODO: add hidden crystal
                    }
                    break;
                case 'n':
                case -102:
                    sprite = CC_TILE_NAME_BUMP_PLATFORM_RED_MID;
                    flags |= CC_TILE_FLAG_SOLID;
                    if (tile_ids[i] == -102) {
                        tile.set_property(CC_HAS_HIDDEN_CRYSTAL, 1);
                    }
                    is_bumpable_platform = false;
                    break;
                default:
                    break;
            }
        } else if (is_sign) {
            switch (tile_id) {
                // [4n = winners drugs sign
                case '4':
                    sprite = CC_TILE_NAME_WINNERS_2;
                    flags |= CC_TILE_FLAG_SOLID_TOP;
                    break;
                case 'n':
                    sprite = CC_TILE_NAME_WINNERS_3;
                    flags |= CC_TILE_FLAG_SOLID_TOP;
                    is_sign = false;
                    break;
                // [m = mine sign
                case 'm':
                    sprite = CC_TILE_NAME_MINE_SIGN_2;
                    flags |= CC_TILE_FLAG_RENDER_IN_FRONT;
                    is_sign = false;
                    break;
                // [d = danger sign
                case 'd':
                    sprite = CC_TILE_NAME_DANGER_2;
                    flags |= CC_TILE_FLAG_SOLID_TOP;
                    is_sign = false;
                    break;
                default:
                    break;
            }
        } else if (is_wood_strut) {
            switch (tile_id) {
                case 'n':
                    sprite = CC_TILE_NAME_WOOD_STRUT_2;
                    is_wood_strut = false;
                    break;
                default:
                    break;
            }
        } else if (is_wood_pillar) {
            switch (tile_id) {
                case 'n':
                    sprite = CC_TILE_NAME_WOOD_PILLAR_2;
                    is_wood_pillar = false;
                    break;
                default:
                    break;
            }
        } else if (is_volcano) {
            sprite = volcano_sprite;
            volcano_sprite++;
            if (tile_ids[i + 1] != 'n') {
                is_volcano = false;
                volcano_sprite = -1;
            }
        } else {
            switch (tile_id) {
                case ' ':
                    break;
                // Blocks
                case 'r':
                    sprite = first_block; // NW
                    flags |= CC_TILE_FLAG_SOLID;
                    break;
                case 't':
                    sprite = first_block + 1; // N
                    flags |= CC_TILE_FLAG_SOLID;
                    break;
                case 'y':
                    sprite = first_block + 2; // NE
                    flags |= CC_TILE_FLAG_SOLID;
                    break;
                case '4':
                    sprite = first_block + 8; // W
                    flags |= CC_TILE_FLAG_SOLID;
                    break;
                case '5':
                    sprite = first_block + 9; // MID
                    flags |= CC_TILE_FLAG_SOLID;
                    break;
                case '6':
                    sprite = first_block + 10; // E
                    flags |= CC_TILE_FLAG_SOLID;
                    break;
                case 'f':
                    sprite = first_block + 4; // SW
                    flags |= CC_TILE_FLAG_SOLID;
                    break;
                case 'g':
                    sprite = first_block + 5; // S
                    flags |= CC_TILE_FLAG_SOLID;
                    break;
                case 'h':
                    sprite = first_block + 6; // SE
                    flags |= CC_TILE_FLAG_SOLID;
                    break;
                // Bumpable platforms
                case 'D':
                case -104:
                    // Keep adding bumpable platforms until we get an 'n'
                    sprite = static_cast <int>(CC_TILE_NAME_BUMP_PLATFORM_RED_L);
                    flags |= CC_TILE_FLAG_SOLID;
                    if (tile_id == -104) {
                        tile.set_property(CC_HAS_HIDDEN_CRYSTAL, 1);
                    }
                    is_bumpable_platform = true;
                    break;
                case 'd':
                    sprite = CC_TILE_NAME_BUMP_PLATFORM_RED_MID;
                    flags |= CC_TILE_FLAG_SOLID;
                    break;
                case 'H':
                    sprite = CC_TILE_NAME_PLATFORM_H_1;
                    tile_type = CC_CELL_PLATFORM_H;
                    sprites_count = 4;
                    break;
                case 'k':
                    sprite = CC_TILE_NAME_CONCRETE_V;
                    flags |= CC_TILE_FLAG_SOLID;
                    break;
                case 'K':
                    sprite = CC_TILE_NAME_CONCRETE;
                    flags |= CC_TILE_FLAG_SOLID;
                    break;
                case 'l':
                    sprite = CC_TILE_NAME_CONCRETE_X;
                    flags |= CC_TILE_FLAG_SOLID;
                    break;
                case 'L':
                    sprite = CC_TILE_NAME_CONCRETE_H;
                    flags |= CC_TILE_FLAG_SOLID;
                    break;
                case 'm':
                    tile_type = CC_CELL_EARTH;
                    break;
                case 'n':
                    // TODO: volcano spawn point?
                    sprite = static_cast <int>(CC_TILE_NAME_VOLCANO_EJECTA_L_1);
                    sprites_count = 4;
                    flags |= CC_TILE_FLAG_ANIMATED;
                    break;
                case 'N':
                    tile_type = CC_CELL_MOON;
                    break;
                case 'u':
                    // TODO: volcano spawn point?
                    sprite = CC_TILE_NAME_VOLCANO_EJECTA_R_1;
                    sprites_count = 4;
                    flags |= CC_TILE_FLAG_ANIMATED;
                    tile_type = CC_VULCANO_SPAWN;
                    break;
                case 'V':
                    sprite = CC_TILE_NAME_PLATFORM_V_1;
                    tile_type = CC_CELL_PLATFORM_V;
                    sprites_count = 4;
                    // this->moving_platforms.push_back({
                    //     geometry::Position{x * 16, y * 16},
                    //     false,
                    // });
                    break;
                case 'x':
                    // TODO: remember completion state
                    // Everything is under construction...
                    sprite = CC_TILE_NAME_ENTRY_1;
                    flags |= CC_TILE_FLAG_RENDER_IN_FRONT;
                    tile_type = CC_CELL_DOOR;
                    tile.set_property(CC_ENTRANCE_TO, entrance_level);
                    entrance_level++;
                    break;
                case 'Y':
                    tile_type = CC_CELL_PLAYER_START;
                    break;
                case 'z':
                    if (is_horizon_row || (x == 0 && tile_ids[i + 1] == 'Z')) {
                        // Random horizon tile
                        bg = choose_random_of(HORIZON);
                        is_horizon_row = true;
                    } else {
                        // Random star tile
                        bg = choose_random_of(STARS);
                        is_stars_row = true;
                    }
                    break;
                case 'Z':
                    break;
                case '[':
                    switch (tile_ids[i + 1]) {
                        // [4n = winners drugs sign
                        case '4':
                            sprite = CC_TILE_NAME_WINNERS_1;
                            flags |= CC_TILE_FLAG_SOLID_TOP;
                            is_sign = true;
                            break;
                        // [m = mine sign
                        case 'm':
                            sprite = CC_TILE_NAME_MINE_SIGN_1;
                            flags |= CC_TILE_FLAG_RENDER_IN_FRONT;
                            is_sign = true;
                            break;
                        // [d = danger sign
                        case 'd':
                            sprite = CC_TILE_NAME_DANGER_1;
                            flags |= CC_TILE_FLAG_SOLID_TOP;
                            is_sign = true;
                            break;
                        default:
                            break;
                    }
                    break;
                case -6:
                    sprite = CC_TILE_NAME_BARREL_CRACKED;
                    flags |= CC_TILE_FLAG_SOLID_TOP;
                    break;
                case -7:
                    sprite = CC_TILE_NAME_BARREL;
                    flags |= CC_TILE_FLAG_SOLID_TOP;
                    break;
                case -16:
                    if (tile_ids[i + 1] == 'n') {
                        // Wood struts
                        sprite = CC_TILE_NAME_WOOD_STRUT_1;
                        is_wood_strut = true;
                    }
                    break;
                case -43:
                    // TODO: animated
                    sprite = CC_TILE_NAME_TORCH_1;
                    sprites_count = 4;
                    flags |= CC_TILE_FLAG_ANIMATED;
                    tile_type = CC_CELL_TORCH;
                    break;
                case -58:
                    sprite = CC_TILE_NAME_SIGN_DOWN;
                    break;
                case -77:
                    if (tile_ids[i + 1] == 'n') {
                        // Wood pillar
                        sprite = CC_TILE_NAME_WOOD_PILLAR_1;
                        is_wood_pillar = true;
                    }
                    break;
                case -78:
                    sprite = CC_TILE_NAME_WOOD_V;
                    flags |= CC_TILE_FLAG_SOLID;
                    break;
                case -79:
                    sprite = CC_TILE_NAME_WOOD_H;
                    flags |= CC_TILE_FLAG_SOLID;
                    break;
                case -113:
                    if (tile_ids[i + 1] == 'n') {
                        // -113 nnn = bottom of volcano
                        sprite = CC_TILE_NAME_VOLCANO_BOTTOM_1;
                        is_volcano = true;
                        volcano_sprite = sprite + 1;
                    }
                    break;
                case -114:
                    if (tile_ids[i + 1] == 'n') {
                        // -114 n = top of volcano
                        sprite = CC_TILE_NAME_VOLCANO_TOP_1;
                        is_volcano = true;
                        volcano_sprite = sprite + 1;
                    }
                    break;
                default:
                    break;
            }
        }
        if (sprite == -1 && tile_type == -1) {
            tile.set_property(CC_IS_INVALID, tile_ids[i]);
            tile.set_property(CC_INVALID_POS_X, static_cast<int>(x));
            tile.set_property(CC_INVALID_POS_Y, static_cast<int>(y));
        } else {
            tile.tile_name_id = sprite;
            if (sprites_count > 1) {
                tile.set_property(CC_SPRITES_COUNT, sprites_count);
            }
            if (tile_type != -1) {
                tile.set_property(CC_CELL_TYPE, tile_type);
            }
            tile.flags = flags;
        }
        out.cells.push_back(tile);
        out.backround.push_back(bg);
    }
    return out;
}
