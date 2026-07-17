//
// Created by igor on 09/07/2026.
//

#pragma once

/**
 * @file camera.hh
 * @brief A 2D render-space camera plus visibility culling for tile layers --
 *        finite or infinite/chunked, orthogonal, isometric, or hexagonal.
 *
 * The camera lives in tile-pixel space (@ref world_point), deliberately separate
 * from the physics module's world-unit coordinate model: the tile world is
 * authored in pixels, which is the space it is culled and positioned in. Making
 * the camera follow a physics body is an explicit conversion at the call site,
 * not a shared type.
 *
 * @ref visible_cell_range inverts the exact transform @ref to_screen applies, so
 * culling and drawing can never drift apart.
 */

#include <cmath>
#include <neutrino/neutrino_export.h>
#include <neutrino/world/world.hh>

namespace neutrino {
    /**
     * @brief A 2D "look-at" camera over the tile world.
     *
     * The camera is defined by the world point it centres on (@ref target) and a
     * @ref zoom. This target formulation keeps parallax simple: a layer with parallax
     * factor `f` shows the *effective* target `rest + f*(target - rest)` at the screen
     * centre, so `f = 1` follows the camera and `f = 0` stays pinned to @ref
     * parallax_rest. There is no separate top-left/view-centre bookkeeping — the
     * viewport contributes only a centring term — and layers align exactly when
     * `target == rest`.
     */
    struct camera {
        world_point target{0.0f, 0.0f};        ///< World point shown at the viewport centre (the look-at point).
        float zoom{1.0f};                      ///< Scale factor; > 1 zooms in, < 1 zooms out. Must be positive and finite.
        world_point parallax_rest{0.0f, 0.0f}; ///< The target value at which every parallax layer aligns (the map's parallax origin).
    };

    namespace details {
        /**
         * @brief World point a layer draws at the viewport's top-left corner.
         *
         * Applies parallax by picking the layer's *effective target*
         * `rest + factor*(target - rest)` (per axis), then backs off half the viewport
         * to reach the top-left corner and subtracts the layer's static offset. At
         * factor 1 the effective target is the camera target (ordinary tile layers);
         * at factor 0 it is @ref camera::parallax_rest (a fixed background).
         *
         * @param cam        Active camera.
         * @param parallax   Per-axis parallax factors (`{parallax_x, parallax_y}`).
         * @param layer_offs Layer's static draw offset in world pixels.
         * @param viewport   Viewport size in screen pixels.
         * @return The layer origin in world pixels (drawn at the viewport top-left).
         */
        inline world_point eval_layer_origin(const camera& cam,
                                             const sdlpp::point <float>& parallax,
                                             const world_point& layer_offs,
                                             dim viewport) {
            const float eff_x = cam.parallax_rest.x + parallax.x * (cam.target.x - cam.parallax_rest.x);
            const float eff_y = cam.parallax_rest.y + parallax.y * (cam.target.y - cam.parallax_rest.y);
            return world_point{
                eff_x - static_cast<float>(viewport.width) / (2.0f * cam.zoom),
                eff_y - static_cast<float>(viewport.height) / (2.0f * cam.zoom)
            } - layer_offs;
        }
    }

    /**
     * @brief Project a world point into screen pixels for a specific layer.
     *
     * Honours the layer's parallax (anchored at the view centre, Tiled-style) and its
     * static offset, then scales by zoom and rounds to the nearest pixel. @p viewport
     * is the destination size in screen pixels (its centre is the parallax anchor).
     *
     * @param cam      Active camera.
     * @param layer    Layer whose parallax/offset apply.
     * @param viewport Destination size in screen pixels.
     * @param p        Point in world pixels.
     * @return The screen pixel (rounded), relative to the viewport's top-left.
     */
    [[nodiscard]] inline
    point to_screen(const camera& cam, const world_layer_header& layer, dim viewport, const world_point& p) {
        auto layer_origin = details::eval_layer_origin(cam,
                                                       {layer.parallax_x, layer.parallax_y},
                                                       layer.offset, viewport);

        auto temp = (p - layer_origin) * cam.zoom;
        return {static_cast <int>(std::round(temp.x)), static_cast <int>(std::round(temp.y))};
    }

    /**
     * @brief Project a world point into screen pixels at parallax 1 (centred on the
     *        camera target), with no layer offset.
     *
     * The overload for foreground/object-space points that do not belong to a
     * parallaxed tile layer. Equivalent to a layer with parallax 1 and no offset.
     *
     * @param cam      Active camera.
     * @param viewport Viewport size in screen pixels.
     * @param p        Point in world pixels.
     * @return The screen pixel (rounded), relative to the viewport's top-left.
     */
    [[nodiscard]] inline
    point to_screen(const camera& cam, dim viewport, const world_point& p) {
        auto layer_origin = details::eval_layer_origin(cam, {1.0f, 1.0f}, world_point{0.0f, 0.0f}, viewport);
        auto temp = (p - layer_origin) * cam.zoom;
        return {static_cast <int>(std::round(temp.x)), static_cast <int>(std::round(temp.y))};
    }

    /**
     * @brief A half-open, layer-clamped rectangle of cell indices `[x0,x1) x [y0,y1)`.
     *
     * Produced by @ref visible_cell_range. Being half-open and pre-clamped, it can
     * be iterated directly and never yields negative or out-of-bounds indices.
     */
    struct cell_range {
        int x0{0}, y0{0}, x1{0}, y1{0}; ///< Half-open bounds `[x0,x1) x [y0,y1)`, already clamped to the layer.

        /// @return true when the range covers no cells.
        [[nodiscard]] bool empty() const noexcept {
            return x1 <= x0 || y1 <= y0;
        }
    };

    /**
     * @brief Visible cell rectangle, half-open, **unclamped** to any layer bounds.
     *
     * The raw floor/ceil inverse of @ref to_screen for the layer: near edges floored,
     * far edges ceiled, so every cell the viewport touches is included. Indices may be
     * negative or arbitrarily large — this is the form infinite/chunked layers need,
     * since they have no finite `width`/`height` to clamp against; chunk intersection
     * uses it directly.
     *
     * @param w        World supplying the map tile size (the cell stride).
     * @param layer    Layer header supplying parallax + offset.
     * @param cam      Active camera. Its zoom must be positive and finite.
     * @param viewport Viewport size in screen pixels.
     * @return The visible cell rectangle, half-open and unclamped.
     * @throws std::runtime_error when `cam.zoom` is not positive and finite.
     */
    [[nodiscard]] NEUTRINO_EXPORT cell_range visible_cell_bounds(const world& w, const world_layer_header& layer, const camera& cam, dim viewport);

    /**
     * @brief Cells of a finite orthogonal tile layer intersecting the viewport.
     *
     * @ref visible_cell_bounds clamped into the layer grid, so the result never yields
     * a negative or out-of-bounds index. Use this for finite layers; use
     * @ref visible_cell_bounds for infinite/chunked ones.
     *
     * @param w        World supplying the map tile size (the cell stride).
     * @param layer    Layer to cull; its width/height, parallax and offset apply.
     * @param cam      Active camera. Its zoom must be positive and finite.
     * @param viewport Viewport size in screen pixels.
     * @return The visible cell rectangle, half-open and clamped to the layer.
     * @throws std::runtime_error when `cam.zoom` is not positive and finite.
     */
    [[nodiscard]] NEUTRINO_EXPORT cell_range visible_cell_range(const world& w, const world_tile_layer& layer, const camera& cam, dim viewport);

    /**
     * @brief World point at grid cell `(cx, cy)`'s bottom-left corner, honoring the
     *        map's orientation.
     *
     * The one place the cell→pixel layout lives. Orthogonal maps use the plain grid
     * `(cx·tw, (cy+1)·th)`; hexagonal (staggered) maps use Tiled's hex stagger formula
     * (`hex_side_length`, `stagger_axis`, `stagger_index`). The renderer anchors each
     * tile here, so adding an orientation only touches this and @ref visible_cell_bounds.
     */
    [[nodiscard]] NEUTRINO_EXPORT world_point cell_to_world(const world& w, int cx, int cy);
}
