//
// Created by igor on 10/07/2026.
//

#include <neutrino/video/world/sprite_batch.hh>

#include <algorithm>
#include <cmath>
#include <utility>

namespace neutrino {
    sprite_batch::sprite_batch(const camera& cam, rect viewport, const world_layer_header& plane)
        : m_world(world_transform{cam, viewport, &plane}) {
    }

    // The depth-only overloads are the layer-aware ones at the default band, so a caller can mix
    // the two forms in one batch and get a well-defined order rather than two disjoint schemes.
    void sprite_batch::add(world_point pos, float depth, sprite_visual_ref visual, sprite_draw_params params) {
        add(pos, draw_layer{}, depth, visual, params);
    }

    void sprite_batch::add(world_point pos, float depth, std::optional <sprite_visual_ref> visual,
                           sprite_draw_params params) {
        add(pos, draw_layer{}, depth, visual, params);
    }

    void sprite_batch::add(world_point pos, float depth, sprite_state_id state, sprite_draw_params params) {
        add(pos, draw_layer{}, depth, state, params);
    }

    void sprite_batch::add(world_point pos, draw_layer layer, float depth, sprite_visual_ref visual,
                           sprite_draw_params params) {
        m_entries.push_back(entry{pos, layer, depth, batch_visual{visual}, params});
    }

    void sprite_batch::add(world_point pos, draw_layer layer, float depth,
                           std::optional <sprite_visual_ref> visual, sprite_draw_params params) {
        if (visual) {
            add(pos, layer, depth, *visual, params);
        }
    }

    void sprite_batch::add(world_point pos, draw_layer layer, float depth, sprite_state_id state,
                           sprite_draw_params params) {
        m_entries.push_back(entry{pos, layer, depth, batch_visual{state}, params});
    }

    std::vector <sprite_draw> sprite_batch::plan() const {
        std::vector <entry> ordered = m_entries;
        // Layer first, then depth. Stable, so equal keys keep call order -- that is what lets one
        // add() cover both sorted and unsorted use.
        std::stable_sort(ordered.begin(), ordered.end(),
                         [](const entry& a, const entry& b) {
                             return a.layer != b.layer ? a.layer < b.layer : a.depth < b.depth;
                         });

        std::vector <sprite_draw> out;
        out.reserve(ordered.size());
        for (const entry& e : ordered) {
            if (m_world) {
                const dim vp = m_world->viewport.dimensions();
                const point sp = to_screen(m_world->cam, *m_world->plane, vp, e.pos);
                const point pos{m_world->viewport.x + sp.x, m_world->viewport.y + sp.y};
                // The caller's scale composes on top of the camera zoom (the tile anchor path
                // does the same: it passes {cam.zoom}).
                const sprite_draw_params params{
                    e.params.scale * m_world->cam.zoom, e.params.flip, e.params.rotation_degrees};
                out.push_back(sprite_draw{pos, e.visual, params});
            } else {
                // Screen-space: the position is a literal renderer pixel; no transform, no zoom.
                const point pos{static_cast <int>(std::lround(e.pos.x)),
                                static_cast <int>(std::lround(e.pos.y))};
                out.push_back(sprite_draw{pos, e.visual, e.params});
            }
        }
        return out;
    }

    void sprite_batch::flush() {
        for (const sprite_draw& d : plan()) {
            std::visit([&](auto&& ref) {
                // Skip invalid content (an invalid state would trip draw_sprite's registry
                // enforcement) and ignore per-sprite draw failures: a bad sprite is a
                // no-op, never a thrown exception -- same policy as draw_stats.
                if (ref.valid()) {
                    (void) draw_sprite(d.position, ref, d.params);
                }
            }, d.visual);
        }
        m_entries.clear();
    }
}
