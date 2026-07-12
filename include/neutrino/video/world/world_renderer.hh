//
// Created by igor on 08/07/2026.
//

#pragma once

#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/world/world.hh>
#include <neutrino/video/world/camera.hh>
#include <neutrino/video/world/resource_cache.hh>

namespace neutrino {
    struct sprite_draw_params; // <neutrino/video/draw.hh>

    /**
     * @brief Outcome tallies for one @ref world_renderer::draw call.
     *
     * Empty cells (gid 0) are not counted -- they are absent, not skipped. A
     * non-empty gid that resolves to no tileset or an invalid visual increments
     * @ref skipped; a per-tile draw error increments @ref failed; a successful
     * tile draw increments @ref drawn. A content problem never throws, so a single
     * bad cell cannot kill a scene.
     */
    struct draw_stats {
        std::size_t drawn{0};   ///< Tiles successfully drawn.
        std::size_t skipped{0}; ///< Non-empty cells whose gid did not resolve to a visual.
        std::size_t failed{0};  ///< Tiles whose draw call returned an error.
    };

    /**
     * @brief Per-level binding between a @ref world and its render resources.
     *
     * On construction it acquires one @ref bundle_handle per tileset in the world
     * from a @ref resource_cache and holds them in a table aligned with
     * @ref world::tilesets (index i is the bundle for tileset i). On destruction it
     * releases every handle. This is the object whose lifetime drives the cache's
     * refcounts: a tileset shared between two live renderers stays resident because
     * both hold it.
     *
     * By default it binds to the application-wide cache published through the
     * service locator, so call sites just write `world_renderer r(level)`. A second
     * constructor takes an explicit cache for tests and tools.
     *
     * Move-only. A moved-from renderer holds no handles and releases nothing. Use
     * @ref switch_to for a level switch: it acquires the next level's bundles before
     * releasing the current level's, so a shared tileset never drops to refcount
     * zero (and never rebuilds) across the switch.
     *
     * @ref draw renders the world's finite orthogonal tile layers (static and
     * animated tiles), tile objects, and image layers.
     */
    class NEUTRINO_EXPORT world_renderer {
        public:
            /**
             * @brief Bind to @p w, acquiring bundles from the application-wide cache.
             * @pre The application is ready (the service locator publishes a cache).
             * @throws (via @ref resource_cache::acquire) if a tileset cannot build.
             */
            explicit world_renderer(const world& w);

            /**
             * @brief Bind to @p w, acquiring bundles from an explicit @p cache.
             * @throws (via @ref resource_cache::acquire) if a tileset cannot build.
             */
            world_renderer(const world& w, resource_cache& cache);

            ~world_renderer();

            world_renderer(const world_renderer&)            = delete;
            world_renderer& operator=(const world_renderer&) = delete;

            world_renderer(world_renderer&& other) noexcept;
            world_renderer& operator=(world_renderer&& other) noexcept;

            /**
             * @brief Rebind to @p next, acquiring its bundles before releasing the
             *        current level's (acquire-before-release).
             *
             * Reuses the cache this renderer was constructed against. Encodes the one
             * ordering a level manager could get backwards, so call sites cannot.
             */
            void switch_to(const world& next);

            /// @brief Number of tilesets (and handles) this renderer holds.
            [[nodiscard]] std::size_t tileset_count() const noexcept { return m_handles.size(); }

            /**
             * @brief The map's parallax rest point (its @ref world::parallax_origin),
             *        which @ref draw injects into the camera.
             *
             * A compositor needs it to place game-drawn layers on the same parallax
             * planes as the tile layers. Returns the origin, or (0,0) if moved-from.
             */
            [[nodiscard]] world_point parallax_rest() const noexcept;

            /**
             * @brief The bundle handle for tileset @p index (aligned with
             *        @ref world::tilesets).
             * @throws std::out_of_range if @p index is past the tileset count.
             */
            [[nodiscard]] const bundle_handle& handle_for_tileset(std::size_t index) const {
                return m_handles.at(index);
            }

            /**
             * @brief The synthetic bundle backing an image layer's picture (static or
             *        animated), or nullptr if the layer carries no image.
             *
             * Mirrors @ref handle_for_tileset for the per-image-layer bundles. An
             * animated image layer's current frame is @c state(0); a static one is
             * @c visual(0).
             */
            [[nodiscard]] const bundle_handle* image_layer_handle(const world_image_layer& layer) const noexcept;

            /**
             * @brief The pixel height of the frame an image layer currently shows -- the
             *        height the draw path anchors that frame by.
             *
             * For an animated layer this tracks the current frame (so mixed-height frames
             * stay top-left aligned); for a static one it is the image height. 0 if the
             * layer carries no image.
             */
            [[nodiscard]] unsigned image_layer_current_height(const world_image_layer& layer) const;


            /**
             * @brief Draw the world's tile, object, and image layers into @p viewport.
             *
             * Iterates visible layers in map order. Tile layers cull to the camera
             * (inflated for oversized-tile overhang) and draw each non-empty cell with
             * its flip and hex-rotation; tile objects (gid) draw at their origin with
             * rotation/flip; image layers draw their picture once. @p viewport is a
             * destination rectangle in the current renderer's pixels: its size drives
             * culling, its top-left translates every screen position. Animated tiles
             * resolve off the shared clock. Never throws on a per-item content problem;
             * see @ref draw_stats.
             */
            draw_stats draw(const camera& cam, const rect& viewport);

            /**
             * @brief Draw as @ref draw, firing @p after_layer after each map layer is
             *        drawn, in map order, with that layer's id.
             *
             * The seam a compositor uses to interleave game-drawn content between static
             * layers. The renderer stays agnostic of what the callback draws.
             */
            draw_stats draw(const camera& cam, const rect& viewport,
                            const std::function <void(world_layer_id done)>& after_layer);

        private:
            void draw_layer(const world_tile_layer& layer, const camera& cam, const rect& viewport, draw_stats& stats) const;
            void draw_layer(const world_image_layer& layer, const camera& cam, const rect& viewport, draw_stats& stats) const;
            void draw_layer(const world_object_layer& layer, const camera& cam, const rect& viewport, draw_stats& stats) const;
            /// Resolve @p gid to its bundle visual (or shared animation state) and draw
            /// it at @p pos, updating @p stats. Shared by the tile-cell and tile-object
            /// paths. Counts a skip for an unresolvable gid, never throws.
            /// Draw the tile @p gid with its bottom-left at world @p anchor, composing
            /// @p flip / @p rotation. Non-rotated, non-diagonal tiles use the seam-free
            /// two-corner path (dst from two rounded world corners) so adjacent tiles
            /// and tiled objects meet exactly under fractional zoom; rotated/diagonal
            /// ones use the anchor/scale path. Shared by tile cells and tile objects.
            /// An unresolvable gid counts a skip; never throws.
            void draw_gid_at(world_tile_id gid, const world_point& anchor, sprite_flip flip, float rotation,
                             const world_layer_header& layer, const camera& cam, const rect& viewport,
                             draw_stats& stats) const;
            /// Draw one tile cell at world grid coordinate (@p cx, @p cy). Shared by the
            /// finite-grid and infinite-chunk paths; empty cells are a no-op.
            void draw_cell(const world_tile_cell& cell, int cx, int cy, const world_layer_header& layer,
                           const camera& cam, const rect& viewport, draw_stats& stats) const;
            void release_all() noexcept;

            const world*               m_world{nullptr};
            resource_cache*            m_cache{nullptr};
            std::vector<bundle_handle> m_handles;     ///< Index-aligned with m_world->tilesets().
            int                        m_overhang_cells{0}; ///< Cull inflation for tiles larger than the map cell.
            /// One content-keyed bundle per image layer that carries an image (a
            /// synthetic single-tile tileset), keyed by the layer's stable address.
            std::unordered_map<const world_image_layer*, bundle_handle> m_image_handles;
    };
}
