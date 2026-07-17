//
// Created by igor on 17/07/2026.
//

#pragma once

#include <neutrino/video/world/render_layer.hh>

// The game-drawn "actors" layer: fills the sprite_batch with the moving sprites (just the
// paddle for now). A world_compositor slots it right after the brick tile layer.
class actors_layer : public neutrino::render_layer {
    public:
        neutrino::sprite_visual_ref paddle;
        neutrino::world_point paddle_pos{0.0f, 0.0f}; // sprite pivot position, world/screen pixels

        void draw(const neutrino::layer_view& view, neutrino::sprite_batch& batch) override;
};


