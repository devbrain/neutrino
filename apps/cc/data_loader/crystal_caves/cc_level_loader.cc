//
// Created by igor on 9/15/24.
//

#include <vector>
#include <array>
#include "cc_level_loader.hh"
#include "cc_map_cell.hh"
#include <neutrino/utils/random.hh>

#include "neutrino/ini/config_parser.hh"

static const std::pair <cc_tile_name_t, neutrino::sdl::area_type> BACKGROUNDS[] = {
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
static constexpr std::array <cc_tile_name_t, 18> BLOCK_COLORS = {
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

template<std::size_t N>
static cc_tile_name_t choose_random_of(const std::array <cc_tile_name_t, N>& arr) {
    return arr[neutrino::random::get_int <int>(0, N - 1)];
}

// ================================================================================================
namespace state {
    struct stars_row {
    };

    struct horizon_row {
    };

    struct bumpable_platform {
    };

    struct sign {
    };

    struct wood_strut {
    };

    struct wood_pilar {
    };

    struct parse_block {
    };

    struct volcano {
        volcano()
            : sprite(-1) {
        }

        explicit volcano(int s)
            : sprite(s) {
        }

        int sprite;
    };
}

using parser_state = std::variant <std::monostate, state::stars_row, state::horizon_row,
                                   state::bumpable_platform, state::sign, state::wood_pilar,
                                   state::wood_strut, state::volcano, state::parse_block>;

class parser_event {
    public:
        parser_event(const std::vector <int>& tiles, unsigned width, unsigned height, int first_block)
            : m_index(0),
              m_x(0),
              m_y(0),
              m_tile_id(tiles[0]),
              m_tiles(tiles),
              m_width(width),
              m_height(height),
              m_first_block(first_block),
              m_entrance_level(3) {
        }

        bool next() {
            if (m_index < m_tiles.size()) {
                m_tile_id = m_tiles[m_index];
                m_x = m_index % m_width;
                m_y = m_index / m_height;
                m_index++;
                return true;
            }
            return false;
        }

        int next_entrance_level() {
            return m_entrance_level++;
        }

        [[nodiscard]] unsigned int get_x() const {
            return m_x;
        }

        [[nodiscard]] unsigned int get_y() const {
            return m_y;
        }

        [[nodiscard]] int get_tile() const {
            return m_tile_id;
        }

        [[nodiscard]] int look(int delta) const {
            if (delta >= 0) {
                ENFORCE(m_index + delta < m_tiles.size());
                return m_tiles[m_index + delta];
            } else {
                ENFORCE(-delta < m_index);
                return m_tiles[m_index + delta];
            }
        }

        [[nodiscard]] int get_first_block() const {
            return m_first_block;
        }

    private:
        std::size_t m_index;
        unsigned int m_x;
        unsigned int m_y;
        int m_tile_id;
        const std::vector <int>& m_tiles;
        const unsigned m_width;
        const unsigned m_height;
        int m_first_block;
        int m_entrance_level;
};

struct parser_context {
    parser_context(const std::pair <cc_tile_name_t, neutrino::sdl::area_type>& lbg, const parser_event& ev)
        : sprite(-1),
          sprites_count(0),
          flags(0),
          tile_type(-1),
          bg(-1) {
        auto [level_bg, level_bg_size] = lbg;
        bg = level_bg + (((ev.get_y() + 1) % level_bg_size.h) * 4) + (ev.get_x() % level_bg_size.w);
    }

    int sprite;
    int sprites_count;
    unsigned int flags;
    int tile_type;
    cell tile;
    int bg;
};

static parser_state on_event(const state::bumpable_platform&, const parser_event& ev, parser_context& out) {
    auto tile = ev.get_tile();
    switch (tile) {
        case 'd':
        case -103:
            out.sprite = CC_TILE_NAME_BUMP_PLATFORM_RED_MID;
            out.flags |= CC_TILE_FLAG_SOLID;
            if (tile == -103) {
                out.tile.set_property(CC_HAS_HIDDEN_CRYSTAL, 1);
            }
            return state::bumpable_platform{};
        case 'n':
        case -102:
            out.sprite = CC_TILE_NAME_BUMP_PLATFORM_RED_MID;
            out.flags |= CC_TILE_FLAG_SOLID;
            if (tile == -102) {
                out.tile.set_property(CC_HAS_HIDDEN_CRYSTAL, 1);
            }
            return state::parse_block{};
        default:
            return {};
    }
}

static parser_state on_event(const state::stars_row&, const parser_event&, parser_context& out) {
    out.bg = choose_random_of(STARS);
    return state::parse_block{};
}

static parser_state on_event(const state::horizon_row&, const parser_event&, parser_context& out) {
    out.bg = choose_random_of(HORIZON);
    return state::parse_block{};
}

static parser_state on_event(const state::sign&, const parser_event& ev, parser_context& out) {
    auto tile_id = ev.get_tile();
    switch (tile_id) {
        // [4n = winners drugs sign
        case '4':
            out.sprite = CC_TILE_NAME_WINNERS_2;
            out.flags |= CC_TILE_FLAG_SOLID_TOP;
            return state::sign{};
        case 'n':
            out.sprite = CC_TILE_NAME_WINNERS_3;
            out.flags |= CC_TILE_FLAG_SOLID_TOP;
            return state::parse_block{};
        // [m = mine sign
        case 'm':
            out.sprite = CC_TILE_NAME_MINE_SIGN_2;
            out.flags |= CC_TILE_FLAG_RENDER_IN_FRONT;
            return state::parse_block{};
        // [d = danger sign
        case 'd':
            out.sprite = CC_TILE_NAME_DANGER_2;
            out.flags |= CC_TILE_FLAG_SOLID_TOP;
            return state::parse_block{};
        default:
            return {};
    }
}

static parser_state on_event(const state::wood_strut&, const parser_event& ev, parser_context& out) {
    auto tile_id = ev.get_tile();
    switch (tile_id) {
        case 'n':
            out.sprite = CC_TILE_NAME_WOOD_STRUT_2;
            return state::parse_block{};
        default:
            return {};
    }
}

static parser_state on_event(const state::wood_pilar&, const parser_event& ev, parser_context& out) {
    auto tile_id = ev.get_tile();
    switch (tile_id) {
        case 'n':
            out.sprite = CC_TILE_NAME_WOOD_PILLAR_2;
            return state::parse_block{};
        default:
            return {};
    }
}

static parser_state on_event(const state::volcano& v, const parser_event& ev, parser_context& out) {
    out.sprite = v.sprite;
    if (ev.look(1) != 'n') {
        return state::parse_block{};
    }
    return state::volcano(v.sprite + 1);
}

static parser_state on_event(const state::parse_block&, parser_event& ev, parser_context& out) {
    auto tile_id = ev.get_tile();
    auto first_block = ev.get_first_block();;

    switch (tile_id) {
        case ' ':
            return state::parse_block{};;
        // Blocks
        case 'r':
            out.sprite = first_block; // NW
            out.flags |= CC_TILE_FLAG_SOLID;
            return state::parse_block{};;
        case 't':
            out.sprite = first_block + 1; // N
            out.flags |= CC_TILE_FLAG_SOLID;
            return state::parse_block{};;
        case 'y':
            out.sprite = first_block + 2; // NE
            out.flags |= CC_TILE_FLAG_SOLID;
            return state::parse_block{};;
        case '4':
            out.sprite = first_block + 8; // W
            out.flags |= CC_TILE_FLAG_SOLID;
            return state::parse_block{};;
        case '5':
            out.sprite = first_block + 9; // MID
            out.flags |= CC_TILE_FLAG_SOLID;
            return state::parse_block{};;
        case '6':
            out.sprite = first_block + 10; // E
            out.flags |= CC_TILE_FLAG_SOLID;
            return state::parse_block{};;
        case 'f':
            out.sprite = first_block + 4; // SW
            out.flags |= CC_TILE_FLAG_SOLID;
            return state::parse_block{};;
        case 'g':
            out.sprite = first_block + 5; // S
            out.flags |= CC_TILE_FLAG_SOLID;
            return state::parse_block{};;
        case 'h':
            out.sprite = first_block + 6; // SE
            out.flags |= CC_TILE_FLAG_SOLID;
            return state::parse_block{};;
        // Bumpable platforms
        case 'D':
        case -104:
            // Keep adding bumpable platforms until we get an 'n'
            out.sprite = CC_TILE_NAME_BUMP_PLATFORM_RED_L;
            out.flags |= CC_TILE_FLAG_SOLID;
            if (tile_id == -104) {
                out.tile.set_property(CC_HAS_HIDDEN_CRYSTAL, 1);
            }
            return state::bumpable_platform{};
        case 'd':
            out.sprite = CC_TILE_NAME_BUMP_PLATFORM_RED_MID;
            out.flags |= CC_TILE_FLAG_SOLID;
            return state::parse_block{};;
        case 'H':
            out.sprite = CC_TILE_NAME_PLATFORM_H_1;
            out.tile_type = CC_CELL_PLATFORM_H;
            out.sprites_count = 4;
            return state::parse_block{};;
        case 'k':
            out.sprite = CC_TILE_NAME_CONCRETE_V;
            out.flags |= CC_TILE_FLAG_SOLID;
            return state::parse_block{};;
        case 'K':
            out.sprite = CC_TILE_NAME_CONCRETE;
            out.flags |= CC_TILE_FLAG_SOLID;
            return state::parse_block{};;
        case 'l':
            out.sprite = CC_TILE_NAME_CONCRETE_X;
            out.flags |= CC_TILE_FLAG_SOLID;
            return state::parse_block{};;
        case 'L':
            out.sprite = CC_TILE_NAME_CONCRETE_H;
            out.flags |= CC_TILE_FLAG_SOLID;
            return state::parse_block{};;
        case 'm':
            out.tile_type = CC_CELL_EARTH;
            return state::parse_block{};;
        case 'n':
            // TODO: volcano spawn point?
            out.sprite = static_cast <int>(CC_TILE_NAME_VOLCANO_EJECTA_L_1);
            out.sprites_count = 4;
            out.flags |= CC_TILE_FLAG_ANIMATED;
            return state::parse_block{};;
        case 'N':
            out.tile_type = CC_CELL_MOON;
            return state::parse_block{};;
        case 'u':
            // TODO: volcano spawn point?
            out.sprite = CC_TILE_NAME_VOLCANO_EJECTA_R_1;
            out.sprites_count = 4;
            out.flags |= CC_TILE_FLAG_ANIMATED;
            out.tile_type = CC_VULCANO_SPAWN;
            return state::parse_block{};;
        case 'V':
            out.sprite = CC_TILE_NAME_PLATFORM_V_1;
            out.tile_type = CC_CELL_PLATFORM_V;
            out.sprites_count = 4;
            return state::parse_block{};;
        case 'x':
            out.sprite = CC_TILE_NAME_ENTRY_1;
            out.flags |= CC_TILE_FLAG_RENDER_IN_FRONT;
            out.tile_type = CC_CELL_DOOR;
            out.tile.set_property(CC_ENTRANCE_TO, ev.next_entrance_level());
            return state::parse_block{};;
        case 'Y':
            out.tile_type = CC_CELL_PLAYER_START;
            return state::parse_block{};;
        case 'z':
            if (ev.get_x() == 0 && ev.look(1) == 'Z') {
                // Random horizon tile
                out.bg = choose_random_of(HORIZON);
                return state::horizon_row{};
            } else {
                // Random star tile
                out.bg = choose_random_of(STARS);
                return state::stars_row{};
            }
            break;
        case 'Z':
            break;
        case '[':
            switch (ev.look(1)) {
                // [4n = winners drugs sign
                case '4':
                    out.sprite = CC_TILE_NAME_WINNERS_1;
                    out.flags |= CC_TILE_FLAG_SOLID_TOP;
                    return state::sign{};
                // [m = mine sign
                case 'm':
                    out.sprite = CC_TILE_NAME_MINE_SIGN_1;
                    out.flags |= CC_TILE_FLAG_RENDER_IN_FRONT;
                    return state::sign{};
                // [d = danger sign
                case 'd':
                    out.sprite = CC_TILE_NAME_DANGER_1;
                    out.flags |= CC_TILE_FLAG_SOLID_TOP;
                    return state::sign{};
                default:
                    return {};
            }
            break;
        case -6:
            out.sprite = CC_TILE_NAME_BARREL_CRACKED;
            out.flags |= CC_TILE_FLAG_SOLID_TOP;
            return state::parse_block{};
        case -7:
            out.sprite = CC_TILE_NAME_BARREL;
            out.flags |= CC_TILE_FLAG_SOLID_TOP;
            return state::parse_block{};
        case -16:
            if (ev.look(1) == 'n') {
                // Wood struts
                out.sprite = CC_TILE_NAME_WOOD_STRUT_1;
                return state::wood_strut{};
            }
            break;
        case -43:
            // TODO: animated
            out.sprite = CC_TILE_NAME_TORCH_1;
            out.sprites_count = 4;
            out.flags |= CC_TILE_FLAG_ANIMATED;
            out.tile_type = CC_CELL_TORCH;
            return state::parse_block{};
        case -58:
            out.sprite = CC_TILE_NAME_SIGN_DOWN;
            return state::parse_block{};
        case -77:
            if (ev.look(1) == 'n') {
                // Wood pillar
                out.sprite = CC_TILE_NAME_WOOD_PILLAR_1;
                return state::wood_pilar{};
            }
            return state::parse_block{};
        case -78:
            out.sprite = CC_TILE_NAME_WOOD_V;
            out.flags |= CC_TILE_FLAG_SOLID;
            return state::parse_block{};
        case -79:
            out.sprite = CC_TILE_NAME_WOOD_H;
            out.flags |= CC_TILE_FLAG_SOLID;
            return state::parse_block{};
        case -113:
            if (ev.look(1) == 'n') {
                // -113 nnn = bottom of volcano
                out.sprite = CC_TILE_NAME_VOLCANO_BOTTOM_1;
                return state::volcano{out.sprite+1};
            }
            break;
        case -114:
            if (ev.look(1) == 'n') {
                // -114 n = top of volcano
                out.sprite = CC_TILE_NAME_VOLCANO_TOP_1;
                return state::volcano{out.sprite+1};
            }
            break;
        default:
            break;
    }
    return {};
}

// ================================================================================================
raw_level_map parse_level(const cc_level_description& descr) {
    raw_level_map out(descr.level, descr.level_dims);
    for (int tile : descr.tiles) {
        out.raw_map.push_back(tile);
    }

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

        if (is_bumpable_platform) {
            switch (tile_id) {
                case 'd':
                case -103:
                    sprite = CC_TILE_NAME_BUMP_PLATFORM_RED_MID;
                    flags |= CC_TILE_FLAG_SOLID;
                    if (tile_ids[i] == -103) {
                        tile.set_property(CC_HAS_HIDDEN_CRYSTAL, 1);
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
                    sprite = CC_TILE_NAME_BUMP_PLATFORM_RED_L;
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
            tile.set_property(CC_INVALID_POS_X, static_cast <int>(x));
            tile.set_property(CC_INVALID_POS_Y, static_cast <int>(y));
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
