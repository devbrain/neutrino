//
// Created by igor on 8/25/24.
//

#include "maps_registry.hh"
#include "cc_world_mapper.hh"

maps_registry::maps_registry(std::vector<std::tuple<bg_map_t, fg_map_t>> maps)
    : m_maps(std::move(maps)) {
}

neutrino::tiled::world_model maps_registry::get_map(int name, neutrino::ecs::registry& reg) const {
    return cc_create_world_model(std::get<1>(m_maps[name]), std::get<0>(m_maps[name]), reg);
}

