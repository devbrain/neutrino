//
// Created by igor on 8/25/24.
//

#ifndef  MAPS_REGISTRY_HH
#define  MAPS_REGISTRY_HH

#include <memory>
#include <neutrino/modules/video/world_renderer.hh>
#include "data_loader/raw_level_map.hh"
#include "level/level.hh"

class maps_registry {
    public:
        static constexpr int INTRO = 0;
        static constexpr int OUTRO = 1;
        static constexpr int MAIN_LEVEL = 2;

        explicit maps_registry(std::vector <raw_level_map> maps);
        virtual ~maps_registry();
        virtual std::unique_ptr<level> get_map(int name, neutrino::world_renderer& wr) const = 0;
    protected:
        [[nodiscard]] const raw_level_map& get_by_name(int name) const;
        std::vector <raw_level_map> m_maps;
};



#endif
