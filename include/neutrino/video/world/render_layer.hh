//
// Created by igor on 10/07/2026.
//

#pragma once

/**
 * @file render_layer.hh
 * @brief A game-drawn layer slotted into the world's layer order.
 *
 * A @ref render_layer is runtime, game-owned content -- actors (player, enemies,
 * projectiles) or a live/procedural background -- interleaved among the static map
 * layers at a chosen z-slot by the compositor. It is NOT part of @ref world and NOT
 * a @ref world_layer variant: the map stays const and shareable; the layer draws
 * @ref sprite_visual_ref / @ref sprite_state_id the game already resolved, never
 * touching the tileset, gids, or the resource cache.
 *
 * "Actor" and "background" are roles, not classes: an actor fills the @ref
 * sprite_batch with depth-sorted sprites; a live background ignores the batch and
 * blits its own texture using the plane transform. Both are @ref render_layer.
 */

#include <neutrino/world/world_common.hh>
#include <neutrino/world/world_layers.hh>
#include <neutrino/video/world/sprite_batch.hh>

namespace neutrino {
    /**
     * @brief Read-only per-frame inputs handed to a @ref render_layer::draw.
     *
     * @ref visible is the camera's visible world rectangle **on this layer's plane**
     * (parallax + zoom already applied), so the layer culls against its own view.
     * @ref alpha is the fixed-step interpolation factor, for `lerp(prev, curr, alpha)`.
     */
    struct layer_view {
        world_rect visible;     ///< Visible world region, in world pixels, on this slot's plane.
        float      alpha{1.0f}; ///< Fixed-step interpolation factor in [0, 1].
        rect       viewport;    ///< Destination rectangle in renderer pixels (for layers that blit directly, e.g. a live background).
    };

    /**
     * @brief Base for a game-drawn layer. Subclass and implement @ref draw.
     */
    class NEUTRINO_EXPORT render_layer {
        public:
            virtual ~render_layer() = default;

            /// @brief Parallax / offset / opacity / tint -- picks the plane, like a tile layer.
            world_layer_header plane;

            /**
             * @brief Fill @p batch from the layer's (interpolated, culled) content.
             *
             * The compositor builds @p batch on @ref plane and flushes it after this
             * returns, so depth sorting is scoped to this slot.
             */
            virtual void draw(const layer_view& view, sprite_batch& batch) = 0;
    };
}
