//
// Created by igor on 06/07/2021.
//

#ifndef NEUTRINO_TILED_WORLD_TILE_DESCRIPTION_HH
#define NEUTRINO_TILED_WORLD_TILE_DESCRIPTION_HH

#include <cstdint>
#include <vector>
#include <variant>
#include <neutrino/tiled/world/tile_types.hh>

namespace neutrino::tiled {
    enum class flip_t {
        NONE,
        HORIZONTAL,
        VERTICAL
    };

    struct tile_description
    {
        tile_sheet_id_t tile_sheet_id;
        std::size_t tile_id;
        flip_t m_flip;
        double rotate_radians;
    };

    struct empty_tile {};

    using tile_t = std::variant<empty_tile, tile_description>;

    struct sprite {
        std::vector<tile_description> frames;
    };

    // all tiles in the layer should be of the same dimension
    struct tiles_layer {
        int w; // width in tiles
        int h; // height in tiles
        int tile_width;
        int tile_height;
        std::vector<tile_t> tiles;
    };

    struct image_layer {
        int w;
        int h;
        int tile_width;
        int tile_height;
        tile_description image;
    };

    using layer = std::variant<tiles_layer, image_layer>;

}


#endif
