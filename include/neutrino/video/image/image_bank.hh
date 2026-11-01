//
// Created by igor on 29/10/2026.
//

#pragma once

/**
 * @file image_bank.hh
 * @brief @ref image_bank: many numbered pictures, the input to atlas building.
 *
 * A bank is plain data next to @ref image and @ref image_builder: it depends on neither
 * loading nor atlases, so a file parser, a tileset cropping its tiles, or a procedural
 * generator can all fill one.
 */

#include <cstdint>
#include <vector>

#include <neutrino/video/image/image.hh>

namespace neutrino {
    /**
     * @brief Many numbered pictures, in order -- typically the pictures of one multi-picture file.
     *
     * What a parser of a sprite bank, tile set or font returns. Each entry is an ordinary
     * @ref image in the file's native format: an indexed file gives INDEX8 images sharing one
     * palette, with the transparent index as their colour key; anything else gives RGBA.
     *
     * **The id is the game's key**: @ref entry::id is how the game refers to a picture --
     * typically the file's own number (a BOB block, a tile number, a character code), possibly
     * sparse. It is opaque to the engine and need not be unique; it is carried through atlas
     * building and looked up with @ref image_atlas::find_by_id. Anything the game knows per
     * picture beyond its pixels (a hit box, a flag) belongs in its own tables keyed by id.
     * The entry's position (region @c i of the atlas is entry @c i) is an engine-side index.
     */
    struct image_bank {
        /// @brief One picture of the bank.
        struct entry {
            image pixels; ///< The picture's pixels.
            uint32_t id = 0; ///< The game's key for this picture (usually the file's own number).
        };

        std::vector <entry> entries; ///< The pictures, in order.
    };
}
