//
// Created by igor on 9/22/24.
//

#include "sprite_components_factory.hh"
#include "crystal_caves_sprite_states.hh"
#include "tile_names.hh"

neutrino::single_tile_sprite create_single_tile_component(int sprite) {
	return neutrino::single_tile_sprite{neutrino::tile(MAIN_TEX, neutrino::tile_id_t(sprite))};
}

neutrino::sprite_bank_array create_mylo_sprite_component() {
	neutrino::sprite_bank_array sc(
		neutrino::sprite_bank(MAIN_TEX, 262, 273),
		neutrino::sprite_bank(MAIN_TEX, 250, 261),
		neutrino::sprite_bank(MAIN_TEX, 362, 373),
		neutrino::sprite_bank(MAIN_TEX, 350, 361),
		neutrino::sprite_bank(MAIN_TEX, 275, 275),
		neutrino::sprite_bank(MAIN_TEX, 274, 274),
		neutrino::sprite_bank(MAIN_TEX, 277, 277),
		neutrino::sprite_bank(MAIN_TEX, 276, 276),
		neutrino::sprite_bank(MAIN_TEX, 375, 375),
		neutrino::sprite_bank(MAIN_TEX, 374, 374),
		neutrino::sprite_bank(MAIN_TEX, 377, 377),
		neutrino::sprite_bank(MAIN_TEX, 376, 376),
		neutrino::sprite_bank(MAIN_TEX, 800, 816),
		neutrino::sprite_bank(MAIN_TEX, 817, 820)
	);
	sc.current = SPRITE_STATE_MOVE_LEFT;

	return sc;
}