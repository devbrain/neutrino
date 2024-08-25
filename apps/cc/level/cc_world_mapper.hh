//
// Created by igor on 8/25/24.
//

#ifndef  CC_WORLD_MAPPER_HH
#define  CC_WORLD_MAPPER_HH

#include "data_loader/map_tile.hh"
#include <neutrino/ecs/registry.hh>
#include <neutrino/modules/video/world/world_model.hh>

neutrino::tiled::world_model cc_create_world_model(const fg_map_t& fg_map, const bg_map_t& bg_map, neutrino::ecs::registry& reg);



#endif
