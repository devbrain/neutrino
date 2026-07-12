//
// Created by igor on 10/07/2026.
//

#include <neutrino/video/world/sprite_batch.hh>

#include <algorithm>
#include <utility>

namespace neutrino {
    sprite_batch::sprite_batch(const camera& cam, rect viewport, const world_layer_header& plane)
        : m_cam(cam), m_viewport(viewport), m_plane(&plane) {
    }

    void sprite_batch::add(world_point pos, float depth, sprite_visual_ref visual, sprite_draw_params params) {
        m_entries.push_back(entry{pos, depth, batch_visual{visual}, params});
    }

    void sprite_batch::add(world_point pos, float depth, sprite_state_id state, sprite_draw_params params) {
        m_entries.push_back(entry{pos, depth, batch_visual{state}, params});
    }

    std::vector <sprite_draw> sprite_batch::plan() const {
        std::vector <entry> ordered = m_entries;
        // Stable so equal depths keep call order -- that is what lets one add() cover
        // both sorted and unsorted use.
        std::stable_sort(ordered.begin(), ordered.end(),
                         [](const entry& a, const entry& b) { return a.depth < b.depth; });

        const dim vp = m_viewport.dimensions();
        std::vector <sprite_draw> out;
        out.reserve(ordered.size());
        for (const entry& e : ordered) {
            const point sp = to_screen(m_cam, *m_plane, vp, e.pos);
            const point pos{m_viewport.x + sp.x, m_viewport.y + sp.y};
            // The caller's scale composes on top of the camera zoom (the tile anchor path
            // does the same: it passes {cam.zoom}).
            const sprite_draw_params params{e.params.scale * m_cam.zoom, e.params.flip, e.params.rotation_degrees};
            out.push_back(sprite_draw{pos, e.visual, params});
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
