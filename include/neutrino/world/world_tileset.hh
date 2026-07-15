//
// Created by igor on 05/07/2026.
//

#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/world/world_common.hh>
#include <neutrino/world/world_layers.hh>
#include <neutrino/video/geometry_types.hh>

namespace neutrino {
    /**
     * @brief One frame of a tile animation.
     */
    struct world_tile_animation_frame {
        world_local_tile_id tile{};           ///< Local tile id shown for this frame.
        std::chrono::milliseconds duration{}; ///< How long the frame is shown.
    };

    /**
     * @brief Per-tile metadata inside a tileset.
     */
    struct world_tile : world_component {
        world_local_tile_id id{};          ///< Tileset-local tile id.
        std::optional <world_image> image; ///< The tile's own image (collection tilesets); unset for a uniform grid tile.
        /// Sub-rectangle of @ref image this tile occupies. Used by image-collection
        /// tilesets that slice one shared image into per-tile regions; when unset the
        /// tile is the whole image.
        std::optional <rect> source_rect;
        std::optional <world_object_layer> objects; ///< Per-tile collision/anchor objects, if any.
        std::vector <world_tile_animation_frame> animation; ///< Animation frames; empty for a static tile.
    };

    /**
     * @brief Optional grid metadata for isometric tilesets.
     */
    struct world_tileset_grid {
        bool orthogonal{true}; ///< true = orthogonal grid; false = isometric.
        unsigned width{};      ///< Grid cell width in pixels.
        unsigned height{};     ///< Grid cell height in pixels.
    };

    /**
     * @brief Format-neutral, render-neutral description of how to draw one tile.
     *
     * Unifies the two tileset kinds so callers never special-case them: for a
     * uniform grid tileset @ref image is the shared tileset image and @ref src is
     * the tile's sub-rectangle; for a collection-of-images tileset @ref image is
     * the tile's own image and @ref src covers all of it. @ref origin is the
     * tileset's per-tile draw offset in pixels (applied by the renderer).
     * @ref animated is true when the tile has an animation — resolve its frames
     * with @ref world_tileset::animation_of. Pure data: the bridge turns @ref image
     * into a texture; world never touches the GPU.
     */
    struct tile_drawable {
        const world_image* image{nullptr}; ///< Shared tileset image, or the tile's own image.
        rect               src{};          ///< Source sub-rect of @ref image for this tile.
        point              origin{};       ///< Tileset per-tile draw offset, in pixels.
        bool               animated{false}; ///< Whether this tile has an animation.
    };

    /**
     * @brief Tileset metadata and tile rectangles.
     */
    struct NEUTRINO_EXPORT world_tileset : world_component {
        unsigned first_gid{};   ///< Global id of this tileset's local tile 0.
        std::string name;       ///< Tileset name.
        unsigned tile_width{};  ///< Tile width in pixels (uniform grid).
        unsigned tile_height{}; ///< Tile height in pixels (uniform grid).
        unsigned spacing{};     ///< Gap in pixels between tiles in the shared image.
        unsigned margin{};      ///< Border in pixels around the shared image before the first tile.
        unsigned tile_count{};  ///< Number of tiles (0 => derive from the shared image).
        unsigned columns{};     ///< Columns in the shared image (0 => derive from its width).
        int offset_x{};         ///< Per-tile draw offset in x, in pixels.
        int offset_y{};         ///< Per-tile draw offset in y, in pixels.
        std::optional <world_image> image;       ///< Shared image for a uniform grid tileset; unset for a collection.
        std::optional <world_tileset_grid> grid; ///< Optional grid metadata (isometric tilesets).
        std::vector <world_tile> tiles;          ///< Per-tile metadata; sparse for collection tilesets.

        [[nodiscard]] const world_tile* tile(world_local_tile_id id) const noexcept;
        [[nodiscard]] rect tile_rect(world_local_tile_id id) const;

        /**
         * @brief The number of grid slots in a uniform (shared image) tileset.
         *
         * Returns @ref tile_count when it is known; otherwise derives it from the
         * shared image dimensions (as Tiled does), so a hand-authored or direct-built
         * tileset that omitted tile_count still yields the right slot count instead of
         * building zero visuals. Returns 0 when it cannot be derived (no shared image,
         * or zero tile size) -- collection tilesets number their slots from @ref tiles.
         */
        [[nodiscard]] unsigned effective_tile_count() const noexcept;

        /**
         * @brief Resolve how to draw a local tile, hiding uniform vs collection.
         *
         * Uniform tileset: @ref tile_drawable::image is the shared image and
         * @ref tile_drawable::src is @ref tile_rect(id). Collection tileset (the
         * tile owns an image): its image, with @c src covering the whole image.
         *
         * @throws (via @ref tile_rect) for a uniform tile with no shared image or
         *         an out-of-range id.
         */
        [[nodiscard]] tile_drawable drawable(world_local_tile_id id) const;

        /**
         * @brief The animation frames of a tile, or nullptr if it is not animated.
         */
        [[nodiscard]] const std::vector <world_tile_animation_frame>*
            animation_of(world_local_tile_id id) const noexcept;

        /**
         * @brief Whether this tileset owns the given global tile id.
         *
         * The empty gid (0) belongs to no tileset. When tile_count is zero the
         * upper bound is unknown and any gid at or above first_gid matches.
         */
        [[nodiscard]] bool contains(world_tile_id gid) const noexcept;

        /**
         * @brief Convert a global tile id to a tileset-local id.
         *
         * @throws std::out_of_range when gid is below first_gid (including the
         *         empty gid 0); use world::tileset_for to pick the owning tileset.
         */
        [[nodiscard]] world_local_tile_id to_local(world_tile_id gid) const;
        [[nodiscard]] world_tile_id to_global(world_local_tile_id id) const noexcept;
    };
}
