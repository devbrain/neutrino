//
// Created by igor on 8/25/24.
//

#ifndef  MAPS_REGISTRY_HH
#define  MAPS_REGISTRY_HH

#include <neutrino/modules/video/world/world_model.hh>
#include "data_loader/data_directory.hh"
#include <neutrino/ecs/registry.hh>
#include <neutrino/modules/video/world_renderer.hh>
#include "level/ecs_registry.hh"

class maps_registry {
    public:
        static constexpr int INTRO = 0;
        static constexpr int OUTRO = 1;
        static constexpr int MAIN_LEVEL = 2;

        explicit maps_registry(std::vector <raw_level_map> maps);
        virtual ~maps_registry();
        virtual neutrino::tiled::world_model get_map(int name,
            ecs_registry& reg,
            neutrino::world_renderer& wr) const = 0;
    protected:
        [[nodiscard]] const raw_level_map& get_by_name(int name) const;
        std::vector <raw_level_map> m_maps;
};



#endif
