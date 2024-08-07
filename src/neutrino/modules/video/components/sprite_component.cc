//
// Created by igor on 8/7/24.
//

#include <neutrino/modules/video/components/sprite_component.hh>

namespace neutrino {
    single_tile_sprite::single_tile_sprite(const tile& sprite)
        : sprite(sprite) {
    }

    animated_sprite::animated_sprite(const tiled::animation_sequence& sequence)
        : current_frame(0),
          time_in_state(0),
          sequence(sequence) {
    }

    animated_sprite_sequence::animated_sprite_sequence()
        : current_state(0) {
    }

    void animated_sprite_sequence::add_state(const tiled::animation_sequence& sequence) {
        states.emplace_back(sequence);
    }
}
