//
// Created by igor on 8/25/24.
//

#include "maps_registry.hh"
#include "cc_world_mapper.hh"

maps_registry::maps_registry(std::vector <raw_level_map> maps)
    : m_maps(std::move(maps)) {
}

maps_registry::~maps_registry() = default;


