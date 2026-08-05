//
// Created by igor on 10/07/2026.
//

#pragma once

/**
 * @file sprite_batch.hh
 * @brief A depth-sorted sprite draw sink, screen-space or camera-aware.
 *
 * Fill a batch with @ref sprite_batch::add during a draw pass, then @ref
 * sprite_batch::flush stable-sorts the queued sprites by depth and draws them
 * back-to-front. A single @c add covers both sorted and unsorted use: equal depths
 * keep call order (the sort is stable), so "unsorted" is just "give everything the
 * same depth". It is a pure draw sink -- it holds only the transform and never
 * decides *which* sprites to draw.
 *
 * Two modes. The **default** constructor makes a *screen-space* batch: @c add
 * positions are literal renderer pixels -- no camera, parallax plane, or zoom. That
 * is the natural sink for a HUD, floating text, or compositing into a
 * @ref render_texture. The **camera** constructor makes a *world-space* batch that
 * runs each position through @ref to_screen for a camera + parallax plane, for actor
 * layers over a scrolling map.
 */

#include <cstddef>
#include <optional>
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
     * @brief A coarse ordering band, sorted before @c depth.
     *
     * The batch's full sort key is `(layer, depth, insertion order)`. Layers separate
     * *categories* that must stack in a fixed order regardless of position — sparks always over
     * capsules, capsules always over actors — while @c depth (usually `pos.y`) orders *within*
     * a category, and the stable sort keeps call order for exact ties.
     *
     * Without this a caller has only the one float, so category order has to be faked by adding
     * a constant big enough to clear the depth range: KE drew balls at `y + 1000`, capsules at
     * `y + 1500` and sparks at `y + 2000`. Those numbers silently encode an assumption about how
     * large `y` can get, and they collapse the moment a world is taller than the gap. A separate
     * band cannot collide with a coordinate.
     *
     * Games name their own; the engine assigns no meaning beyond ordering. Default `{0}` is what
     * the depth-only @ref sprite_batch::add overloads use, so mixing the two is well-defined.
     */
    struct draw_layer {
        int value{};

        [[nodiscard]] friend constexpr bool operator==(draw_layer, draw_layer) = default;
        [[nodiscard]] friend constexpr auto operator<=>(draw_layer, draw_layer) = default;
    };

    /**
     * @brief One resolved, screen-space draw produced by @ref sprite_batch::plan.
     */
    struct sprite_draw {
        point             position; ///< Screen pixel (viewport top-left already added).
        batch_visual      visual;   ///< The visual/state to draw.
        sprite_draw_params params;  ///< Draw transform; @ref sprite_draw_params::scale already folds camera zoom.
    };

    /**
     * @brief A depth-sorted sprite draw sink: fill it with @ref add during a draw pass,
     *        then @ref flush sorts by depth and draws back-to-front.
     */
    class NEUTRINO_EXPORT sprite_batch {
        public:
            /**
             * @brief A screen-space batch: @ref add positions are literal renderer pixels,
             *        with no camera, parallax plane, or zoom applied.
             */
            sprite_batch() = default;

            /**
             * @brief A world-space batch: each @ref add position is transformed by the
             *        camera and parallax plane before drawing.
             *
             * @param cam      Active camera (copied; small).
             * @param viewport Destination rectangle in renderer pixels.
             * @param plane    Layer whose parallax/offset this batch draws on. Must
             *                 outlive the batch.
             */
            sprite_batch(const camera& cam, rect viewport, const world_layer_header& plane);

            /// @brief Queue a static visual at @p pos (world or screen space per the ctor),
            ///        sorted by @p depth (usually pos.y).
            void add(world_point pos, float depth, sprite_visual_ref visual, sprite_draw_params params = {});
            /// @brief Queue a static visual, or do nothing when @p visual is nullopt. Lets a
            ///        caller forward a lookup result -- @c set.visual(i), @ref find_visual_ref --
            ///        straight to the batch without unwrapping or guarding the optional.
            void add(world_point pos, float depth, std::optional <sprite_visual_ref> visual,
                     sprite_draw_params params = {});
            /// @brief Queue an animated runtime state; its current frame resolves at flush/plan.
            void add(world_point pos, float depth, sprite_state_id state, sprite_draw_params params = {});

            /// @brief Queue a static visual in an explicit @ref draw_layer, sorted by @p depth
            ///        within that layer. @see draw_layer
            void add(world_point pos, draw_layer layer, float depth, sprite_visual_ref visual,
                     sprite_draw_params params = {});
            /// @copydoc add(world_point, draw_layer, float, sprite_visual_ref, sprite_draw_params)
            /// Does nothing when @p visual is nullopt.
            void add(world_point pos, draw_layer layer, float depth, std::optional <sprite_visual_ref> visual,
                     sprite_draw_params params = {});
            /// @brief Queue an animated runtime state in an explicit @ref draw_layer.
            void add(world_point pos, draw_layer layer, float depth, sprite_state_id state,
                     sprite_draw_params params = {});

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
                draw_layer         layer;
                float              depth;
                batch_visual       visual;
                sprite_draw_params params;
            };

            /// @brief The world-space transform, present only for a camera batch.
            struct world_transform {
                camera                    cam;
                rect                      viewport;
                const world_layer_header* plane;
            };

            std::optional <world_transform> m_world;   ///< nullopt => screen-space (identity).
            std::vector <entry>             m_entries;
    };
}
