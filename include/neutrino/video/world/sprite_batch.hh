//
// Created by igor on 10/07/2026.
//

#pragma once

/**
 * @file sprite_batch.hh
 * @brief A camera-aware, depth-sorted sprite draw sink.
 *
 * Fill a batch with @ref sprite_batch::add during a draw pass, then @ref
 * sprite_batch::flush stable-sorts the queued sprites by depth and draws them
 * back-to-front through the camera. A single @c add covers both sorted and
 * unsorted use: equal depths keep call order (the sort is stable), so "unsorted"
 * is just "give everything the same depth". It is a pure draw sink -- it holds
 * only the transform (camera + viewport + plane) and never decides *which*
 * sprites to draw. Reusable beyond actor layers (HUD, particles, floating text).
 */

#include <cstddef>
#include <variant>
#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/video/draw.hh>
#include <neutrino/video/geometry_types.hh>
#include <neutrino/video/world/camera.hh>
#include <neutrino/video/sprite/sprite_sheet.hh>
#include <neutrino/video/sprite/sprite_state.hh>
#include <neutrino/world/world_layers.hh>

namespace neutrino {
    /// @brief What a batch entry draws: a static visual or an animated runtime state.
    using batch_visual = std::variant <sprite_visual_ref, sprite_state_id>;

    /**
     * @brief One resolved, screen-space draw produced by @ref sprite_batch::plan.
     */
    struct sprite_draw {
        point             position; ///< Screen pixel (viewport top-left already added).
        batch_visual      visual;   ///< The visual/state to draw.
        sprite_draw_params params;  ///< Draw transform; @ref sprite_draw_params::scale already folds camera zoom.
    };

    /**
     * @brief A camera-aware, depth-sorted sprite draw sink: fill it with @ref add during
     *        a draw pass, then @ref flush sorts by depth and draws back-to-front.
     */
    class NEUTRINO_EXPORT sprite_batch {
        public:
            /**
             * @param cam      Active camera (copied; small).
             * @param viewport Destination rectangle in renderer pixels.
             * @param plane    Layer whose parallax/offset this batch draws on. Must
             *                 outlive the batch.
             */
            sprite_batch(const camera& cam, rect viewport, const world_layer_header& plane);

            /// @brief Queue a static visual at world @p pos, sorted by @p depth (usually pos.y).
            void add(world_point pos, float depth, sprite_visual_ref visual, sprite_draw_params params = {});
            /// @brief Queue an animated runtime state; its current frame resolves at flush/plan.
            void add(world_point pos, float depth, sprite_state_id state, sprite_draw_params params = {});

            /**
             * @brief Stable-sort the queued sprites by depth ascending and resolve each
             *        to a screen-space @ref sprite_draw. Does not draw or clear.
             *
             * Position is @ref to_screen for the plane plus the viewport top-left; the
             * result's @c params.scale is the caller's scale times the camera zoom.
             */
            [[nodiscard]] std::vector <sprite_draw> plan() const;

            /// @brief @ref plan the queued sprites, draw each back-to-front, then clear.
            ///        A per-sprite content error is a no-op (never throws).
            void flush();

            /// @brief True when no sprites are queued (nothing added since construction/flush).
            [[nodiscard]] bool empty() const noexcept { return m_entries.empty(); }
            /// @brief Number of sprites currently queued (awaiting @ref flush).
            [[nodiscard]] std::size_t size() const noexcept { return m_entries.size(); }

        private:
            struct entry {
                world_point        pos;
                float              depth;
                batch_visual       visual;
                sprite_draw_params params;
            };

            camera                    m_cam;
            rect                      m_viewport;
            const world_layer_header* m_plane;
            std::vector <entry>       m_entries;
    };
}
