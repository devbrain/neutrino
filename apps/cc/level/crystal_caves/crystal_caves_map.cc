//
// Created by igor on 9/17/24.
//

#include "crystal_caves_map.hh"

crystal_caves_map::crystal_caves_map(std::vector<raw_level_map> maps)
    : maps_registry(std::move(maps)) {
}

neutrino::tiled::world_model crystal_caves_map::get_map(int name, neutrino::ecs::registry& reg) const {
    neutrino::tiled::world_model wm;
    return wm;
}


