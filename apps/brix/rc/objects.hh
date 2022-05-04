//
// Created by igor on 27/09/2020.
//

#ifndef NEUTRINO_OBJECTS_HH
#define NEUTRINO_OBJECTS_HH

#include <array>

#include "rc/game_assets.hh"
#include "rc/map.hh"

enum rc_object_t
{
    eRC_OBJ_EMPTY,
    eRC_OBJ_GEM,
    eRC_OBJ_BRICK_BORDER,
    eRC_OBJ_DIGIT,
    eRC_OBJ_BORDER,
    eRC_OBJ_PORTAL,
    eRC_OBJ_GUN,
    eRC_OBJ_SAND,
    eRC_OBJ_WATER,
    eRC_OBJ_CLOCK,
    eRC_OBJ_BULLET,
    eRC_OBJ_PICTURE,
    eRC_OBJ_CURSOR,

    eRC_OBJ_DESTRUCTABLE_BRICK,
    eRC_OBJ_DESTRUCTABLE_BRICK_ANIMATION,
    eRC_OBJ_EXPLOSION_ANIMATION,
    eRC_OBJ_PORTAL_ANIMATION,
    eRC_OBJ_CLOCK_ANIMATION
};

enum class glyph_type_t : int
{
    WHITE = 0,
    GREY = 1,
    ICE = 2,
    FIRE = 3,
    SWAMP = 4,
    SAND = 5
};

enum tile_t
{
    eTILE_EMPTY,

    eTILE_OBJ1,
    eTILE_OBJ2,
    eTILE_OBJ3,
    eTILE_OBJ4,
    eTILE_OBJ5,
    eTILE_OBJ6,
    eTILE_OBJ7,
    eTILE_OBJ8,

    /* brick borders */
    eTILE_BRICK1,
    eTILE_BRICK2,
    eTILE_BRICK3,
    eTILE_BRICK4,
    eTILE_BRICK5,
    eTILE_BRICK6,
    eTILE_BRICK7,
    eTILE_BRICK8,
    eTILE_BRICK9,

    /* digits */
    eTILE_DIGIT_RED_0,
    eTILE_DIGIT_RED_1,
    eTILE_DIGIT_RED_2,
    eTILE_DIGIT_RED_3,
    eTILE_DIGIT_RED_4,
    eTILE_DIGIT_RED_5,
    eTILE_DIGIT_RED_6,
    eTILE_DIGIT_RED_7,
    eTILE_DIGIT_RED_8,
    eTILE_DIGIT_RED_9,

    eTILE_DIGIT_BLUE_0,
    eTILE_DIGIT_BLUE_1,
    eTILE_DIGIT_BLUE_2,
    eTILE_DIGIT_BLUE_3,
    eTILE_DIGIT_BLUE_4,
    eTILE_DIGIT_BLUE_5,
    eTILE_DIGIT_BLUE_6,
    eTILE_DIGIT_BLUE_7,
    eTILE_DIGIT_BLUE_8,
    eTILE_DIGIT_BLUE_9,

    /* borders */
    eTILE_BORDER_UL,
    eTILE_BORDER_HU,
    eTILE_BORDER_HD,
    eTILE_BORDER_VL,
    eTILE_BORDER_VR,
    eTILE_BORDER_UR,
    eTILE_BORDER_DL,
    eTILE_BORDER_DR,

    /* special bricks */
    eTILE_DESTRUCTIBLE_BRICK,
    eTILE_PORTAL_A_1,
    eTILE_PORTAL_A_2,
    eTILE_PORTAL_A_3,
    eTILE_PORTAL_A_4,
    eTILE_PORTAL_A_5,
    eTILE_PORTAL_B_1,
    eTILE_PORTAL_B_2,
    eTILE_PORTAL_B_3,
    eTILE_PORTAL_B_4,
    eTILE_PORTAL_B_5,
    eTILE_SAND,
    eTILE_WATER,

    /* guns */
    eTILE_GUN_L,
    eTILE_GUN_R,
    eTILE_GUN_U,
    eTILE_GUN_D,

    /* bullets */
    eTILE_BULLET_1,
    eTILE_BULLET_2,

    eTILE_CLOCK,

    /* effects */
    eTILE_GEM_EXPLOSION,
    eTILE_BRICK_DESTRUCTION,
    eTILE_PORTAL_ANIMATION,
    eTILE_CLOCK_ANIMATION,

    /* pictures */
    eTILE_LOGO,
    eTILE_TITLE1,
    eTILE_PIC1,
    eTILE_PIC2,
    eTILE_PIC3,
    eTILE_PIC4,
    eTILE_PIC5,
    eTILE_PIC6,
    eTILE_PIC7,
    eTILE_PIC8,
    eTILE_PIC9,
    eTILE_PIC10,

    eTILE_MAX
};

static constexpr std::array<tile_t, 10> blue_digits {
        eTILE_DIGIT_BLUE_0, eTILE_DIGIT_BLUE_1, eTILE_DIGIT_BLUE_2,
        eTILE_DIGIT_BLUE_3, eTILE_DIGIT_BLUE_4, eTILE_DIGIT_BLUE_5,
        eTILE_DIGIT_BLUE_6, eTILE_DIGIT_BLUE_7, eTILE_DIGIT_BLUE_8,
        eTILE_DIGIT_BLUE_9
};

static constexpr std::array<tile_t, 10> red_digits {
        eTILE_DIGIT_RED_0, eTILE_DIGIT_RED_1, eTILE_DIGIT_RED_2,
        eTILE_DIGIT_RED_3, eTILE_DIGIT_RED_4, eTILE_DIGIT_RED_5,
        eTILE_DIGIT_RED_6, eTILE_DIGIT_RED_7, eTILE_DIGIT_RED_8,
        eTILE_DIGIT_RED_9
};

static constexpr std::array<tile_t, 8> gems {
        eTILE_OBJ1, eTILE_OBJ2, eTILE_OBJ3, eTILE_OBJ4,
        eTILE_OBJ5, eTILE_OBJ6, eTILE_OBJ7, eTILE_OBJ8
};

static constexpr int tile_width = 16;
static constexpr int tile_height = 16;

class rc_object
{
public:


    static void init(resource_loader& loader, graphic_assets_builder& g_assets_builder);
    static const rc_object& get(tile_t tile);

    [[nodiscard]] rc_object_t type () const noexcept;
    [[nodiscard]] tile_id_t tile_id() const noexcept;
    [[nodiscard]] asset_id_t asset_id() const noexcept;

    const std::vector<tile_id_t>& frames() const noexcept;

    template<typename Func>
    static void map_text(glyph_type_t glyph, const std::string& text, Func&& f)
    {
        for (const auto ch : text)
        {
            f(s_glyphs[(int)glyph][ch - ' ']);
        }
    }

    static const level_map& get_map(std::size_t lev_num)
    {
        return s_maps [lev_num];
    }

    static std::size_t maps_size()
    {
        return s_maps.size();
    }
private:
    rc_object() = default;
    rc_object(asset_id_t asset);
    rc_object(rc_object_t type, tile_id_t tile);
    rc_object(rc_object_t type, const std::vector<tile_id_t>& frames);
private:
    rc_object_t m_type;
    tile_id_t m_tile_id;
    asset_id_t m_asset_id;
    std::vector<tile_id_t> m_frames;

    static std::vector<std::vector<tile_id_t>> s_glyphs;
    static std::vector<level_map> s_maps;
    static rc_object s_objects[eTILE_MAX];
};

#endif //NEUTRINO_OBJECTS_HH
