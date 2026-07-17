//
// Created by igor on 07/07/2026.
//

#pragma once

/**
 * @file tileset_bundle.hh
 * @brief Render resources for one tileset: a local-tile-id facade over a
 *        @ref render_bundle, plus the @ref build_bundle / @ref destroy_bundle
 *        routines that pack, register, and tear those resources down.
 *
 * A @ref tileset_bundle maps each @ref world_local_tile_id to a registered
 * @ref sprite_visual_ref (and, for animated tiles, a shared @ref sprite_state_id),
 * so the tile-draw hot path resolves a tile to a drawable with a single indexed
 * load. @ref build_bundle does the throwing packing/decoding work up front and
 * registers atomically; @ref destroy_bundle (or the bundle's RAII destructor)
 * releases in dependency order.
 */

#include <vector>

#include <neutrino/neutrino_export.h>
#include <neutrino/world/world.hh>
#include <neutrino/video/sprite/render_bundle.hh>
#include <neutrino/video/sprite/sprite_sheet.hh>
#include <neutrino/video/sprite/sprite_state.hh>

namespace neutrino {
    /**
     * @brief Registered render resources for one @ref world_tileset.
     *
     * A local-tile-id facade over a @ref render_bundle (which owns the atlases,
     * sheets, animations, and shared states and tears them down in order). @ref
     * build_bundle packs a tileset's tiles into atlas page(s), uploads them, and
     * exposes each local tile as a registered @ref sprite_visual_ref; animated tiles
     * additionally get one shared @ref sprite_state_id (in @ref render_bundle::states,
     * indexed by local id) playing their frames.
     *
     * Lookup is by @ref world_local_tile_id, and those ids are small and dense
     * (@c 0..tile_count-1), so the maps are flat vectors indexed directly by id -- a
     * single contiguous load on the tile-draw hot path, no hashing. Absent tiles
     * (a sparse collection tileset's gaps) hold an invalid entry; @ref visual /
     * @ref state bounds-check and return the invalid sentinel.
     *
     * Because a @ref sprite_sheet backs exactly one atlas, a tileset that spills
     * past the page cap yields several pages, sheets, and the @ref visuals map
     * points each tile at whichever page/sheet holds it -- callers never see the
     * page split.
     *
     * Move-only and RAII via the @ref render_bundle base (which owns/tears down the
     * registered ids). The move operations are explicit so they *swap* @ref visuals too,
     * leaving a moved-from bundle provably empty rather than in @c std::vector's
     * valid-but-unspecified moved-from state -- keeping the whole bundle's "empty after
     * move" contract uniform. The destructor is implicit (the base releases).
     */
    struct tileset_bundle : render_bundle {
        std::vector <sprite_visual_ref> visuals; ///< Indexed by local tile id; invalid = absent.

        tileset_bundle()                                 = default;
        tileset_bundle(tileset_bundle&&) noexcept;
        tileset_bundle& operator=(tileset_bundle&&) noexcept;

        /**
         * @brief Registered visual for a local tile, or an invalid ref if absent.
         */
        [[nodiscard]] sprite_visual_ref visual(world_local_tile_id id) const noexcept {
            return id < visuals.size() ? visuals[id] : sprite_visual_ref{};
        }

        /**
         * @brief Shared animation state for a local tile, or an invalid id if the
         *        tile is not animated.
         */
        [[nodiscard]] sprite_state_id state(world_local_tile_id id) const noexcept {
            return id < states.size() ? states[id] : sprite_state_id{};
        }
    };

    /**
     * @brief Build and register the render resources for one tileset.
     *
     * Uniform grid tiles pack sub-rectangles of the shared image; collection tiles
     * pack each tile's own decoded image; mixed tilesets are handled per tile. Every
     * tile is packed with a gutter belt (no bleed under filtering/scale), so uniform
     * and collection follow one path. Distinct source images are decoded once.
     *
     * All decoding and packing (the throwing work) happens before any resource is
     * registered; if a later registration throws, everything already registered is
     * torn down before the exception propagates, so a failed build leaks nothing.
     *
     * @pre An application must be initialized (registration touches the sprite
     *      resource manager).
     * @throws std::runtime_error (via @ref load_image) when a source image
     *         cannot be decoded.
     */
    [[nodiscard]] NEUTRINO_EXPORT tileset_bundle build_bundle(const world_tileset& tileset);

    /**
     * @brief Build a tileset bundle with an explicit per-page size cap.
     *
     * Identical to @ref build_bundle(const world_tileset&) but packs into pages no
     * larger than @p max_page_size instead of resolving the cap from the active
     * renderer. Use it when a tileset must spill across several atlas pages under a
     * known bound (and in tests, to force the multi-page path deterministically).
     * The page pixel format is still inferred from the source images.
     *
     * @pre @p max_page_size must be positive and at least as large as the biggest
     *      tile plus its gutter, or packing throws (a tile can never fit a page).
     */
    [[nodiscard]] NEUTRINO_EXPORT tileset_bundle build_bundle(const world_tileset& tileset,
                                                              dim max_page_size);

    /**
     * @brief Eagerly unregister a bundle's resources in dependency order and clear it.
     *
     * Optional: the destructor performs the same teardown, so a bundle need not be
     * passed here to release cleanly. Use it to free GPU/sprite resources at a precise
     * point rather than at scope exit. After it runs the bundle is empty, so the later
     * destructor (and any repeat call) is a harmless no-op.
     *
     * Order is states -> animations -> sheets -> atlases, the reverse of the
     * registration dependencies (a state references an animation, an animation and a
     * fixed state reference sheet visuals, a sheet uses an atlas). Invalid handles
     * are ignored, so double teardown is a no-op.
     */
    NEUTRINO_EXPORT void destroy_bundle(tileset_bundle& bundle) noexcept;
}
