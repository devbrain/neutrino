//
// Created by igor on 9/1/24.
//

#ifndef NEUTRINO_VIDEO_PROPERTIES_SPRITE_PROPERTIES_HH
#define NEUTRINO_VIDEO_PROPERTIES_SPRITE_PROPERTIES_HH

#include <map>
#include <neutrino/s11n/s11n.hh>
#include <neutrino/modules/video/types.hh>
#include <neutrino/modules/video/properties/tile_properties.hh>

namespace neutrino {
    struct NEUTRINO_EXPORT sprite_properties {
        static constexpr int INVALID_OBJECT_ID = -1;
        sprite_properties() = default;

        int object_id{INVALID_OBJECT_ID};
        std::map <int, tiles_sequence> tiles;

        SERIALIZATION_SCHEMA(object_id, tiles)
    };
}

#endif
