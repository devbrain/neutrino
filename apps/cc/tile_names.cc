//
// Created by igor on 7/21/24.
//

#define INSTANTIATE_TILES
#include "tile_names.hh"

void setup_cc_names(neutrino::texture_atlas& atlas,
                    neutrino::texture_id_t cc_mini_tiles_1,
                    neutrino::texture_id_t cc_mini_tiles_2,
                    neutrino::texture_id_t cc_mini_tiles_3) {
	TILE_DIALOG_UP_LEFT = {cc_mini_tiles_1, 0};
	TILE_DIALOG_UP = {cc_mini_tiles_1, 1};
	TILE_DIALOG_UP_RIGHT = {cc_mini_tiles_1, 2};
	TILE_DIALOG_DOWN_RIGHT = {cc_mini_tiles_1, 3};
	TILE_DIALOG_DOWN_LEFT = {cc_mini_tiles_1, 4};
	TILE_DIALOG_LEFT = {cc_mini_tiles_1, 5};
	TILE_DIALOG_RIGHT = {cc_mini_tiles_1, 7};
	TILE_DIALOG_DOWN = {cc_mini_tiles_1, 6};
	TILE_DIALOG_BG = {cc_mini_tiles_1, 8};
	TILE_DIALOG_EMPTY = {cc_mini_tiles_1, 9};

	ANI_DIALOG_STAR.add_frame({TILE_DIALOG_EMPTY, std::chrono::milliseconds(500)});
	ANI_DIALOG_STAR.add_frame({{cc_mini_tiles_1, 38}, std::chrono::milliseconds(500)});
	ANI_DIALOG_STAR.add_frame({{cc_mini_tiles_1, 39}, std::chrono::milliseconds(500)});
	ANI_DIALOG_STAR.add_frame({{cc_mini_tiles_1, 40}, std::chrono::milliseconds(500)});

	DIALOG_FONTS = neutrino::tile_font_spec_builder(&atlas)
	               .add(cc_mini_tiles_1, neutrino::tile_id_t(10), " !\"#$%&'()*+,-.^`")
	               .add(cc_mini_tiles_1, neutrino::tile_id_t(26), '0', '9')
	               .add(cc_mini_tiles_1, neutrino::tile_id_t(36), ":@")
	               .add(cc_mini_tiles_1, neutrino::tile_id_t(41), "?'")
	               .add(cc_mini_tiles_1, neutrino::tile_id_t(43), "ABCDEFG")
	               .add(cc_mini_tiles_2, neutrino::tile_id_t(0), "HIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz")
	               .build();

	ANI_DIALOG_QMARK.add_frame({{cc_mini_tiles_2, 45}, std::chrono::milliseconds(200)});
	ANI_DIALOG_QMARK.add_frame({{cc_mini_tiles_2, 46}, std::chrono::milliseconds(200)});
	ANI_DIALOG_QMARK.add_frame({{cc_mini_tiles_2, 47}, std::chrono::milliseconds(200)});
	ANI_DIALOG_QMARK.add_frame({{cc_mini_tiles_2, 48}, std::chrono::milliseconds(200)});
	ANI_DIALOG_QMARK.add_frame({{cc_mini_tiles_2, 49}, std::chrono::milliseconds(200)});

	TILE_HUD_GUN = {cc_mini_tiles_3, 0};
	TILE_HUD_HEART = {cc_mini_tiles_3, 4};
	TILE_HUD_KEY = {cc_mini_tiles_3, 5};
	HUD_FONTS = neutrino::tile_font_spec_builder(&atlas)
	            .add(cc_mini_tiles_1, neutrino::tile_id_t(8), "0123456789")
	            .build();
}
