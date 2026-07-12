//
// Created by igor on 09/07/2026.
//

#include <neutrino/video/world/camera.hh>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

#include <failsafe/enforce.hh>

namespace neutrino {
    namespace {
        // Derived hexagonal stagger geometry (Tiled's HexagonalRenderer::RenderParams).
        struct hex_layout {
            bool stagger_x;    // stagger axis X (flat-top, columns staggered)
            bool stagger_even; // stagger the even index rather than the odd
            int  tile_w, tile_h;
            int  col_w, row_h; // compressed column/row stride
        };

        hex_layout hex_of(const world& w) {
            hex_layout h{};
            h.stagger_x = w.stagger_axis() == world_stagger_axis::x;
            h.stagger_even = w.stagger_index() == world_stagger_index::even;
            h.tile_w = static_cast <int>(w.tile_width());
            h.tile_h = static_cast <int>(w.tile_height());
            const int side = static_cast <int>(w.hex_side_length());
            const int side_x = h.stagger_x ? side : 0;
            const int side_y = h.stagger_x ? 0 : side;
            h.col_w = (h.tile_w - side_x) / 2 + side_x;
            h.row_h = (h.tile_h - side_y) / 2 + side_y;
            return h;
        }
    }

    world_point cell_to_world(const world& w, int cx, int cy) {
        const auto tw = static_cast <float>(w.tile_width());
        const auto th = static_cast <float>(w.tile_height());
        switch (w.orientation()) {
        case world_orientation::isometric: {
            // Diamond projection (Tiled IsometricRenderer): tileToScreenCoords is the
            // top corner; the tile's bbox bottom-left is (x - tw/2, y + th). originX
            // shifts the map into positive x, matching Tiled's world coordinates.
            const float origin_x = static_cast <float>(w.height()) * tw / 2.0f;
            const float px = static_cast <float>(cx - cy) * tw / 2.0f + origin_x;
            const float py = static_cast <float>(cx + cy) * th / 2.0f;
            return world_point{px - tw / 2.0f, py + th};
        }
        case world_orientation::hexagonal:
        case world_orientation::staggered: { // staggered == hex with side length 0
            const hex_layout h = hex_of(w);
            int px, py; // tile bounding-box top-left (Tiled tileToScreenCoords)
            if (h.stagger_x) {
                px = cx * h.col_w;
                py = cy * h.tile_h + (((cx & 1) ^ static_cast <int>(h.stagger_even)) ? h.row_h : 0);
            } else {
                px = cx * h.tile_w + (((cy & 1) ^ static_cast <int>(h.stagger_even)) ? h.col_w : 0);
                py = cy * h.row_h;
            }
            return world_point{static_cast <float>(px), static_cast <float>(py + h.tile_h)}; // bottom-left
        }
        default: // orthogonal / unknown
            return world_point{static_cast <float>(cx) * tw, static_cast <float>(cy + 1) * th};
        }
    }

    cell_range visible_cell_bounds(const world& w, const world_layer_header& layer,
                                   const camera& cam, dim viewport) {
        ENFORCE(cam.zoom > 0.0f && std::isfinite(cam.zoom));

        auto o = details::eval_layer_origin(cam,
                                            {layer.parallax_x, layer.parallax_y},
                                            layer.offset, viewport);

        // World-x/y at the viewport's left/top (screen 0) and right/bottom edges.
        const auto wr = o.x + static_cast<float>(viewport.width) / cam.zoom;
        const auto wb = o.y + static_cast<float>(viewport.height) / cam.zoom;

        const auto orient = w.orientation();
        if (orient == world_orientation::hexagonal || orient == world_orientation::staggered) {
            // Conservative: cells are staggered and overlap, so map the world rect to
            // grid indices by the per-axis stride and inflate to cover the half-stagger
            // shift and the full tile bounding box. Over-included cells clip off-screen.
            // (Staggered is hex with side length 0.)
            const hex_layout h = hex_of(w);
            const auto sx = static_cast <float>(h.stagger_x ? h.col_w : h.tile_w);
            const auto sy = static_cast <float>(h.stagger_x ? h.tile_h : h.row_h);
            const int mx = 2 + h.tile_w / std::max(1, static_cast <int>(sx));
            const int my = 2 + h.tile_h / std::max(1, static_cast <int>(sy));
            return {
                static_cast <int>(std::floor(o.x / sx)) - mx,
                static_cast <int>(std::floor(o.y / sy)) - my,
                static_cast <int>(std::ceil(wr / sx)) + mx,
                static_cast <int>(std::ceil(wb / sy)) + my,
            };
        }

        if (orient == world_orientation::isometric) {
            // Invert the diamond projection at the four view-rect corners and take the
            // bounding cell box. cx = (a+b)/2, cy = (b-a)/2 with a = 2*(x-originX)/tw,
            // b = 2*y/th. Inflate for tiles taller than the diamond (vertical overhang).
            const auto tw = static_cast <float>(w.tile_width());
            const auto th = static_cast <float>(w.tile_height());
            const float origin_x = static_cast <float>(w.height()) * tw / 2.0f;
            int cx_lo = std::numeric_limits <int>::max(), cy_lo = std::numeric_limits <int>::max();
            int cx_hi = std::numeric_limits <int>::min(), cy_hi = std::numeric_limits <int>::min();
            for (const auto& [px, py] : {std::pair{o.x, o.y}, {wr, o.y}, {o.x, wb}, {wr, wb}}) {
                const float a = 2.0f * (px - origin_x) / tw;
                const float b = 2.0f * py / th;
                const int cx = static_cast <int>(std::floor((a + b) / 2.0f));
                const int cy = static_cast <int>(std::floor((b - a) / 2.0f));
                cx_lo = std::min(cx_lo, cx); cx_hi = std::max(cx_hi, cx);
                cy_lo = std::min(cy_lo, cy); cy_hi = std::max(cy_hi, cy);
            }
            const int m = 3; // half-cell diagonal rounding + tile-height overhang
            return {cx_lo - m, cy_lo - m, cx_hi + m + 1, cy_hi + m + 1};
        }

        const auto tw = static_cast <float>(w.tile_width());
        const auto th = static_cast <float>(w.tile_height());
        // floor the near edge, ceil the far edge: include every cell the viewport
        // touches. Half-open, unclamped (infinite layers have no bounds to clamp to).
        return {
            static_cast <int>(std::floor(o.x / tw)),
            static_cast <int>(std::floor(o.y / th)),
            static_cast <int>(std::ceil(wr / tw)),
            static_cast <int>(std::ceil(wb / th)),
        };
    }

    cell_range visible_cell_range(const world& w, const world_tile_layer& layer,
                                  const camera& cam, dim viewport) {
        const cell_range b = visible_cell_bounds(w, layer, cam, viewport);
        const int lw = static_cast <int>(layer.width);
        const int lh = static_cast <int>(layer.height);
        return {
            std::clamp(b.x0, 0, lw),
            std::clamp(b.y0, 0, lh),
            std::clamp(b.x1, 0, lw),
            std::clamp(b.y1, 0, lh),
        };
    }
}
