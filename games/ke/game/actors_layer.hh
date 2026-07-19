//
// Created by igor on 17/07/2026.
//

#pragma once

#include <neutrino/video/world/render_layer.hh>

// The game-drawn "actors" layer: fills the sprite_batch with the moving sprites (just the
// paddle for now). A world_compositor slots it right after the brick tile layer.
class actors_layer : public neutrino::render_layer {
    public:
        void draw(const neutrino::layer_view& view, neutrino::sprite_batch& batch) override;
    private:
        void draw_paddle(neutrino::sprite_batch& batch);
};
