//
// Created by igor on 7/21/24.
//

#ifndef  TILE_NAMES_HH
#define  TILE_NAMES_HH

#include <neutrino/modules/video/tile.hh>
#include <neutrino/modules/video/world/animation_sequence.hh>
#include <neutrino/modules/video/tile_font_spec.hh>
#include <neutrino/modules/video/texture_atlas.hh>

inline constexpr int TILE_W = 16;
inline constexpr int TILE_H = 16;

inline constexpr int HUD_TILE_W = 8;
inline constexpr int HUD_TILE_H = 8;

neutrino::tile create_tile(int sprite_id);

void setup_cc_names(neutrino::texture_atlas& atlas,
                    neutrino::texture_id_t cc_mini_tiles_1,
                    neutrino::texture_id_t cc_mini_tiles_2,
                    neutrino::texture_id_t cc_mini_tiles_3,
                    neutrino::texture_id_t main_tiles
);

#if !defined(INSTANTIATE_TILES)
#define dNAME_ACCESS extern
#else
#define dNAME_ACCESS
#endif

#define d_DECLARE_TILE(NAME) dNAME_ACCESS neutrino::tile NAME
#define d_DECLARE_ANI(NAME)  dNAME_ACCESS neutrino::tiled::animation_sequence NAME
#define d_DECLARE_FONT_SPEC(NAME) dNAME_ACCESS neutrino::tile_font_spec NAME

#if !defined(INSTANTIATE_TILES)
#define d_DECLARE_TEXTURE(NAME) dNAME_ACCESS neutrino::texture_id_t NAME
#else
#define d_DECLARE_TEXTURE(NAME) dNAME_ACCESS neutrino::texture_id_t NAME{0}
#endif

// DIALOG TILES
d_DECLARE_TILE(TILE_DIALOG_UP_LEFT);
d_DECLARE_TILE(TILE_DIALOG_UP);
d_DECLARE_TILE(TILE_DIALOG_UP_RIGHT);
d_DECLARE_TILE(TILE_DIALOG_LEFT);
d_DECLARE_TILE(TILE_DIALOG_RIGHT);
d_DECLARE_TILE(TILE_DIALOG_DOWN_LEFT);
d_DECLARE_TILE(TILE_DIALOG_DOWN);
d_DECLARE_TILE(TILE_DIALOG_DOWN_RIGHT);
d_DECLARE_TILE(TILE_DIALOG_BG);
d_DECLARE_TILE(TILE_DIALOG_EMPTY);
d_DECLARE_ANI(ANI_DIALOG_STAR);
d_DECLARE_FONT_SPEC(DIALOG_FONTS);
d_DECLARE_ANI(ANI_DIALOG_QMARK);
d_DECLARE_TILE(TILE_HUD_GUN);
d_DECLARE_TILE(TILE_HUD_HEART);
d_DECLARE_TILE(TILE_HUD_KEY);
d_DECLARE_TILE(TILE_HUD_DOLLAR);
d_DECLARE_FONT_SPEC(HUD_FONTS);
d_DECLARE_TEXTURE(MAIN_TEX);
d_DECLARE_TILE(TILE_BG_BRICK_LEFT);
d_DECLARE_TILE(TILE_BG_BRICK_RIGHT);
d_DECLARE_TILE(TILE_BG_BRICK_INNER1);
d_DECLARE_TILE(TILE_BG_BRICK_INNER2);
d_DECLARE_TILE(TILE_STAR_BG_0);
d_DECLARE_TILE(TILE_STAR_BG_1);
d_DECLARE_TILE(TILE_STAR_BG_2);
d_DECLARE_TILE(TILE_STAR_BG_3);
d_DECLARE_TILE(TILE_STAR_BG_4);
d_DECLARE_TILE(TILE_STAR_BG_5);
d_DECLARE_TILE(TILE_STAR_BG_6);
d_DECLARE_ANI(TILE_TORCH_ANI);

#endif
