//
// Created by igor on 17/07/2026.
//

#include "actors_layer.hh"

void actors_layer::draw(const neutrino::layer_view&, neutrino::sprite_batch& batch) {
    if (paddle.valid()) {
        batch.add(paddle_pos, paddle_pos.y, paddle);
    }
}


