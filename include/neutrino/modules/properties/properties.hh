//
// Created by igor on 9/1/24.
//

#ifndef NEUTRINO_PROPERTIES_PROPERTIES_HH
#define NEUTRINO_PROPERTIES_PROPERTIES_HH

#include <neutrino/modules/video/properties/tile_properties.hh>
#include <neutrino/modules/video/properties/sprite_properties.hh>

namespace neutrino {
    struct NEUTRINO_EXPORT properties {
        /**
         * Map from sprite_id to tile sequences
         */
        std::map<int, sprite_properties> sprites;

        SERIALIZATION_SCHEMA(sprites)
    };
}

#endif
