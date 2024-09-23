//
// Created by igor on 9/17/24.
//

#ifndef  CRYSTAL_CAVES_MAP_HH
#define  CRYSTAL_CAVES_MAP_HH

#include "level/maps_registry.hh"

class crystal_caves_map : public maps_registry {
    public:
        explicit crystal_caves_map(std::vector <raw_level_map> maps);
        [[nodiscard]] neutrino::tiled::world_model get_map(int name,
                                                           ecs_registry& reg,
                                                           neutrino::world_renderer& wr) const override;
};

#endif
