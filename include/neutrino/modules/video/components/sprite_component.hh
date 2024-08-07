//
// Created by igor on 8/7/24.
//

#ifndef NEUTRINO_MODULES_VIDEO_SPRITE_COMPONENT_SPRITE_COMPONENT_HH
#define NEUTRINO_MODULES_VIDEO_SPRITE_COMPONENT_SPRITE_COMPONENT_HH

#include <vector>
#include <neutrino/neutrino_export.hh>
#include <neutrino/modules/video/tile.hh>
#include <neutrino/modules/video/world/animation_sequence.hh>

namespace neutrino {
    struct NEUTRINO_EXPORT single_tile_sprite {
        explicit single_tile_sprite(const tile& sprite);
        tile sprite;
    };

    struct NEUTRINO_EXPORT animated_sprite {
        explicit animated_sprite(const tiled::animation_sequence& sequence);

        std::size_t current_frame;
        std::chrono::milliseconds time_in_state;
        const tiled::animation_sequence& sequence;
    };

    struct NEUTRINO_EXPORT animated_sprite_sequence {
        animated_sprite_sequence();
        void add_state(const tiled::animation_sequence& sequence);
        std::size_t current_state;
        std::vector<animated_sprite> states;
    };
}

#endif
