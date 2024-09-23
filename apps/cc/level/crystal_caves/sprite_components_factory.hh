//
// Created by igor on 9/22/24.
//

#ifndef  SPRITE_COMPONENTS_FACTORY_HH
#define  SPRITE_COMPONENTS_FACTORY_HH

#include <neutrino/modules/video/components/sprite_component.hh>

neutrino::single_tile_sprite create_single_tile_component(int sprite);
neutrino::sprite_bank_array create_mylo_sprite_component();

#endif
