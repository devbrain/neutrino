//
// Created by igor on 17/07/2026.
//

#include <string>

#include <ke/game/actors_layer.hh>
#include <ke/game/model.hh>
#include <ke/assets/registry.hh>
#include <ke/assets/sprites.hh>

void actors_layer::draw(const neutrino::layer_view&, neutrino::sprite_batch& batch) {
   draw_paddle(batch);
}

void actors_layer::draw_paddle(neutrino::sprite_batch& batch) {
    const auto& assets = rs::require_ke_assets();
    if (!assets.paddle.valid()) {
        return;
    }
    const paddle_info& inf = model::instance().get_paddle();

    // The paddle's form is (state, size): the state picks the KE_RACK frame range, the size the frame within it.
    const auto visual = assets.paddle.visual(std::to_string(rs::ke_paddle_frame(inf.state, inf.size)));
    if (!visual) {
        return;
    }

    // Pivot position; depth = y so the paddle sorts by its scanline like other actors.
    const neutrino::world_point pos{static_cast <float>(inf.x), static_cast <float>(inf.y)};
    batch.add(pos, pos.y, *visual);
}


