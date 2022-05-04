//
// Created by igor on 27/09/2020.
//
#include "objects.hh"

rc_object rc_object::s_objects[eTILE_MAX];
std::vector<std::vector<tile_id_t>> rc_object::s_glyphs;
std::vector<level_map> rc_object::s_maps;
// ================================================================================================
rc_object::rc_object(asset_id_t asset)
: m_type(eRC_OBJ_PICTURE),
  m_asset_id(asset)
{

}
// -----------------------------------------------------------------------------------------------
rc_object::rc_object(rc_object_t type, tile_id_t tile)
: m_type(type),
  m_tile_id(tile)
{

}
// -----------------------------------------------------------------------------------------------
rc_object::rc_object(rc_object_t type, const std::vector<tile_id_t>& frames)
: m_type(type),
  m_frames(frames)
{

}
// -----------------------------------------------------------------------------------------------
const rc_object& rc_object::get(tile_t tile)
{
    return s_objects[tile];
}
// -----------------------------------------------------------------------------------------------
rc_object_t rc_object::type () const noexcept
{
    return m_type;
}
// -----------------------------------------------------------------------------------------------
tile_id_t rc_object::tile_id() const noexcept
{
    return m_tile_id;
}
// -----------------------------------------------------------------------------------------------
asset_id_t rc_object::asset_id() const noexcept
{
    return m_asset_id;
}
// -----------------------------------------------------------------------------------------------
const std::vector<tile_id_t>& rc_object::frames() const noexcept
{
    return m_frames;
}
// -----------------------------------------------------------------------------------------------

// -----------------------------------------------------------------------------------------------
void rc_object::init(resource_loader& loader, graphic_assets_builder& g_assets_builder)
{
    game_assets ga(loader, g_assets_builder);
    std::swap(s_glyphs, ga.m_glyphs);
    std::swap(s_maps, ga.m_maps);

    auto first_tile = (std::size_t)ga.m_first_tile;
#define T(X) tile_id_t(first_tile + X)
    static std::vector<tile_id_t> gem_explosion     = { T(14), T(15), T(16), T(17), T(18)};
    static std::vector<tile_id_t> brick_destruction = { T(51), T(52), T(53), T(54), T(55), T(56), T(57), T(58), T(59)};
    static std::vector<tile_id_t> portal_ani        = { T(70), T(71), T(72), T(73), T(74), T(75), T(76), T(77), T(78), T(79)};
    static std::vector<tile_id_t> clock_ani         = { T(91), T(92), T(93), T(94), T(95), T(96), T(97), T(98), T(99), T(100)};

#define set_object_in_rep(T, OBJ) s_objects[T] = OBJ
#define create_empty(X) rc_object(eRC_OBJ_EMPTY, T(X))
    /* setup simple tiles */
    set_object_in_rep (eTILE_EMPTY, create_empty (0));
    /* setup gems */
#define create_gem(X) rc_object(eRC_OBJ_GEM, T(X))
    set_object_in_rep (eTILE_OBJ1 , create_gem   (1));
    set_object_in_rep (eTILE_OBJ2 , create_gem   (2));
    set_object_in_rep (eTILE_OBJ3 , create_gem   (3));
    set_object_in_rep (eTILE_OBJ4 , create_gem   (4));
    set_object_in_rep (eTILE_OBJ5 , create_gem   (5));
    set_object_in_rep (eTILE_OBJ6 , create_gem   (6));
    set_object_in_rep (eTILE_OBJ7 , create_gem   (7));
    set_object_in_rep (eTILE_OBJ8 , create_gem   (8));

    /* bricks */
#define create_brick_border(X) rc_object(eRC_OBJ_BRICK_BORDER, T(X))
    set_object_in_rep (eTILE_BRICK1 , create_brick_border (10));
    set_object_in_rep (eTILE_BRICK2 , create_brick_border (11));
    set_object_in_rep (eTILE_BRICK3 , create_brick_border (12));
    set_object_in_rep (eTILE_BRICK4 , create_brick_border (19));
    set_object_in_rep (eTILE_BRICK5 , create_brick_border (39));
    set_object_in_rep (eTILE_BRICK6 , create_brick_border (84));
    set_object_in_rep (eTILE_BRICK7 , create_brick_border (87));
    set_object_in_rep (eTILE_BRICK8 , create_brick_border (89));
    set_object_in_rep (eTILE_BRICK9 , create_brick_border (9));

    /* digits */
#define create_digit(X) rc_object(eRC_OBJ_DIGIT, T(X))
    set_object_in_rep (eTILE_DIGIT_RED_0 , create_digit (20));
    set_object_in_rep (eTILE_DIGIT_RED_1 , create_digit (21));
    set_object_in_rep (eTILE_DIGIT_RED_2 , create_digit (22));
    set_object_in_rep (eTILE_DIGIT_RED_3 , create_digit (23));
    set_object_in_rep (eTILE_DIGIT_RED_4 , create_digit (24));
    set_object_in_rep (eTILE_DIGIT_RED_5 , create_digit (25));
    set_object_in_rep (eTILE_DIGIT_RED_6 , create_digit (26));
    set_object_in_rep (eTILE_DIGIT_RED_7 , create_digit (27));
    set_object_in_rep (eTILE_DIGIT_RED_8 , create_digit (28));
    set_object_in_rep (eTILE_DIGIT_RED_9 , create_digit (29));

    set_object_in_rep (eTILE_DIGIT_BLUE_0 , create_digit (40));
    set_object_in_rep (eTILE_DIGIT_BLUE_1 , create_digit (41));
    set_object_in_rep (eTILE_DIGIT_BLUE_2 , create_digit (42));
    set_object_in_rep (eTILE_DIGIT_BLUE_3 , create_digit (43));
    set_object_in_rep (eTILE_DIGIT_BLUE_4 , create_digit (44));
    set_object_in_rep (eTILE_DIGIT_BLUE_5 , create_digit (45));
    set_object_in_rep (eTILE_DIGIT_BLUE_6 , create_digit (46));
    set_object_in_rep (eTILE_DIGIT_BLUE_7 , create_digit (47));
    set_object_in_rep (eTILE_DIGIT_BLUE_8 , create_digit (48));
    set_object_in_rep (eTILE_DIGIT_BLUE_9 , create_digit (49));

    /* borders */
#define create_border(X) rc_object(eRC_OBJ_BORDER, T(X))
    set_object_in_rep (eTILE_BORDER_UL , create_border (30));
    set_object_in_rep (eTILE_BORDER_VL , create_border (31));
    set_object_in_rep (eTILE_BORDER_DL , create_border (32));
    set_object_in_rep (eTILE_BORDER_DR , create_border (33));
    set_object_in_rep (eTILE_BORDER_UR , create_border (34));
    set_object_in_rep (eTILE_BORDER_VR , create_border (35));
    set_object_in_rep (eTILE_BORDER_HU , create_border (36));
    set_object_in_rep (eTILE_BORDER_HD , create_border (37));


    /* special bricks */
#define create_destructable_brick(X) rc_object(eRC_OBJ_DESTRUCTABLE_BRICK, T(X))
    set_object_in_rep (eTILE_DESTRUCTIBLE_BRICK , create_destructable_brick (13));
#define create_portal(X) rc_object(eRC_OBJ_PORTAL, T(X))
    set_object_in_rep (eTILE_PORTAL_A_1         , create_portal (60));
    set_object_in_rep (eTILE_PORTAL_A_2         , create_portal (61));
    set_object_in_rep (eTILE_PORTAL_A_3         , create_portal (62));
    set_object_in_rep (eTILE_PORTAL_A_4         , create_portal (63));
    set_object_in_rep (eTILE_PORTAL_A_5         , create_portal (64));

    set_object_in_rep (eTILE_PORTAL_B_1         , create_portal (65));
    set_object_in_rep (eTILE_PORTAL_B_2         , create_portal (66));
    set_object_in_rep (eTILE_PORTAL_B_3         , create_portal (67));
    set_object_in_rep (eTILE_PORTAL_B_4         , create_portal (68));
    set_object_in_rep (eTILE_PORTAL_B_5         , create_portal (69));
#define create_sand(X) rc_object(eRC_OBJ_SAND, T(X))
    set_object_in_rep (eTILE_SAND , create_sand (85));
#define create_water(X) rc_object(eRC_OBJ_WATER, T(X))
    set_object_in_rep (eTILE_WATER, create_water (88));
    /* guns */
#define create_gun(X) rc_object(eRC_OBJ_GUN, T(X))
    set_object_in_rep (eTILE_GUN_R, create_gun (80));
    set_object_in_rep (eTILE_GUN_L, create_gun (81));
    set_object_in_rep (eTILE_GUN_U, create_gun (82));
    set_object_in_rep (eTILE_GUN_D, create_gun (83));

#define create_bullet(X) rc_object(eRC_OBJ_BULLET, T(X))
    set_object_in_rep (eTILE_BULLET_1, create_bullet (38));
    set_object_in_rep (eTILE_BULLET_2, create_bullet (50));

#define create_clock(X) rc_object(eRC_OBJ_CLOCK, T(X))
    set_object_in_rep (eTILE_CLOCK, create_clock (90));

    /* effects */
    set_object_in_rep (eTILE_GEM_EXPLOSION    , rc_object (eRC_OBJ_EXPLOSION_ANIMATION, gem_explosion));
    set_object_in_rep (eTILE_BRICK_DESTRUCTION, rc_object (eRC_OBJ_DESTRUCTABLE_BRICK_ANIMATION, brick_destruction));
    set_object_in_rep (eTILE_PORTAL_ANIMATION , rc_object (eRC_OBJ_PORTAL_ANIMATION, portal_ani));
    set_object_in_rep (eTILE_CLOCK_ANIMATION  , rc_object (eRC_OBJ_CLOCK_ANIMATION, clock_ani));

    std::size_t first_backdrop = (std::size_t)ga.m_first_backdrop;
#define dP(X) asset_id_t(first_backdrop + X)
    /* pictures */
    set_object_in_rep (eTILE_LOGO  ,  rc_object (ga.m_logo));
    set_object_in_rep (eTILE_TITLE1,  rc_object (ga.m_title));
    set_object_in_rep (eTILE_PIC1  , rc_object (dP(0)));
    set_object_in_rep (eTILE_PIC2  , rc_object (dP(1)));
    set_object_in_rep (eTILE_PIC3  , rc_object (dP(2)));
    set_object_in_rep (eTILE_PIC4  , rc_object (dP(3)));
    set_object_in_rep (eTILE_PIC5  , rc_object (dP(4)));
    set_object_in_rep (eTILE_PIC6  , rc_object (dP(5)));
    set_object_in_rep (eTILE_PIC7  , rc_object (dP(6)));
    set_object_in_rep (eTILE_PIC8  , rc_object (dP(7)));
    set_object_in_rep (eTILE_PIC9  , rc_object (dP(8)));
    set_object_in_rep (eTILE_PIC10 , rc_object (dP(9)));

}