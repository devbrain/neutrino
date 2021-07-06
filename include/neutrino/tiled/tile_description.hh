//
// Created by igor on 06/07/2021.
//

#ifndef NEUTRINO_TILED_TILE_DESCRIPTION_HH
#define NEUTRINO_TILED_TILE_DESCRIPTION_HH

#include <cstdint>
#include <vector>
#include <variant>

namespace neutrino::tiled {
    enum class flip_t {
        NONE,
        HORIZONTAL,
        VERTICAL
    };

    struct tile_description
    {
        std::size_t tile_sheet_id;
        std::size_t tile_id;
        flip_t m_flip;
        double rotate_radians;
    };

    struct sprite {
        std::vector<tile_description> frames;
    };

    struct tiles_layer {
        int w;
        int h;
        std::vector<tile_description> tiles;
    };

    struct image_layer {
        int w;
        int h;
        tile_description image;
    };

    using layer = std::variant<tiles_layer, image_layer>;

}


#endif
