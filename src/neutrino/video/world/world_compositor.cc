//
// Created by igor on 10/07/2026.
//

#include <neutrino/video/world/world_compositor.hh>

namespace neutrino {
    world_compositor::world_compositor(world_renderer& renderer)
        : m_renderer(&renderer) {
    }

    void world_compositor::insert_after(world_layer_id after, render_layer& layer) {
        m_slots.emplace_back(after, &layer);
    }

    void world_compositor::insert_bottom(render_layer& layer) {
        m_bottom.push_back(&layer);
    }

    draw_stats world_compositor::draw(const camera& cam, const rect& viewport, float alpha) {
        // The camera the batches use must carry the map's parallax rest, exactly as the
        // renderer injects it for the tile layers -- otherwise game-drawn layers on a
        // parallax plane would not align with the tiles.
        camera view = cam;
        view.parallax_rest = m_renderer->parallax_rest();
        const dim vp = viewport.dimensions();

        // Build this slot's layer_view (visible rect on its plane) + a batch on its plane,
        // let it draw, and flush -- so depth sorting is scoped to the slot. The visible
        // rect uses the same eval_layer_origin transform tile culling does (no drift).
        auto draw_slot = [&](render_layer& layer) {
            const world_point origin = details::eval_layer_origin(
                view, {layer.plane.parallax_x, layer.plane.parallax_y}, layer.plane.offset, vp);
            const layer_view lv{
                world_rect{origin.x, origin.y,
                           static_cast <float>(vp.width) / view.zoom,
                           static_cast <float>(vp.height) / view.zoom},
                alpha,
                viewport};

            sprite_batch batch(view, viewport, layer.plane);
            layer.draw(lv, batch);
            batch.flush();
        };

        // Backgrounds first, beneath every map layer.
        for (render_layer* layer : m_bottom) {
            draw_slot(*layer);
        }

        return m_renderer->draw(cam, viewport, [&](world_layer_id done) {
            for (const auto& [after, layer] : m_slots) {
                if (after == done) {
                    draw_slot(*layer);
                }
            }
        });
    }
}
