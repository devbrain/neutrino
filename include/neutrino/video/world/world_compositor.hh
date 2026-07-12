//
// Created by igor on 10/07/2026.
//

#pragma once

/**
 * @file world_compositor.hh
 * @brief Interleaves a @ref world_renderer's static layers with game-drawn
 *        @ref render_layer slots.
 *
 * The compositor owns the seam: it draws the map's static layers in order and, at
 * each slot, builds a @ref sprite_batch on that slot's plane, computes the slot's
 * @ref layer_view (the visible world rect on that plane, plus the frame's alpha),
 * calls @ref render_layer::draw, and flushes -- so depth sorting is scoped to the
 * slot. The renderer stays agnostic of what a slot draws.
 */

#include <utility>
#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/world/camera.hh>
#include <neutrino/video/world/render_layer.hh>
#include <neutrino/video/world/world_renderer.hh>

namespace neutrino {
    /**
     * @brief Slots @ref render_layer instances into a @ref world_renderer's layer order.
     */
    class NEUTRINO_EXPORT world_compositor {
        public:
            /// @param renderer Bound renderer; must outlive the compositor.
            explicit world_compositor(world_renderer& renderer);

            /**
             * @brief Draw @p layer immediately after the map layer whose id is @p after.
             *
             * @p layer must outlive the compositor. Multiple layers can share a slot;
             * they draw in insertion order. A slot whose id matches no map layer never
             * draws.
             */
            void insert_after(world_layer_id after, render_layer& layer);

            /**
             * @brief Draw @p layer beneath every map layer (a background), before the
             *        static layers. Bottom layers draw in insertion order. @p layer must
             *        outlive the compositor.
             */
            void insert_bottom(render_layer& layer);

            /**
             * @brief Draw the static layers and the slotted @ref render_layer content.
             * @param alpha Fixed-step interpolation factor passed to each slot's @ref layer_view.
             */
            draw_stats draw(const camera& cam, const rect& viewport, float alpha = 1.0f);

        private:
            world_renderer*                                             m_renderer;
            std::vector <render_layer*>                                  m_bottom;
            std::vector <std::pair <world_layer_id, render_layer*>>      m_slots;
    };
}
