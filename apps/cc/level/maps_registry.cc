//
// Created by igor on 8/25/24.
//

#include "maps_registry.hh"
#include <bsw/exception.hh>

maps_registry::maps_registry(std::vector <raw_level_map> maps)
    : m_maps(std::move(maps)) {
}

maps_registry::~maps_registry() = default;

const raw_level_map& maps_registry::get_by_name(int name) const {
    for (const auto& lvl : m_maps) {
        if (lvl.level_num == name) {
            return lvl;
        }
    }
    RAISE_EX("Can not find map ", name);
}


