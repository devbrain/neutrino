//
// Created by igor on 8/25/24.
//

#ifndef  MAPS_REGISTRY_HH
#define  MAPS_REGISTRY_HH

#include <neutrino/modules/video/world/world_model.hh>
#include "data_loader/data_directory.hh"
#include <neutrino/ecs/registry.hh>


class maps_registry {
    public:
        static constexpr int INTRO = 0;
        static constexpr int OUTRO = 1;
        static constexpr int MAIN_LEVEL = 2;

        explicit maps_registry(std::vector <std::tuple <bg_map_t, fg_map_t>> maps);
        [[nodiscard]] neutrino::tiled::world_model get_map(int name, neutrino::ecs::registry& reg) const;
    private:
        std::vector <std::tuple <bg_map_t, fg_map_t>> m_maps;
};

neutrino::tiled::world_model get_map(int name, neutrino::ecs::registry& reg);

#endif
