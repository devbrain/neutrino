//
// Created by igor on 7/21/24.
//

#ifndef  TILE_NAMES_HH
#define  TILE_NAMES_HH

#include <neutrino/systems/video/tile.hh>
#include <neutrino/systems/video/world/animation_sequence.hh>
#include <neutrino/systems/video/tile_font_spec.hh>
#include <neutrino/systems/video/texture_atlas.hh>

void setup_cc_names(neutrino::texture_atlas& atlas,
		neutrino::texture_id_t cc_mini_tiles_1,
		neutrino::texture_id_t cc_mini_tiles_2,
		neutrino::texture_id_t cc_mini_tiles_3
		);

#if !defined(INSTANTIATE_TILES)
#define d_DECLARE_TILE(NAME) extern neutrino::tile NAME
#define d_DECLARE_ANI(NAME) extern neutrino::tiled::animation_sequence NAME
#define d_DECLARE_FONT_SPEC(NAME) extern neutrino::tile_font_spec NAME
#else
#define d_DECLARE_TILE(NAME) neutrino::tile NAME
#define d_DECLARE_ANI(NAME)  neutrino::tiled::animation_sequence NAME
#define d_DECLARE_FONT_SPEC(NAME) neutrino::tile_font_spec NAME
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
d_DECLARE_FONT_SPEC(HUD_FONTS);




#endif
